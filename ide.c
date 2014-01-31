// Simple MMC controller driver for IDE emulation

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "traps.h"
#include "spinlock.h"
#include "buf.h"
#include "gci.h"

struct gci_mmcc *mmcc;
uchar *mmcc_buf;

// 1GB
static int disksize = 0x200000;

void
ideinit(void)
{
  mmcc = gci_nodes[GCI_MMCC_NUM].device_area;
  mmcc_buf = (uchar *)mmcc + MMCC_BUFFER_OFFSET;

  mmcc->init_command = MMCC_INITIAL_COMMAND;

  // idtenable(IRQ_GCIMMCC);
  // ioapicenable(IRQ_IDE, ncpu - 1);
}

// Interrupt handler.
void
ideintr(void)
{
  // no-op
}

// Sync buf with disk. 
// If B_DIRTY is set, write buf to disk, clear B_DIRTY, set B_VALID.
// Else if B_VALID is not set, read buf from disk, set B_VALID.
void
iderw(struct buf *b)
{
  if(!(b->flags & B_BUSY))
    panic("iderw: buf not busy");
  if((b->flags & (B_VALID|B_DIRTY)) == B_VALID)
    panic("iderw: nothing to do");
  if(b->dev != 1)
    panic("iderw: request not for disk 1");
  if(b->sector >= disksize)
    panic("iderw: sector out of range");

  if(b->flags & B_DIRTY){
    // write
    b->flags &= ~B_DIRTY;
    memmove(mmcc_buf, b->data, 512);
    mmcc->sector_write = b->sector << 9;
  } else{
    // read
    mmcc->sector_read = b->sector << 9;
    memmove(b->data, mmcc_buf, 512);
  }

  b->flags |= B_VALID;
}
