#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "mist32.h"
#include "traps.h"
#include "spinlock.h"

// Interrupt vector struct
struct idt_entry {
  unsigned int flags;
  void (*handler)(void);
};

#define IDT_FLAGS_VALID 0x1
#define IDT_FLAGS_ENABLE 0x2

#define SETINTVEC(irq, func, valid, enable)             \
{                                                       \
  idt[irq].flags = ((valid) ? IDT_FLAGS_VALID : 0) |    \
                   ((enable) ? IDT_FLAGS_ENABLE : 0);   \
  idt[irq].handler = (void *)func;                      \
}

// Interrupt descriptor table (shared by all CPUs).
struct idt_entry idt[128];
uint idtset;
extern uint vectors[];  // in vectors.S: array of 128 entry pointers
struct spinlock tickslock;
uint ticks;

void
tvinit(void)
{
  int i;

  for(i = 0; i < 38; i++) {
    // DPS, GCI
    SETINTVEC(i, vectors[i], 1, idt[i].flags & IDT_FLAGS_ENABLE);
  }
  for(; i < 64; i++) {
    // Fault, Abort
    SETINTVEC(i, vectors[i], 1, 1);
  }

  SETINTVEC(T_SYSCALL, vectors[T_SYSCALL], 1, 1);

  initlock(&tickslock, "time");
}

void
idtinit(void)
{
  sridtw(v2p(idt));
  idtset = 1;
  idts();
}

void
idtenable(int irq)
{
  idt[irq].flags |= IDT_FLAGS_ENABLE;
  if(idtset)
    idts();
}

//PAGEBREAK: 41
void
trap(struct trapframe *tf)
{
  if(tf->trapno == T_SYSCALL){
    if(proc()->killed)
      exit();
    proc()->tf = tf;
    syscall();
    if(proc()->killed)
      exit();
    return;
  }

  switch(tf->trapno){
  case IRQ_TIMER:
    if(cpu()->id == 0){
      acquire(&tickslock);
      ticks++;
      wakeup(&ticks);
      release(&tickslock);
    }
    timereoi();
    break;
/*
  case T_IRQ0 + IRQ_IDE:
    ideintr();
    // lapiceoi();
    break;
  case T_IRQ0 + IRQ_IDE+1:
    // Bochs generates spurious IDE1 interrupts.
    break;
*/
  case IRQ_KMC:
    kbdintr();
    // lapiceoi();
    break;
  case IRQ_DPSLS:
    uartintr();
    // lapiceoi();
    break;
/*
  case T_IRQ0 + 7:
  case T_IRQ0 + IRQ_SPURIOUS:
    cprintf("cpu%d: spurious interrupt at %x\n",
            cpu()->id, tf->ppcr);
    // lapiceoi();
    break;
*/
   
  //PAGEBREAK: 13
  default:
    if(proc() == 0 || (tf->ppcr&3) == 0){
      // In kernel, it must be our mistake.
      cprintf("unexpected trap %d from cpu %d pc %x (fi0r=0x%x)\n",
              tf->trapno, cpu()->id, tf->ppcr, srfi0r());
      panic("trap");
    }
    // In user space, assume process misbehaved.
    cprintf("pid %d %s: trap %d on cpu %d "
            "pc 0x%x addr 0x%x--kill proc\n",
            proc()->pid, proc()->name, tf->trapno, cpu()->id, tf->ppcr, 
            srfi0r());
    proc()->killed = 1;
  }

  // Force process exit if it has been killed and is in user space.
  // (If it is still executing in the kernel, let it keep running 
  // until it gets to the regular system call return.)
  if(proc() && proc()->killed && (tf->ppsr & PSR_CMOD_USER))
    exit();

  // Force process to give up CPU on clock tick.
  // If interrupts were on while locks held, would need to check nlock.
  if(proc() && proc()->state == RUNNING && tf->trapno == IRQ_TIMER)
    yield();

  // Check if the process has been killed since we yielded
  if(proc() && proc()->killed && (tf->ppsr & PSR_CMOD_USER))
    exit();
}
