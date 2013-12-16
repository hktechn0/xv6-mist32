// DPS Timer
// Only used on uniprocessors;
// SMP machines use the local APIC timer.

#include "types.h"
#include "defs.h"
#include "traps.h"
#include "mist32.h"

#define DPS_UTIM64A 0x000
#define DPS_UTIM64B 0x040
#define DPS_UTIM64FLAGS 0x07c

// Frequency of all three count-down timers;
// (TIMER_FREQ/freq) is the appropriate count
// to generate a frequency of freq Hz.

#define TIMER_FREQ      49152000
#define TIMER_DIV(x)    ((TIMER_FREQ+(x)/2)/(x))

#define UTIM64MCFG_ENA 0x1
#define UTIM64CFG_ENA 0x1
#define UTIM64CFG_IE 0x2
#define UTIM64CFG_BIT 0x4
#define UTIM64CFG_MODE 0x8

typedef volatile struct _dps_utim64 {
  volatile unsigned int mcfg;
  volatile unsigned int mc[2];
  volatile unsigned int cc[4][2];
  volatile unsigned int cccfg[4];
} dps_utim64;

volatile uint *utim64_flags;

void
timerinit(void)
{
  dps_utim64 *a, *b;

  // Interrupt 100 times/sec.
  a = (dps_utim64 *)((char *)sriosr() + DPS_UTIM64A);
  b = (dps_utim64 *)((char *)sriosr() + DPS_UTIM64B);
  utim64_flags = (uint *)((char *)sriosr() + DPS_UTIM64FLAGS);

  a->cc[0][1] = TIMER_DIV(100);
  a->cccfg[0] = UTIM64CFG_MODE | UTIM64CFG_IE | UTIM64CFG_ENA;
  a->mcfg = UTIM64MCFG_ENA;

  b->mcfg = 0;

  idtenable(IRQ_TIMER);
}

void
timereoi(void)
{
  asm volatile("ld32 r0, %0" : : "r"(utim64_flags));
}
