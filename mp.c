// Multiprocessor support
// mist32 is not supported multiprocessor

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

struct cpu cpus[NCPU];
int ismp;
int ncpu;

void
mpinit(void)
{
  ismp = 0;
  ncpu = 1;
  lapic = 0;

  cpus[ncpu].id = ncpu;
  ncpu++;

  return;
}
