#include "types.h"
#include "stat.h"
#include "user.h"

void
test_basic(uint n)
{
  int **p;
  int i;

  p = malloc(n * sizeof(int *));

  // ALLOC
  printf(1, "Alloc %d\n", n);

  for(i = 0; i < n; i++) {
    p[i] = oalloc(1, 0xff);
  }

  printf(1, "OK\n");

  // WRITE
  printf(1, "Write %d\n", n);

  for(i = 0; i < n; i++) {
    *p[i] = i;
  }

  printf(1, "OK\n");

  // READ
  printf(1, "Read %d\n", n);

  for(i = 0; i < n; i++) {
    printf(1, "%d ", *p[i]);
  }

  printf(1, "OK\n");

  // FREE
  printf(1, "Free %d\n", n);

  for(i = 0; i < n; i++) {
    ofree(p[i]);
  }

  printf(1, "OK\n");

  free(p);
}

int
main(int argc, char *argv[])
{
  char c[100];

  gets(c, 100);
  test_basic(atoi(c));

  exit();
}
