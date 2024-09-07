#include <stdio.h>
#include <stdlib.h>

int main() {
  int *as;
  as = (int *)malloc(10 * sizeof(int));
  int sum = 0;

  for (int ix = 0; ix < 10; ++ix)
    as[ix] = ix;

  for (int ix = 0; ix < 10; ++ix)
    sum += as[ix];

  printf("%d\n", sum);

  free(as);

  free(as);
  return 0;
}

/*
   When the initialization of 'as' is commented, valgrind will signal an uninitialized value of size 8, namely,
   the use of the pointer 'as' in the first iteration of the first for loop.
   Consequentially, there will be also an invalid write of size 4, namely, the int 'ix'.
   
   When the free(as) statement is commented, valgrind will signal that 40 bytes are lost. The 40 bytes are the ones 
   written in the first for loop.

   When at the end an additional free(as) is added, valgrind recognizes that the first block of memory was freed and a
   second invalid free was called.
   Valgrind brings up the memory addresses that point to the lines in the main function where the free(as) was called, 
   aswell as their line number in the code (line 17 and 19).
   

*/

