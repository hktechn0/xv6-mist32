#include "param.h"
#include "types.h"
#include "defs.h"
#include "mist32.h"
#include "memlayout.h"
#include "mmu.h"
#include "flashmmu.h"

// mist32 Flash MMU hardware managed area
char *flashmmu_pagebuf;
char *flashmmu_objcache;
// pagebuffer entries
volatile uint *omap_pagebuf;
// object table
struct flashmmu_object *omap_objects;

// Flash MMU variables
uint omap_objects_next;
struct flashmmu_pool *omap_objcache_pool[4];
struct flashmmu_pool *omap_pool_freelist;

//void *omap_flash[FLASHMMU_OBJ_MAX];
void *omap_flash[10000];

static inline struct flashmmu_pool *
omap_pool_list_new(void)
{
  struct flashmmu_pool *l;
  uint i;

  // no free pool list?
  if(omap_pool_freelist == NULL) {
    // alloc slab allocator list free pool
    l = (struct flashmmu_pool *)kalloc();
    for(i = 0; i < (PGSIZE / sizeof(struct flashmmu_pool)); i++) {
      l->next = omap_pool_freelist;
      omap_pool_freelist = l++;
    }
  }

  // get pool free list
  l = omap_pool_freelist;
  omap_pool_freelist = l->next;

  return l;
}

static inline void
omap_pool_list_release(struct flashmmu_pool *l)
{
  // release slab
  l->next = omap_pool_freelist;
  omap_pool_freelist = l;
}

void
omap_init(void)
{
  struct flashmmu_pool *l;
  char *p;

  omap_objects_next = 0;

  // set cache area
  flashmmu_pagebuf = (char *)FMMUVIRT;
  flashmmu_objcache = (char *)flashmmu_pagebuf + FLASHMMU_PAGEBUF_SIZE;

  // init tables
  omap_pagebuf = (uint *)((char *)flashmmu_objcache + FLASHMMU_OBJCACHE_SIZE);
  omap_objects = (struct flashmmu_object *)((char *)omap_pagebuf + (FLASHMMU_PAGEBUF_MAX * sizeof(uint)));
  memset((void *)omap_pagebuf, 0, FLASHMMU_PAGEBUF_MAX * sizeof(uint));
  memset(omap_objects, 0, FLASHMMU_OBJ_MAX * sizeof(struct flashmmu_object));

  // init slab
  for(p = flashmmu_objcache; p < flashmmu_objcache + FLASHMMU_OBJCACHE_SIZE; p += 4096) {
    // make slab
    l = omap_pool_list_new();
    l->p = p;
    l->next = omap_objcache_pool[3];

    // push slab
    omap_objcache_pool[3] = l;
  }
}

// merge sort
struct flashmmu_pool *omap_slab_sort(struct flashmmu_pool *list)
{
  struct flashmmu_pool *a, *b, *p, head;

  if(list == NULL || list->next == NULL) {
    // length is 0 or 1
    return list;
  }

  // split list
  a = list;
  b = list->next->next;

  while(b) {
    a = a->next;
    b = b->next;

    if(b->next) {
      b = b->next;
    }
  }

  b = a->next;
  a->next = NULL;

  // recursion sort
  a = omap_slab_sort(list);
  b = omap_slab_sort(b);

  p = &head;

  // merge
  while(a != NULL && b != NULL) {
    if(a->p <= b->p) {
      // link sorted list
      p->next = a;
      // next sorted
      p = p->next;
      // next unsorted
      a = a->next;
    }
    else {
      p->next = b;
      p = p->next;
      b = b->next;
    }
  }

  // link remaining data
  if(a) {
    p->next = a;
  }
  else {
    p->next = b;
  }

  return head.next;
}

void
omap_slab_make(uint pool)
{
  struct flashmmu_pool *slab, *slab2;
  int i;

  switch(pool) {
  case 0:
  case 1:
  case 2:
    // no bigger slab?
    if(omap_objcache_pool[pool + 1] == NULL) {
      omap_slab_make(pool + 1);

      if(omap_objcache_pool[pool + 1] == NULL) {
        // no slab...
        return;
      }
    }

    // pop bigger slab
    slab = omap_objcache_pool[pool + 1];
    omap_objcache_pool[pool + 1] = slab->next;

    // split slab
    slab2 = omap_pool_list_new();
    slab2->p = (char *)slab->p + ((pool + 1) << 9);

    slab->next = omap_objcache_pool[pool];
    slab2->next = slab;
    omap_objcache_pool[pool] = slab2;    
    break;
  case 3:
    // sort and merge
    for(i = 0; i < 3; i++) {
      slab = omap_slab_sort(omap_objcache_pool[i]);
      omap_objcache_pool[i] = slab;

      // slab merging
      while(slab) {
        if(slab->next == NULL || slab->next->next == NULL) {
          break;
        }

        // is continuous free area?
        if(slab->p + 1 == slab->next->p) {
          slab2 = slab->next;

          slab->p = slab2->next->p;
          slab->next = slab2->next->next;
          omap_pool_list_release(slab2->next);

          // link big slab
          slab2->p = slab->p;
          slab2->next = omap_objcache_pool[i];
          omap_objcache_pool[i] = slab2;
        }

        slab = slab->next;
      }
    }
    break;
  default:
    panic("omap_slab_make");
    break;
  }
}

