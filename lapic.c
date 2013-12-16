// The local APIC manages internal (non-I/O) interrupts.
// See Chapter 8 & Appendix C of Intel processor manual volume 3.

#include "types.h"
#include "defs.h"
#include "memlayout.h"
#include "traps.h"
#include "mmu.h"
#include "mist32.h"

volatile uint *lapic;  // Initialized in mp.c

void
lapicinit(void)
{
  if(!lapic) 
    return;
}

int
cpunum(void)
{
  return 0;
}
