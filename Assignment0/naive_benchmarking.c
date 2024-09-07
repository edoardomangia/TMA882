#include <stdio.h>
#include <time.h>

// Function to compute the sum of the first billion integers
unsigned long long sum_first_billion() {
  unsigned long long sum = 0;
  for (unsigned long long ix = 1; ix <= 1000000000; ++ix) {
    sum += ix;
  }
  return sum;
}

int main() {
  // Get the start time
  clock_t start = clock();

  // Perform the sum
  unsigned long long result = sum_first_billion();

  // Get the end time
  clock_t end = clock();

  // Calculate the time taken
  double time_taken = (double)(end - start) / CLOCKS_PER_SEC;

  // Output the result and time taken
  printf("Sum: %llu\n", result);
  printf("Time taken: %f seconds\n", time_taken);

  return 0;
}

/*
   -O0 (No optimization):
   This will likely be the slowest version because the compiler does not optimize the code at all. It will generate a straightforward translation of the C code into machine code.

	 -O1 (Optimize):
   This will be faster than -O0. The compiler performs optimizations that do not require a trade-off between compilation time and runtime performance.
   
	 -O2 (Optimize more):
   This level performs nearly all supported optimizations that do not involve a space-speed tradeoff. It should be faster than -O1.
   
	 -O3 (Optimize fully):
   This performs all optimizations, including those that might increase the size of the code. It should generally be the fastest.
   
	 -Os (Optimize for size):
   This optimization focuses on reducing the size of the compiled code. It may not be as fast as -O3 but should be faster than -O0.
   
	 -Og (Optimize for debugging):
   This provides optimizations that do not interfere with debugging. It is a balance between -O0 and -O1, focusing on maintaining the debugging experience.
*/
