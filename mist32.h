// Routines to let C code use special x86 instructions.

static inline void*
sriosr(void)
{
  void *r;
  asm("sriosr %0" : "=r"(r));
  return r;
}

static inline uint
srieir(void)
{
  uint intena;
  asm volatile("srieir %0" : "=r"(intena));
  return intena;
}

static inline void
srieiw_enable(void)
{
  asm volatile("srieiw 1");
}

static inline void
srieiw_disable(void)
{
  asm volatile("srieiw 0");
}

static inline void*
srpdtr(void)
{
  void *r;
  asm volatile("srpdtr %0" : "=r"(r));
  return r;
}

static inline void
srpdtw(uint val)
{
  asm volatile("srkpdtw %0" : : "r"(val));
  asm volatile("srpdtw %0" : : "r"(val));
}

static inline void
srppdtw(uint val)
{
  asm volatile("srppdtw %0" : : "r"(val));
}

static inline void
srppcw(uint val)
{
  asm volatile("srppcw %0" : "=r"(val));
}

static inline void
sridtw(uint idt)
{
  asm volatile("sridtw %0" : : "r"(idt));
}

static inline void
idts(void)
{
  asm volatile("idts");
}

static inline uint
srfi0r(void)
{
  uint r;
  asm("srfi0r %0" : "=r"(r));
  return r;
}

static inline uint
srfi1r(void)
{
  uint r;
  asm("srfi1r %0" : "=r"(r));
  return r;
}

static inline uint
tas(volatile uint *addr)
{
  uint test;

  asm volatile("tas %0, %1" :
               "=r"(test) : "r"(addr) : "memory");

  return test;
}

//PAGEBREAK: 36
// Layout of the trap frame built on the stack by the
// hardware and by trapasm.S, and passed to trap().
struct trapframe {
  // saved by hardware on interrupt
  uint ptidr;
//  uint ppdtr;
  uint ppsr;
  uint ppcr;
  uint pflagr;

  uint uspr;
  uint rret;
  uint rbase;

  // uint r29;
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

  // caller-saved registers
  uint rtmp;
  uint r6;
  uint r5;
  uint r4;
  uint r3;
  uint r2;
  uint r1;
  uint r0;

  uint trapno;
//  uint err;
};
