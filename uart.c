// DPS SCI module serial port (UART).

#include "types.h"
#include "defs.h"
#include "param.h"
#include "traps.h"
#include "spinlock.h"
#include "fs.h"
#include "file.h"
#include "mmu.h"
#include "proc.h"
#include "mist32.h"

#define DPS_SCI 0x100
#define SCIRXD_VALID 0x80000000
#define SCICFG_TEN 0x1
#define SCICFG_REN 0x2
#define SCICFG_TCLR 0x1000
#define SCICFG_RCLR 0x2000
#define SCICFG_BDR_OFFSET 2
#define SCICFG_TIRE_MASK 0x1c0
#define SCICFG_TIRE_OFFSET 6
#define SCICFG_RIRE_MASK 0xe00
#define SCICFG_RIRE_OFFSET 9

#define DPS_LSFLAGS 0x1fc
#define LSFLAGS_SCITIE 0x01
#define LSFLAGS_SCIRIE 0x02

static int uart;    // is there a uart?

volatile struct _dps_sci {
  volatile unsigned int txd;
  volatile unsigned int rxd;
  volatile unsigned int cfg;
} *dps_sci;

void
uartinit(void)
{
  char *p;
  uint cfg;

  dps_sci = (struct _dps_sci *)((char *)sriosr() + DPS_SCI);

  // 9600 baud
  cfg = 1 << SCICFG_BDR_OFFSET;
  cfg |= SCICFG_TEN | SCICFG_REN;

  // enable interrupts.
  cfg |= 1 << SCICFG_TIRE_OFFSET | 1 << SCICFG_RIRE_OFFSET;

  dps_sci->cfg = cfg;

  uart = 1;
  idtenable(IRQ_DPSLS);
  // ioapicenable(IRQ_COM1, 0);

  // Announce that we're here.
  for(p="xv6...\n"; *p; p++)
    uartputc(*p);
}

void
uartputc(int c)
{
  if(!uart)
    return;

  dps_sci->txd = (unsigned char)c;
}

static int
uartgetc(void)
{
  uint c;

  if(!uart)
    return -1;

  c = dps_sci->rxd;

  if(c & 0x80000000) {
    return (unsigned char)(c & 0xff);
  }

  return -1;
}

unsigned int
uarteoi(void)
{
  return *(volatile unsigned int *)((char *)sriosr() + DPS_LSFLAGS);
}

void
uartintr(void)
{
  consoleintr(uartgetc);
  uarteoi();
}
