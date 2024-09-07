#include <math.h>
#include <stdio.h>
#include <stdlib.h>

int main() {
  int size = pow(10, 8);

  int *as = (int *)malloc(sizeof(int) * size); // Allocating memory on the heap 

  for (size_t ix = 0; ix < size; ++ix) 
    as[ix] = 0;

  printf("%d\n", as[0]);

  free(as);
}

/*
   The heap is another region of memory that grows upward from a lower memory address. 

   It's used for dynamic memory allocation using functions like malloc and calloc.
   
   The heap is managed manually, by explicitly allocating memory using "malloc" and by freeing it using "free" to avoid memory leaks.
  
   The heap has a much larger capacity than the stack, often limited only by the total system memory available.
   
	 For this reason, allocating a 10^8 bytes of memory doesn't trigger a segmentation fault, unlike the example in stack_allocation.c.
*/
