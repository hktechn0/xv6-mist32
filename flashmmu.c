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
static char *flashmmu_pagebuf;
static char *flashmmu_objcache;
// object table
static struct flashmmu_object *omap_objects;

// Flash MMU variables
static uint omap_objects_next;
static uint omap_pagebuf_next;

// page buffer entry
static uint omap_pagebuf[FLASHMMU_OBJ_MAX] = { 0 };

// RAM Object Cache pool
static struct flashmmu_pool *omap_objcache_pool[4] = { NULL };
static struct flashmmu_pool *omap_objcache_used = NULL;
static struct flashmmu_pool *omap_objcache_used_last = NULL;

// list element free pool
static struct flashmmu_pool *omap_pool_freelist = NULL;

// benchmark
static uint omap_pagefault = 0;

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
omap_objcache_alloc(uint objid)
{
  uint i, size;
  char *p;
  struct flashmmu_pool *l;

  size = omap_objects[objid].size;

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
  p = l->p;
  omap_objcache_pool[i] = l->next;

  l->objid = objid;
  l->next = NULL;

  if(omap_objcache_used == NULL) {
    omap_objcache_used = l;
    omap_objcache_used_last = l;
  }
  else {
    omap_objcache_used_last->next = l;
    omap_objcache_used_last = l;
  }

  omap_objects[objid].flags |= FLASHMMU_FLAGS_OBJCACHE;
  omap_objects[objid].cache_offset = (uint)(p - flashmmu_objcache) >> 9;

  return p;
}

static inline void
omap_objcache_free(uint objid)
{
  uint i, size;
  struct flashmmu_pool *l, *pl;

  size = omap_objects[objid].size;

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

  // find in used pool
  l = omap_objcache_used;
  pl = NULL;
  while(l) {
    if(l->objid == objid) {
      break;
    }
    pl = l;
    l = l->next;
  }

  if(l == NULL) {
    panic("omap_objcache_free notfound");
  }

  // drop from used pool
  if(pl == NULL) {
    // head
    omap_objcache_used = l->next;
  }
  else {
    pl->next = l->next;
  }

  if(l->next == NULL) {
    // tail
    omap_objcache_used_last = pl;
  }

  omap_objects[objid].flags &= ~FLASHMMU_FLAGS_OBJCACHE;

  // push slab to free pool
  l->p = flashmmu_objcache + (omap_objects[objid].cache_offset << 9);
  l->next = omap_objcache_pool[i];
  omap_objcache_pool[i] = l;
}

void
omap_init(void)
{
  struct flashmmu_pool *l;
  char *p;

  omap_objects_next = 0;
  omap_pagebuf_next = 0;

  // set cache area
  flashmmu_pagebuf = (char *)FMMUVIRT;
  flashmmu_objcache = (char *)flashmmu_pagebuf + FLASHMMU_PAGEBUF_SIZE;

  // init tables
  omap_objects = (struct flashmmu_object *)((char *)flashmmu_objcache + FLASHMMU_OBJCACHE_SIZE);
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
  omap_objects[objid].flags = FLASHMMU_FLAGS_VALID;

  return objid;
}

void
omap_objfree(uint objid)
{
  uint i;

  if(!(omap_objects[objid].flags & FLASHMMU_FLAGS_VALID)) {
    panic("omap_objfree");
  }

  // free Page Buffer
  if(omap_objects[objid].flags & FLASHMMU_FLAGS_PAGEBUF) {
    for(i = 0; i < FLASHMMU_PAGEBUF_MAX; i++) {
      if(FLASHMMU_OBJID(omap_pagebuf[i]) == objid) {
        omap_pagebuf[i] = 0;
        break;
      }
    }
  }

  // free RAM object cache
  if(omap_objects[objid].flags & FLASHMMU_FLAGS_OBJCACHE) {
    omap_objcache_free(objid);
  }

  // free table entry
  omap_objects[objid].flags = 0;
  omap_objects[objid].cache_offset = 0;

  // free Flash
  //kfree(omap_flash[objid]);
  //omap_flash[objid] = 0;
}

