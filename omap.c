#include "types.h"
#include "stat.h"
#include "user.h"

// Xorshift
unsigned int rand(void)
{
  static unsigned int x=123456789, y=362436069, z=521288629, w=88675123;
  unsigned int t=(x^(x<<11));
  x=y; y=z; z=w;
  return ( w=(w^(w>>19))^(t^(t>>8)) );
}

void
test_seq(uint n)
{
  int **p;
  int i;

  p = malloc(n * sizeof(int *));

  // ALLOC
  printf(1, "Alloc %d\n", n);

  for(i = 0; i < n; i++) {
    p[i] = oalloc(1, 0x8ac);
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

void
test_rand(uint n)
{
  int **p;
  int i;

  p = malloc(n * sizeof(int *));

  // ALLOC
  printf(1, "Alloc %d\n", n);

  for(i = 0; i < n; i++) {
    // random size
    p[i] = oalloc(1, rand() % 0x1000 + 1);
    *p[i] = 0;
  }

  printf(1, "OK\n");

  // WRITE
  printf(1, "Write %d\n", n);

  for(i = 0; i < n; i++) {
    *p[rand() % n] = i;
  }

  printf(1, "OK\n");

  // READ
  printf(1, "Read %d\n", n);

  for(i = 0; i < n; i++) {
    printf(1, "%d ", *p[rand() % n]);
  }

  printf(1, "OK\n");

  // RANDOM READ WRITE
  printf(1, "ReadWrite %d\n", n);

  for(i = 0; i < n; i++) {
    if(rand() & 0x8) {
      printf(1, "R%d ", *p[rand() % n]);
    }
    else {
      printf(1, "W%d ", (*p[rand() % n] = n));
    }
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

/*
void
test_sort(uint n)
{
  uint **p[100];
  int i;

  p = malloc(n * sizeof(int *));

  // ALLOC
  printf(1, "Alloc %d\n", n);

  for(i = 0; i < n; i++) {
    p[i] = oalloc(1, sizeof(uint) * 100);
  }

  printf(1, "OK\n");

  // WRITE
  printf(1, "Write %d\n", n);

  for(i = 0; i < n; i++) {
    *p[i] = rand();
  }

  printf(1, "OK\n");

  // SORT
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
*/

int
main(int argc, char *argv[])
{
  char c[100];

  gets(c, 100);
  test_seq(atoi(c));
  test_rand(atoi(c));

  exit();
}
