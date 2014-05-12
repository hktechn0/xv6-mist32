OBJS = \
	bio.o\
	console.o\
	exec.o\
	file.o\
	fs.o\
	gci.o\
	ide.o\
	kalloc.o\
	kbd.o\
	lapic.o\
	log.o\
	main.o\
	mp.o\
	pipe.o\
	proc.o\
	spinlock.o\
	string.o\
	swtch.o\
	syscall.o\
	sysfile.o\
	sysproc.o\
	timer.o\
	trapasm.o\
	trap.o\
	uart.o\
	vectors.o\
	vm.o\


MRUBY = mruby mirb mrbtest
MRUBY_BENCH = fib39.rb bm_so_lists.rb ao-render.rb

# Cross-compiling
TOOLPREFIX = mist32-elf-

# Using native tools
#TOOLPREFIX = 

CC = $(TOOLPREFIX)gcc
AS = $(TOOLPREFIX)gas
LD = $(TOOLPREFIX)ld
OBJCOPY = $(TOOLPREFIX)objcopy
OBJDUMP = $(TOOLPREFIX)objdump
CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -O -Wall -MD -ggdb -Werror -fno-omit-frame-pointer
#CFLAGS = -fno-pic -static -fno-builtin -fno-strict-aliasing -Wall -MD -ggdb -Werror -fno-omit-frame-pointer
CFLAGS += $(shell $(CC) -fno-stack-protector -E -x c /dev/null >/dev/null 2>&1 && echo -fno-stack-protector)
ASFLAGS = -gdwarf-2

all: xv6.bin xv6memfs.bin

xv6.bin: kernel fs.img
	$(OBJCOPY) -O binary kernel xv6.bin

xv6memfs.bin: kernelmemfs
	$(OBJCOPY) -O binary kernelmemfs xv6memfs.bin

#xv6.img: bootblock kernel fs.img
#	dd if=/dev/zero of=xv6.img count=10000
#	dd if=bootblock of=xv6.img conv=notrunc
#	dd if=kernel of=xv6.img seek=1 conv=notrunc

#xv6memfs.img: bootblock kernelmemfs
#	dd if=/dev/zero of=xv6memfs.img count=10000
#	dd if=bootblock of=xv6memfs.img conv=notrunc
#	dd if=kernelmemfs of=xv6memfs.img seek=1 conv=notrunc

#bootblock: bootasm.S bootmain.c
#	$(CC) $(CFLAGS) -fno-pic -O -nostdinc -I. -c bootmain.c
#	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c bootasm.S
#	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7C00 -o bootblock.o bootasm.o bootmain.o
#	$(OBJDUMP) -S bootblock.o > bootblock.asm
#	$(OBJCOPY) -S -O binary -j .text bootblock.o bootblock
#	./sign.pl bootblock

#entryother: entryother.S
#	$(CC) $(CFLAGS) -fno-pic -nostdinc -I. -c entryother.S
#	$(LD) $(LDFLAGS) -N -e start -Ttext 0x7000 -o bootblockother.o entryother.o
#	$(OBJCOPY) -S -O binary -j .text bootblockother.o entryother
#	$(OBJDUMP) -S bootblockother.o > entryother.asm

initcode: initcode.S
	$(CC) $(CFLAGS) -nostdinc -I. -c initcode.S
	$(LD) $(LDFLAGS) -N -e start -Ttext 0 -o initcode.out initcode.o
	$(OBJCOPY) -S -O binary initcode.out initcode
	$(OBJDUMP) -S initcode.o > initcode.asm

kernel: $(OBJS) entry.o initcode kernel.ld $(MRUBY)
	$(LD) $(LDFLAGS) -T kernel.ld -o kernel entry.o $(OBJS) -b binary initcode $(LDMRUBY)
	$(OBJDUMP) -S kernel > kernel.asm
	$(OBJDUMP) -t kernel | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernel.sym

# kernelmemfs is a copy of kernel that maintains the
# disk image in memory instead of writing to a disk.
# This is not so useful for testing persistent storage or
# exploring disk buffering implementations, but it is
# great for testing the kernel on real hardware without
# needing a scratch disk.
MEMFSOBJS = $(filter-out ide.o,$(OBJS)) memide.o
kernelmemfs: $(MEMFSOBJS) entry.o initcode kernel.ld fs.img $(MRUBY)
	$(LD) $(LDFLAGS) -T kernel.ld -o kernelmemfs entry.o  $(MEMFSOBJS) -b binary initcode $(LDMRUBY) fs.img
	$(OBJDUMP) -S kernelmemfs > kernelmemfs.asm
	$(OBJDUMP) -t kernelmemfs | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > kernelmemfs.sym

tags: $(OBJS) entryother.S _init
	etags *.S *.c

vectors.S: vectors.pl
	perl vectors.pl > vectors.S

ULIB = ulib.o usys.o printf.o umalloc.o

_%: %.o $(ULIB)
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o $@ $^
	$(OBJDUMP) -S $@ > $*.asm
	$(OBJDUMP) -t $@ | sed '1,/SYMBOL TABLE/d; s/ .* / /; /^$$/d' > $*.sym

_forktest: forktest.o $(ULIB)
	# forktest has less library code linked in - needs to be small
	# in order to be able to max out the proc table.
	$(LD) $(LDFLAGS) -N -e main -Ttext 0 -o _forktest forktest.o ulib.o usys.o
	$(OBJDUMP) -S _forktest > forktest.asm

# mruby
LDMRUBY = -b binary mruby -b binary mirb -b binary mrbtest
LDMRUBY = -b binary mruby
mruby:
	cp ../mruby/build/mist32-xv6/bin/mruby .
mirb:
	cp ../mruby/build/mist32-xv6/bin/mirb .
mrbtest:
	cp ../mruby/build/mist32-xv6/test/mrbtest .

mkfs: mkfs.c fs.h
	gcc -Werror -Wall -o mkfs mkfs.c

# Prevent deletion of intermediate files, e.g. cat.o, after first build, so
# that disk image changes after first build are persistent until clean.  More
# details:
# http://www.gnu.org/software/make/manual/html_node/Chained-Rules.html
.PRECIOUS: %.o

UPROGS=\
	_cat\
	_echo\
	_forktest\
	_grep\
	_init\
	_kill\
	_ln\
	_ls\
	_mkdir\
	_rm\
	_sh\
	_stressfs\
	_usertests\
	_wc\
	_zombie\

fs.img: mkfs README $(UPROGS) $(MRUBY_BENCH)
	./mkfs fs.img README $(UPROGS) $(MRUBY_BENCH)

-include *.d

clean: 
	rm -f *.tex *.dvi *.idx *.aux *.log *.ind *.ilg *.bin \
	*.o *.d *.asm *.sym vectors.S bootblock entryother \
	initcode initcode.out kernel xv6.img fs.img kernelmemfs mkfs \
	.gdbinit \
	$(UPROGS) $(MRUBY)