void
omap_pgfault(uint objid)
{
  struct buf *b;
  char *p;
  uint i, victim_id;
  struct flashmmu_pool *l, *victim;

  char *page;
  uint entry;
  pte_t *pte;

  omap_pagefault++;
  cprintf("omap_pagefault: %d\n", omap_pagefault);

  if(omap_objects[objid].flags & FLASHMMU_FLAGS_PAGEBUF) {
    panic("omap_pgfault pagebuf?");
  }
  else if(omap_objects[objid].flags & FLASHMMU_FLAGS_OBJCACHE) {
    goto pagebuf;
  }

  // find free area
  p = omap_objcache_alloc(objid);

  if(p == NULL) {
    // head of used list == oldest object
    victim = omap_objcache_used;

    // refill pagebuf
    do {
      if(victim == NULL) {
        panic("omap_pgfault");
      }

      victim_id = victim->objid;

      // victim should not been recently accessed
      if(omap_objects[victim_id].flags & (FLASHMMU_FLAGS_ACCESS | FLASHMMU_FLAGS_PAGEBUF)) {
        l = victim->next;

        // victim <= victim->next
        victim->objid = l->objid;
        victim->next = l->next;

        // victim links to tail
        l->objid = victim_id;
        l->next = NULL;
        omap_objcache_used_last->next = l;
        omap_objcache_used_last = l;

        // drop FLAGS_ACCESS
        omap_objects[victim_id].flags &= ~FLASHMMU_FLAGS_ACCESS;
        continue;
      }

      // victim writeback
      if(omap_objects[victim_id].flags & FLASHMMU_FLAGS_DIRTY) {
        for(i = 0; i < FLASHMMU_BLOCKS(omap_objects[victim_id].size); i++) {
          b = bread(OMAP_FLASH_DEV, OMAP_FLASH_OFFSET + FLASHMMU_SECTOR(victim_id) + i);
          memmove(b->data, flashmmu_objcache + ((omap_objects[victim_id].cache_offset + i) << 9), 512);
          bwrite(b);
          brelse(b);
        }

        omap_objects[victim_id].flags &= ~(FLASHMMU_FLAGS_DIRTY | FLASHMMU_FLAGS_ACCESS);
      }

      victim = victim->next;

      //cprintf("objcache OUT %x ", victim_id);

      // free objcache and alloc
      omap_objcache_free(victim_id);
      p = omap_objcache_alloc(objid);
    } while(p == NULL);
  }

  // copy to RAM object cache
  for(i = 0; i < FLASHMMU_BLOCKS(omap_objects[objid].size); i++) {
    b = bread(OMAP_FLASH_DEV, OMAP_FLASH_OFFSET + FLASHMMU_SECTOR(objid) + i);
    memmove(p + (512 * i), b->data, 512);
    brelse(b);
  }

pagebuf:
  page = flashmmu_pagebuf + (omap_pagebuf_next << 12);
  entry = omap_pagebuf[omap_pagebuf_next];

  if(entry & FLASHMMU_FLAGS_VALID) {
    // victim writeback
    victim_id = FLASHMMU_OBJID(entry);
    pte = omap_pte(victim_id);

    if(*pte & PTE_D) {
      memmove(flashmmu_objcache + (omap_objects[victim_id].cache_offset << 9), page,
              omap_objects[victim_id].size);
      omap_objects[victim_id].flags |= FLASHMMU_FLAGS_DIRTY;
    }

    if(*pte & PTE_R) {
      omap_objects[victim_id].flags |= FLASHMMU_FLAGS_ACCESS;
    }

    //cprintf("pagebuf OUT %x %x ", victim_id, *(uint *)page);

    // drop flags
    omap_objects[victim_id].flags &= ~FLASHMMU_FLAGS_PAGEBUF;
    *pte &= ~(0xfffff000 | PTE_V | PTE_R | PTE_D);
  }

  // copy
  memmove(page, flashmmu_objcache + (omap_objects[objid].cache_offset << 9),
          omap_objects[objid].size);

  // PTE
  pte = omap_pte(objid);
  *pte = (FMMUSTART + (omap_pagebuf_next << 12)) | PTE_V | (*pte & 0xfff);

  //cprintf("IN %x %x\n", objid, *(uint *)page);

  omap_objects[objid].flags |= FLASHMMU_FLAGS_PAGEBUF;
  omap_pagebuf[omap_pagebuf_next] = FLASHMMU_ADDR(objid) | FLASHMMU_FLAGS_VALID;

  // increment
  if(++omap_pagebuf_next >= FLASHMMU_PAGEBUF_MAX) {
    omap_pagebuf_next = 0;
  }
}