// search slab pool. returns offset
void *
omap_objcache_search(uint size)
{
  struct flashmmu_pool *l;
  char *p;
  uint i, blocks;

  if(!size) {
    // zero size object
    return NULL;
  }

  // calc blocks
  blocks = FLASHMMU_BLOCKS(size);
  for(i = 0; blocks; i++) {
    blocks >>= 1;
  }
  if(i > 3) {
    panic("omap_objcache_search");
  }

  if(omap_objcache_pool[i] == NULL) {
    // cutting or merging slab
    omap_slab_make(i);

    if(omap_objcache_pool[i] == NULL) {
      // no slab...
      return NULL;
    }
  }

  // pop slab
  l = omap_objcache_pool[i];
  omap_objcache_pool[i] = l->next;
  p = l->p;

  omap_pool_list_release(l);

  return p;
}

uint
omap_objalloc(int size)
{
  uint objid, start;

  start = omap_objects_next;

  // find free id
  while(1) {
    if(omap_objects_next >= FLASHMMU_OBJ_MAX) {
      omap_objects_next = 0;
    }

    if(!omap_objects[omap_objects_next].flags) {
      // hit
      break;
    }

    // miss
    omap_objects_next++;

    if(omap_objects_next == start) {
      panic("omap_objalloc full");
    }
  }

  objid = omap_objects_next++;

  // alloc flash
  if(!(omap_flash[objid] = kalloc())) {
    panic("omap_objalloc nomem");
  }

  // set table
  omap_objects[objid].size = size;
  omap_objects[objid].ref = 0;
  omap_objects[objid].flags = FLASHMMU_FLAGS_VALID;

  return objid;
}

void
omap_objfree(uint objid)
{
  struct flashmmu_pool *l;
  uint i, blocks;

  if(!(omap_objects[objid].flags & FLASHMMU_FLAGS_VALID)) {
    panic("omap_objfree");
  }

  // free Page Buffer
  if(omap_objects[objid].flags | FLASHMMU_FLAGS_PAGEBUF) {
    omap_pagebuf[omap_objects[objid].buf_index] = 0;
  }

  // free RAM object cache
  if(omap_objects[objid].flags | FLASHMMU_FLAGS_OBJCACHE) {
    // calculate blocks
    blocks = FLASHMMU_BLOCKS(omap_objects[objid].size);
    for(i = 0; blocks; i++) {
      blocks >>= 1;
    }
    if(i > 3) {
      panic("omap_objfree");
    }

    // set free pointer
    l = omap_pool_list_new();
    l->p = flashmmu_objcache + (omap_objects[objid].cache_offset << 9);

    // push free list
    l->next = omap_objcache_pool[i];
    omap_objcache_pool[i] = l;
  }

  // free table entry
  omap_objects[objid].flags = 0;

  // free Flash
  kfree(omap_flash[objid]);
  omap_flash[objid] = 0;
}

void
omap_pgfault(uint objid)
{
  char *p;

  // find free area
  p = omap_objcache_search(omap_objects[objid].size);

  if(p == NULL) {
    // refill pagebuf
    panic("omap_pgfault full objcache");
    /*
    if(omap_pagebuf[i] | FLASUMMU_FLAGS_DIRTYBUF) {
      // writeback
      memcpy(FLASHMMU_OBJCACHE_OBJ(flashmmu_objcache, omap_objects[objid].cache_offset),
             ,
             victim->size);
      FLASHMMU_PAGEBUF_OBJ(flashmmu_pagebuf, omap_objects[objid].buf_index);
    }
    */
  }

  // copy to RAM object cache
  memmove(p, omap_flash[objid], omap_objects[objid].size);
  omap_objects[objid].cache_offset = (uint)(p - flashmmu_objcache) >> 9;
  omap_objects[objid].flags |= FLASHMMU_FLAGS_OBJCACHE;
}
