#include "types.h"
#include "stat.h"
#include "user.h"
#include "param.h"

void
ofree(void *p)
{
  ounmap(p);
}

void*
oalloc(uint n, uint nbytes)
{
  return (void *)omap(n, nbytes);
}
