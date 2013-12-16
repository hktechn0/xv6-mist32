// mist32 trap and interrupt constants.

// Processor-defined:
#define T_PGFLT         40      // page fault
#define T_GPFLT         41      // general protection fault
#define T_ILLOP         42      // illegal opcode
#define T_VECNP         43      // interrupt vector not present

#define T_TSS           50      // invalid task state structure
#define T_DIVIDE        51      // divide error

#define T_DBLFLT        63      // double fault

// These are arbitrarily chosen, but with care not to overlap
// processor defined exceptions or interrupt vectors.
#define T_SYSCALL       64      // system call
#define T_DEFAULT      500      // catchall

#define IRQ_GCI0         4      // GCI IRQ 0 [4-35]
#define IRQ_KMC          5
#define IRQ_DISP         6
#define IRQ_MMCC         7

#define IRQ_TIMER       36      // DPS UTIM64 Timer
#define IRQ_DPSLS       37      // DPS LSFLAGS
