# Context switch
#
#   void swtch(struct context **old, struct context *new);
# 
# Save current register context in old
# and then load register context from new.

.globl swtch
swtch:
	;; Save old callee-save registers
	push	rret
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
	// push	rglobl		; cpu-local pointer
	push	rbase
	push	rret

	;; Switch stacks
	srspr	rtmp
	st32	rtmp,	r1
	srspw	r2

	;; Load new callee-save registers
	pop	rret
	pop	rbase
	// pop	rglobl		; cpu-local pointer
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

	pop	r0		; context which returns to

	b	r0, #al
