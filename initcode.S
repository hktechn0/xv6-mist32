# Initial process execs /init.

#include "syscall.h"
#include "traps.h"


# exec(init, argv)
.globl start
start:
	lih	r1,	hi(init)
	wl16	r1,	lo(init)
	lih	r2,	hi(argv)
	wl16	r2,	lo(argv)
	lil	rret,	0	; where caller pc would be
	lil	r0,	SYS_exec
	swi	T_SYSCALL

# for(;;) exit();
exit:
	lil	r0,	SYS_exit
	swi	T_SYSCALL
	br	exit,	#al

# char init[] = "/init\0";
init:
  .string "/init\0"

# char *argv[] = { init, 0 };
.p2align 2
argv:
  .long init
  .long 0

