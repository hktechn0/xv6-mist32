#include <stddef.h>
#include <stdint.h>

// mist32 Flash MMU

#define FLASHMMU_START_ADDR 0x80000000
#define FLASHMMU_AREA_SIZE (					     \
  FLASHMMU_PAGEBUF_SIZE						     \
  + FLASHMMU_OBJCACHE_SIZE					     \
  + FLASHMMU_PAGEBUF_MAX * sizeof(uint32_t)			     \
  + FLASHMMU_OBJ_MAX * sizeof(struct flashmmu_object))

// 32MB
#define FLASHMMU_SIZE 0x04000000
#define FLASHMMU_PAGE_SIZE 0x1000
#define FLASHMMU_OBJ_MAX (FLASHMMU_SIZE / FLASHMMU_PAGE_SIZE)

// 8MB
#define FLASHMMU_PAGEBUF_SIZE 0x00800000
#define FLASHMMU_PAGEBUF_MAX (FLASHMMU_PAGEBUF_SIZE >> 12)
#define FLASHMMU_PAGEBUF_ADDR (FLASHMMU_START_ADDR)

// 16MB
#define FLASHMMU_OBJCACHE_SIZE 0x01000000
#define FLASHMMU_OBJCACHE_ADDR (FLASHMMU_START_ADDR + FLASHMMU_PAGEBUF_SIZE)

// FLAGS
#define FLASHMMU_FLAGS_VALID 0x1
#define FLASHMMU_FLAGS_OBJCACHE 0x2
#define FLASHMMU_FLAGS_PAGEBUF 0x4
#define FLASHMMU_FLAGS_DIRTY 0x8
#define FLASHMMU_FLAGS_DIRTYBUF 0x10

#define FLASHMMU_OBJID(paddr) (paddr >> 12)
#define FLASHMMU_OFFSET(paddr) (paddr & 0x3ff)
#define FLASHMMU_ADDR(objid) (objid << 12)
#define FLASHMMU_SECTOR(objid) (objid << 12 >> 9)
#define FLASHMMU_BLOCKS(size) ((size + 511) >> 9)

#define FLASHMMU_PAGEBUF_FLAGS(entry) (entry & 0x3ff)

#define FLASHMMU_PAGEBUF_OBJ(pagebuf, num) ((char *)pagebuf + (num << 12))
#define FLASHMMU_OBJCACHE_OBJ(objcache, offset) ((char *)objcache + (offset << 9))
#define FLASHMMU_BITMAP_GET(bitmap, offset) (bitmap[offset >> 5] & (1 << (offset & 0x1f)))
#define FLASHMMU_BITMAP_SET(bitmap, offset) (bitmap[offset >> 5] |= (1 << (offset & 0x1f)))
#define FLASHMMU_BITMAP_CLEAR(bitmap, offset) (bitmap[offset >> 5] &= ~(1 << (offset & 0x1f)))

struct flashmmu_object
{
  volatile uint16_t size;
  volatile uint16_t buf_index;
  volatile uint32_t cache_offset;
  volatile uint32_t ref;
  volatile uint32_t flags;
} __attribute__ ((packed));

struct flashmmu_pool
{
  struct flashmmu_pool *next;
  void *p;
};

void omap_init(void);
uint omap_objalloc(int size);
void omap_objfree(uint objid);
void omap_pgfault(uint objid);
