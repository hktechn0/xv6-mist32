// Multiprocessor support
// Search memory for MP description structures.
// http://developer.intel.com/design/pentium/datashts/24201606.pdf

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mp.h"
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
