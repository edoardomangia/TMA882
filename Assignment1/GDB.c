#include <stdio.h>
#include <stdlib.h>

int main() {
  int *as = NULL;
	int sum = 0;

  for (int ix = 0; ix < 10; ++ix)
    as[ix] = ix;

  for (int ix = 0; ix < 10; ++ix)
    sum += as[ix];

  free(as);
}

/*
   When printing the last value of 'as' in the debugger, it returns $1 = (int*0 0x0). 
   
	 This is because the pointer 'as', when aborting, is pointing to the address 0x0 which is NULL.
   
	 This value was never changed during the code.
*/
