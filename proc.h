// Per-CPU state
struct cpu {
  uchar id;                    // Local APIC ID; index into cpus[] below
  struct context *scheduler;   // swtch() here to enter scheduler
//  struct taskstate ts;         // Used by x86 to find stack for interrupt
//  struct segdesc gdt[NSEGS];   // x86 global descriptor table
  volatile uint started;       // Has the CPU started?
  int ncli;                    // Depth of pushcli nesting.
  int intena;                  // Were interrupts enabled before pushcli?
  
  // Cpu-local storage variables; see below
  struct cpu *cpu;
  struct proc *proc;           // The currently-running process.
};

extern struct cpu cpus[NCPU];
extern int ncpu;

// Per-CPU variables, holding pointers to the
// current cpu and to the current process.
static inline struct cpu* volatile cpu(void)
{
  struct cpu* volatile p;

  asm volatile("ld32 %0, rglobl" : "=r"(p));

  return p;
}

static inline void cpu_set(struct cpu* volatile p)
{
  asm volatile("st32 %0, rglobl" : : "r"(p) : "memory");
}

static inline struct proc* volatile proc(void)
{
  struct proc* volatile p;

  asm volatile("move %0, rglobl\n"
               "add %0, 4\n"
               "ld32 %0, %0" : "=r"(p));

  return p;
}

static inline void proc_set(struct proc* volatile p)
{
  void* volatile tmp;

  asm volatile("move %0, rglobl\n"
               "add %0, 4" : "=r"(tmp));
  asm volatile("st32 %1, %0" : : "r"(tmp), "r"(p) : "memory");
}

//PAGEBREAK: 17
// Saved registers for kernel context switches.
// Don't need to save all the segment registers (%cs, etc),
// because they are constant across kernel contexts.
// Don't need to save %eax, %ecx, %edx, because the
// x86 convention is that the caller has saved them.
// Contexts are stored at the bottom of the stack they
// describe; the stack pointer is the address of the context.
// The layout of the context matches the layout of the stack in swtch.S
// at the "Switch stacks" comment. Switch doesn't save eip explicitly,
// but it is on the stack and allocproc() manipulates it.
struct context {
  uint rret;
  uint rbase;
  // uint r29; // cpu-local pointer
  uint r28;
  uint r27;
  uint r26;
  uint r25;
  uint r24;
  uint r23;
  uint r22;
  uint r21;
  uint r20;
  uint r19;
  uint r18;
  uint r17;
  uint r16;
  uint r15;
  uint r14;
  uint r13;
  uint r12;
  uint r11;
  uint r10;
  uint r9;
  uint r8;
  uint return_to;
};

enum procstate { UNUSED, EMBRYO, SLEEPING, RUNNABLE, RUNNING, ZOMBIE };

// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
  char *kstack;                // Bottom of kernel stack for this process
  enum procstate state;        // Process state
  volatile int pid;            // Process ID
  struct proc *parent;         // Parent process
  struct trapframe *tf;        // Trap frame for current syscall
  struct context *context;     // swtch() here to run process
  void *chan;                  // If non-zero, sleeping on chan
  int killed;                  // If non-zero, have been killed
  struct file *ofile[NOFILE];  // Open files
  struct inode *cwd;           // Current directory
  char name[16];               // Process name (debugging)
};

// Process memory is laid out contiguously, low addresses first:
//   text
//   original data and bss
//   fixed-size stack
//   expandable heap
