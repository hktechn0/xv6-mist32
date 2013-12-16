#include "mmu.h"

;; vectors.S sends all traps here.
.globl alltraps
alltraps:
	;; Build trap frame.
	push	r0
	push	r1
	push	r2
	push	r3
	push	r4
	push	r5
	push	r6
	push	rtmp

	push	r8
	push	r9
	push	r10
	push	r11
	push 	r12
	push	r13
	push	r14
	push	r15
	push	r16
	push	r17
	push	r18
	push	r19
	push	r20
	push	r21
	push	r22
	push	r23
	push	r24
	push	r25
	push	r26
	push	r27
	push	r28

	push	rbase
	push	rret
	sruspr	rtmp
	push	rtmp

	;; Save previous registers.
	srpflagr rtmp
	push	rtmp
	srppcr	rtmp
	push	rtmp
	srppsr	rtmp
	push	rtmp
//	srppdtr	rtmp
//	push	rtmp
	srptidr	rtmp
	push	rtmp

	;; args[0] <= trapframe
	srspr	r1

	;; return to trapret()
	lih	rret, hi(trapret)
	wl16	rret, lo(trapret)

	;; Call trap(tf), where tf=%sp
	lih	rtmp, hi(trap)
	wl16	rtmp, lo(trap)
	b	rtmp, #al

;; Return falls through to trapret... 
.globl trapret
trapret:
	pop	rtmp
	srptidw	rtmp
//	pop	rtmp
//	srppdtw	rtmp
	pop	rtmp
	srppsw	rtmp
	pop	rtmp
	srppcw	rtmp
	pop	rtmp
	srpflagw rtmp

	pop	rtmp		; user stack pointer
	sruspw	rtmp
	pop	rret		; return pointer
	pop	rbase
	
	pop	r28
	pop	r27
	pop	r26
	pop	r25
	pop	r24
	pop	r23
	pop	r22
	pop	r21
	pop	r20
	pop	r19
	pop	r18
	pop	r17
	pop	r16
	pop	r15
	pop	r14
	pop	r13
	pop	r12
	pop	r11
	pop	r10
	pop	r9
	pop	r8

	pop	rtmp
	pop	r6
	pop	r5
	pop	r4
	pop	r3
	pop	r2
	pop	r1
	pop	r0

	srspadd	4		; wipe trapno

	ib
