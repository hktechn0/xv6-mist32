#include "types.h"
#include "defs.h"
#include "param.h"
#include "stat.h"
#include "mist32.h"
#include "memlayout.h"
#include "mmu.h"
#include "buf.h"
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
uint omap_victim_next;
struct flashmmu_pool *omap_objcache_pool[4] = { NULL };
struct flashmmu_pool *omap_pool_freelist = NULL;

#define OMAP_FLASH_DEV 1
#define OMAP_FLASH_OFFSET ((1024 * 1024) >> 9)

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

// merge sort
static struct flashmmu_pool *
omap_slab_sort(struct flashmmu_pool *list)
{
  struct flashmmu_pool *a, *b, *p, head;

  if(list == NULL || list->next == NULL) {
    // length is 0 or 1
    return list;
  }

  // split list
  a = list;
  b = list->next->next;

  while(b != NULL) {
    a = a->next;
    b = b->next;

    if(b != NULL) {
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

static void
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
    slab2->p = (char *)slab->p + ((1 << (pool + 1)) << 8);

    slab->next = omap_objcache_pool[pool];
    slab2->next = slab;
    omap_objcache_pool[pool] = slab2;
    break;
  case 3:
    // sort and merge
    // FIXME: compaction
    for(i = 0; i < 3; i++) {
      slab = omap_slab_sort(omap_objcache_pool[i]);
      omap_objcache_pool[i] = slab;

      // slab merging
      while(slab) {
        if(slab->next == NULL || slab->next->next == NULL) {
          break;
        }

        // is continuous free area?
        if((char *)slab->p + ((1 << (i + 1)) << 8) == (char *)slab->next->p) {
          slab2 = slab->next;
          slab2->p = slab->p;

          slab->p = slab2->next->p;
          slab->next = slab2->next->next;
          omap_pool_list_release(slab2->next);

          // link big slab
          slab2->next = omap_objcache_pool[i + 1];
          omap_objcache_pool[i + 1] = slab2;
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

static inline void *
omap_objcache_alloc(uint size)
{
  uint i;
  struct flashmmu_pool *l;
  void *p;

  if(!size) {
    // zero size object
    return NULL;
  }

  // calculate blocks
  if(size <= 512)
    i = 0;
  else if(size <= 1024)
    i = 1;
  else if(size <= 2048)
    i = 2;
  else if(size <= 4096)
    i = 3;
  else
    panic("omap_objcache_alloc");

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

static inline void
omap_objcache_free(uint cache_offset, uint size)
{
  uint i;
  struct flashmmu_pool *l;

  if(!size) {
    // zero size object
    return;
  }

  // calculate blocks
  if(size <= 512)
    i = 0;
  else if(size <= 1024)
    i = 1;
  else if(size <= 2048)
    i = 2;
  else if(size <= 4096)
    i = 3;
  else
    panic("omap_objcache_free");

  // set free pointer
  l = omap_pool_list_new();
  l->p = flashmmu_objcache + (cache_offset << 9);

  // push slab to free pool
  l->next = omap_objcache_pool[i];
  omap_objcache_pool[i] = l;
}

void
omap_init(void)
{
  struct flashmmu_pool *l;
  char *p;

  omap_objects_next = 0;
  omap_victim_next = 0;

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

    // push slab
    l->next = omap_objcache_pool[3];
    omap_objcache_pool[3] = l;
  }
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
  //if(!(omap_flash[objid] = kalloc())) {
  //  panic("omap_objalloc nomem");
  //}

  // set table
  omap_objects[objid].size = size;
  omap_objects[objid].ref = 0;
  omap_objects[objid].flags = FLASHMMU_FLAGS_VALID;

  return objid;
}

void
omap_objfree(uint objid)
{
  if(!(omap_objects[objid].flags & FLASHMMU_FLAGS_VALID)) {
    panic("omap_objfree");
  }

  // free Page Buffer
  if(omap_objects[objid].flags & FLASHMMU_FLAGS_PAGEBUF) {
    omap_pagebuf[omap_objects[objid].buf_index] = 0;
  }

  // free RAM object cache
  if(omap_objects[objid].flags & FLASHMMU_FLAGS_OBJCACHE) {
    omap_objcache_free(omap_objects[objid].cache_offset, omap_objects[objid].size);
  }

  // free table entry
  omap_objects[objid].flags = 0;
  omap_objects[objid].buf_index = 0;
  omap_objects[objid].cache_offset = 0;
  omap_objects[objid].ref = 0;

  // free Flash
  //kfree(omap_flash[objid]);
  //omap_flash[objid] = 0;
}

void
omap_pgfault(uint objid)
{
  struct buf *b;
  char *p;
  uint i;
  uint victim;

  // find free area
  p = omap_objcache_alloc(omap_objects[objid].size);

  if(p == NULL) {
    // refill pagebuf
    do {
      if(omap_victim_next >= FLASHMMU_OBJ_MAX) {
        omap_victim_next = 0;
      }

      if((omap_objects[omap_victim_next].flags & FLASHMMU_FLAGS_VALID) &&
         (omap_objects[omap_victim_next].flags & FLASHMMU_FLAGS_OBJCACHE) &&
         !(omap_objects[omap_victim_next].flags & FLASHMMU_FLAGS_PAGEBUF)) {
        // hit
        victim = omap_victim_next;

        if(omap_objects[victim].flags & FLASHMMU_FLAGS_DIRTY) {
          for(i = 0; i < FLASHMMU_BLOCKS(omap_objects[victim].size); i++) {
            b = bread(OMAP_FLASH_DEV, OMAP_FLASH_OFFSET + FLASHMMU_SECTOR(victim) + i);
            memmove(b->data, flashmmu_objcache + ((omap_objects[victim].cache_offset + i) << 9), 512);
            bwrite(b);
            brelse(b);
          }
        }

        omap_objects[victim].flags &= ~(FLASHMMU_FLAGS_OBJCACHE | FLASHMMU_FLAGS_DIRTY);
        omap_objcache_free(omap_objects[victim].cache_offset, omap_objects[victim].size);
        //cprintf("IN %x OUT %x\n", objid, victim);
      }

      // miss
      omap_victim_next++;
    } while(!(p = omap_objcache_alloc(omap_objects[objid].size)));
  }

  omap_objects[objid].cache_offset = (uint)(p - flashmmu_objcache) >> 9;

  // copy to RAM object cache
  for(i = 0; i < FLASHMMU_BLOCKS(omap_objects[objid].size); i++) {
    b = bread(OMAP_FLASH_DEV, OMAP_FLASH_OFFSET + FLASHMMU_SECTOR(objid) + i);
    memmove(p + (512 * i), b->data, 512);
    brelse(b);
  }

  omap_objects[objid].flags |= FLASHMMU_FLAGS_OBJCACHE;
}
