#include <math.h>
#include <stdio.h>

int main() {
  int size = pow(10, 8);

  int as[size];
  for (size_t ix = 0; ix < size; ++ix)
    as[ix] = 0;

  printf("%d\n", as[0]);

  return 0;
}

/*

   The stack is a region of memory that grows downward from a higher memory
   address.

   It's typically used for local variables within functions. The stack has a
   limited size, often much smaller than the total system memory.

   In C, arrays are generally allocated on the stack when declared within a
   function.

   The code declares an array as with size = 1000000000 elements, which takes a
   significant amount of memory.

   The memory might not have enough space to hold such a large array, leading to
   a segmentation fault when the program tries to allocate the memory for the
   array.

   The stack memory is static in the sense that the size of the stack is fixed
   for a program execution and doesn't change during runtime.

*/
