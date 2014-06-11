#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "defs.h"
#include "mist32.h"
#include "elf.h"

#define XV6_MRUBY_MRUBY
#define XV6_MRUBY_MIRB
#define XV6_MRUBY_MRBTEST

int execkb(char *path, char **argv);
#ifdef XV6_MRUBY_MRUBY
extern char _binary_mruby_start[];
#endif
#ifdef XV6_MRUBY_MIRB
extern char _binary_mirb_start[];
#endif
#ifdef XV6_MRUBY_MRBTEST
extern char _binary_mrbtest_start[];
#endif

int
exec(char *path, char **argv)
{
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[MAXARG+1];
  struct elfhdr elf;
  struct inode *ip;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;

  if((ip = namei(path)) == 0)
    return execkb(path, argv);
  ilock(ip);
  pgdir = 0;

  // Check ELF header
  if(readi(ip, (char*)&elf, 0, sizeof(elf)) < sizeof(elf))
    goto bad;
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    if(readi(ip, (char*)&ph, off, sizeof(ph)) != sizeof(ph))
      goto bad;
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(loaduvm(pgdir, (char*)ph.vaddr, ip, ph.off, ph.filesz) < 0)
      goto bad;
  }
  iunlockput(ip);
  ip = 0;

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[argc] = sp;
  }
  ustack[argc] = 0;

  proc()->tf->rret = 0xffffffff;  // fake return PC
  proc()->tf->r1 = argc;
  proc()->tf->r2 = sp - (argc+1)*4;  // argv pointer

  sp -= (argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(proc()->name, last, sizeof(proc()->name));

  // Commit to the user image.
  oldpgdir = proc()->pgdir;
  proc()->pgdir = pgdir;
  proc()->sz = sz;
  proc()->tf->ppcr = elf.entry;  // main
  proc()->tf->uspr = sp;
  switchuvm(proc());
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  if(ip)
    iunlockput(ip);
  return -1;
}

int
execkb(char *path, char **argv)
{
  char *bin;
  char *s, *last;
  int i, off;
  uint argc, sz, sp, ustack[MAXARG+1];
  struct elfhdr elf;
  struct proghdr ph;
  pde_t *pgdir, *oldpgdir;

  if(0)
    return -1;
#ifdef XV6_MRUBY_MRUBY
  else if(strncmp(path, "mruby", 6) == 0)
    bin = _binary_mruby_start;
#endif
#ifdef XV6_MRUBY_MIRB
  else if(strncmp(path, "mirb", 5) == 0)
    bin = _binary_mirb_start;
#endif
#ifdef XV6_MRUBY_MRBTEST
  else if(strncmp(path, "mrbtest", 8) == 0)
    bin = _binary_mrbtest_start;
#endif
  else
    return -1;

  pgdir = 0;

  // Check ELF header
  memmove((char *)&elf, bin, sizeof(elf));
  if(elf.magic != ELF_MAGIC)
    goto bad;

  if((pgdir = setupkvm()) == 0)
    goto bad;

  // Load program into memory.
  sz = 0;
  for(i=0, off=elf.phoff; i<elf.phnum; i++, off+=sizeof(ph)){
    memmove((char*)&ph, bin + off, sizeof(ph));
    if(ph.type != ELF_PROG_LOAD)
      continue;
    if(ph.memsz < ph.filesz)
      goto bad;
    if((sz = allocuvm(pgdir, sz, ph.vaddr + ph.memsz)) == 0)
      goto bad;
    if(copyout(pgdir, ph.vaddr, bin + ph.off, ph.filesz) < 0)
      goto bad;
  }

  // Allocate two pages at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  sz = PGROUNDUP(sz);
  if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
    goto bad;
  clearpteu(pgdir, (char*)(sz - 2*PGSIZE));

  // more stack
  if((sz = allocuvm(pgdir, sz, sz + 10*PGSIZE)) == 0)
    goto bad;

  sp = sz;

  // Push argument strings, prepare rest of stack in ustack.
  for(argc = 0; argv[argc]; argc++) {
    if(argc >= MAXARG)
      goto bad;
    sp = (sp - (strlen(argv[argc]) + 1)) & ~3;
    if(copyout(pgdir, sp, argv[argc], strlen(argv[argc]) + 1) < 0)
      goto bad;
    ustack[argc] = sp;
  }
  ustack[argc] = 0;

  proc()->tf->rret = 0xffffffff;  // fake return PC
  proc()->tf->r1 = argc;
  proc()->tf->r2 = sp - (argc+1)*4;  // argv pointer

  sp -= (argc+1) * 4;
  if(copyout(pgdir, sp, ustack, (argc+1)*4) < 0)
    goto bad;

  // Save program name for debugging.
  for(last=s=path; *s; s++)
    if(*s == '/')
      last = s+1;
  safestrcpy(proc()->name, last, sizeof(proc()->name));

  // Commit to the user image.
  oldpgdir = proc()->pgdir;
  proc()->pgdir = pgdir;
  proc()->sz = sz;
  proc()->tf->ppcr = elf.entry;  // main
  proc()->tf->uspr = sp;
  switchuvm(proc());
  freevm(oldpgdir);
  return 0;

 bad:
  if(pgdir)
    freevm(pgdir);
  return -1;
}
