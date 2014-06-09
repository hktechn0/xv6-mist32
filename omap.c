#include "types.h"
#include "stat.h"
#include "user.h"

#define OBJ_SIZE_MAX 4096

static unsigned int rand_x, rand_y, rand_z, rand_w;

// Xorshift
static inline void srand(void)
{
  rand_x=123456789; rand_y=362436069; rand_z=521288629; rand_w=88675123;
}

static inline unsigned int rand(void)
{
  const unsigned int t=(rand_x^(rand_x<<11));
  rand_x=rand_y; rand_y=rand_z; rand_z=rand_w;
  return ( rand_w=(rand_w^(rand_w>>19))^(t^(t>>8)) );
}

void
test_seq_512(uint n)
{
  int **p;
  unsigned int i, j, test_data;
  const unsigned int size = 512;
  unsigned int buffer[size >> 2];

  p = malloc(n * sizeof(int *));

  // ALLOC
  printf(1, "Alloc %d\n", n);
  for(i = 0; i < n; i++) {
    p[i] = oalloc(1, size);
  }
  printf(1, "OK\n");

  // WRITE
  printf(1, "Write %d\n", n);
  srand();

  for(i = 0; i < n; i++) {
    for(j = 0; j < size >> 2; j++) {
      buffer[j] = rand();
    }
    memmove(p[i], buffer, size);
  }
  printf(1, "OK\n");

  // READ
  printf(1, "Read %d\n", n);
  srand();

  for(i = 0; i < n; i++) {
    memmove(buffer, p[i], size);
    for(j = 0; j < size >> 2; j++) {
      test_data = rand();
      if(buffer[j] != test_data) {
        printf(1, "[INVALID] data[%d][%d] %x != %x\n", i, j, buffer[j], test_data);
      }
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

void
test_seq_randsize(uint n)
{
  int **p;
  unsigned int i, j, test_data;
  unsigned int buffer[OBJ_SIZE_MAX];
  unsigned int *obj_size;

  p = malloc(n * sizeof(int *));
  obj_size = malloc(n * sizeof(unsigned int));

  // ALLOC
  printf(1, "Alloc %d\n", n);
  srand();

  for(i = 0; i < n; i++) {
    obj_size[i] = rand() % OBJ_SIZE_MAX;
    p[i] = oalloc(1, obj_size[i]);
  }
  printf(1, "OK\n");

  // WRITE
  printf(1, "Write %d\n", n);
  srand();

  for(i = 0; i < n; i++) {
    for(j = 0; j < obj_size[i] >> 2; j++) {
      buffer[j] = rand();
    }
    memmove(p[i], buffer, obj_size[i]);
  }
  printf(1, "OK\n");

  // READ
  printf(1, "Read %d\n", n);
  srand();

  for(i = 0; i < n; i++) {
    memmove(buffer, p[i], obj_size[i]);
    for(j = 0; j < obj_size[i] >> 2; j++) {
      test_data = rand();
      if(buffer[j] != test_data) {
        printf(1, "[INVALID] data[%d][%d] %x != %x\n", i, j, buffer[j], test_data);
      }
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

void
test_rand_randsize(uint n)
{
  int **p;
  unsigned int i, j, index, test_data;
  unsigned int buffer[OBJ_SIZE_MAX];
  unsigned int *obj_size;
  unsigned char *obj_accessed;

  p = malloc(n * sizeof(int *));
  obj_size = malloc(n * sizeof(unsigned int));
  obj_accessed = malloc(n * sizeof(char));

  // ALLOC
  printf(1, "Alloc %d\n", n);
  srand();

  for(i = 0; i < n; i++) {
    obj_size[i] = rand() % OBJ_SIZE_MAX;
    obj_accessed[i] = 0;
    p[i] = oalloc(1, obj_size[i]);
  }
  printf(1, "OK\n");

  // WRITE
  printf(1, "Write %d\n", n);
  srand();

  for(i = 0; i < n; i++) {
    index = rand() % n;
    if(obj_accessed[index]) {
      i--;
      continue;
    }
    obj_accessed[index] |= 1;
    printf(1, "%d ", index);

    for(j = 0; j < obj_size[index] >> 2; j++) {
      buffer[j] = rand();
    }
    memmove(p[index], buffer, obj_size[index]);
  }
  printf(1, "OK\n");

  // READ
  printf(1, "Read %d\n", n);
  srand();

  for(i = 0; i < n; i++) {
    index = rand() % n;
    if(obj_accessed[index] & 2) {
      i--;
      continue;
    }
    obj_accessed[index] |= 2;
    printf(1, "%d ", index);

    memmove(buffer, p[index], obj_size[index]);
    for(j = 0; j < obj_size[index] >> 2; j++) {
      test_data = rand();
      if(buffer[j] != test_data) {
        printf(1, "[INVALID] data[%d][%d] %x != %x\n", index, j, buffer[j], test_data);
      }
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

void
test_randrw_randsize(uint n, uint times, uint write_rate)
{
  int **p;
  unsigned int i, j, index, is_write;
  unsigned int buffer[OBJ_SIZE_MAX];
  unsigned int *obj_size;
  unsigned char *obj_accessed;

  p = malloc(n * sizeof(int *));
  obj_size = malloc(n * sizeof(unsigned int));
  obj_accessed = malloc(n * sizeof(char));

  // ALLOC
  printf(1, "Alloc %d\n", n);
  srand();

  for(i = 0; i < n; i++) {
    obj_size[i] = rand() % OBJ_SIZE_MAX;
    obj_accessed[i] = 0;
    p[i] = oalloc(1, obj_size[i]);
  }
  printf(1, "OK\n");

  // WRITE
  printf(1, "Initialize Write %d\n", n);

  for(i = 0; i < n; i++) {
    index = rand() % n;
    if(obj_accessed[index]) {
      i--;
      continue;
    }
    obj_accessed[index] |= 1;
    printf(1, "%d ", index);

    for(j = 0; j < obj_size[index] >> 2; j++) {
      buffer[j] = index;
    }
    memmove(p[index], buffer, obj_size[index]);
  }
  printf(1, "OK\n");

  // READ & WRITE
  printf(1, "ReadWrite %d\n", n);

  for(i = 0; i < n * times; i++) {
    index = (rand() >> 16) % n;
    is_write = !!((rand() % 100) < write_rate);

    printf(1, "%c%d ", is_write ? 'W' : 'R', index);

    if(obj_size[index] < 8) {
      continue;
    }

    if(is_write) {
      p[index][0] = index;
    }

    if(p[index][1] != index) {
      printf(1, "[INVALID] data[%d] %x != %x\n", index, p[index][1], index);
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

#define NULL ((void *)0)

void
test_randrwfree_randsize(uint n, uint times, uint write_rate, uint free_rate)
{
  int **p;
  unsigned int i, index, is_write, test_data;
  unsigned int *obj_size;
  unsigned int *backup;

  p = malloc(n * sizeof(int *));
  obj_size = malloc(n * sizeof(unsigned int));
  backup = malloc(n * sizeof(unsigned int));

  // INIT
  printf(1, "Alloc %d\n", n);
  srand();

  for(i = 0; i < n; i++) {
    obj_size[i] = rand() % OBJ_SIZE_MAX;

    if(obj_size[i] < 4) {
      i--;
      continue;
    }

    p[i] = NULL;
  }
  printf(1, "OK\n");

  // TEST
  printf(1, "TEST %d\n", n);

  for(i = 0; i < n * times; i++) {
    index = rand() % n;

    if(p[index] == NULL) {
      p[index] = oalloc(1, obj_size[index]);

      test_data = rand();
      *p[index] = test_data;
      backup[index] = test_data;

      printf(1, "*");
    }

    if((rand() % 100) < free_rate) {
      ofree(p[index]);
      p[index] = NULL;

      printf(1, "F%d ", index);
      continue;
    }

    is_write = !!((rand() % 100) < write_rate);
    printf(1, "%c%d ", is_write ? 'W' : 'R', index);

    if(*p[index] != backup[index]) {
      printf(1, "[INVALID] data[%d] %x != %x\n", index, *p[index], backup[index]);
    }

    if(is_write) {
      test_data = rand();
      *p[index] = test_data;
      backup[index] = test_data;
    }
  }
  printf(1, "OK\n");

  // FREE
  printf(1, "Free %d\n", n);
  for(i = 0; i < n; i++) {
    if(p[i] != NULL)
      ofree(p[i]);
  }
  printf(1, "OK\n");

  free(p);
}

int
main(int argc, char *argv[])
{
  char c[100];
  unsigned int n;

  gets(c, 100);
  n = atoi(c);

  //test_seq_512(n);
  //test_seq_randsize(n);
  //test_rand_randsize(n);
  //test_randrw_randsize(n, 100, 30);
  test_randrwfree_randsize(n, 100, 30, 10);

  exit();
}
