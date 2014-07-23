#include "memlayout.h"
#include "mmu.h"
#include "param.h"

.p2align 2
.text

;; By convention, the _start symbol specifies the ELF entry point.
;; Since we haven't set up virtual memory yet, our entry point is
;; the physical address of 'entry'.
.globl _start
_start = V2P_WO(entry)

;; Entering xv6 on boot processor, with paging off.
.globl entry
entry:
	;; Set up the stack pointer.
	lih	rtmp, hi(stack + KSTACKSIZE)
	wl16	rtmp, lo(stack + KSTACKSIZE)
	srspw	rtmp

	;; 4Kbyte page mode
	lil	rtmp, 0x00000080
	srpsw	rtmp
	;; Set page directory
	lih	rtmp, hi(V2P_WO(entrypgdir))
	wl16	rtmp, lo(V2P_WO(entrypgdir))
	srkpdtw	rtmp
	srpdtw	rtmp
	;; Turn on paging.
	srmmuw	2
	
	;; Jump to main(), and switch to executing at
	;; high addresses.
	lih	rtmp, hi(main)
	wl16	rtmp, lo(main)
	b	rtmp, #al

.comm stack, KSTACKSIZE
