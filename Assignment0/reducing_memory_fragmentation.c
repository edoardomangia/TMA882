#include <stdio.h>
#include <stdlib.h>

void allocatingContiguousMemory();
void allocatingFragmentedMemory();

int main() {
	allocatingContiguousMemory();
	allocatingFragmentedMemory();

	return 0;
}


void allocatingContiguousMemory() {
	int size = 10;
	
	// Allocates a contiguous block of memory on the heap for the 'asentries' array of integers.
	// 'asentries' will be a 2 dimensional array of size 'size*size' bytes.
	int *asentries = (int*) malloc(sizeof(int) * size*size);
	
	// Allocates memory on the heap for the 'as' array of pointers.
	// 'as' will be of size 'size'.
	int **as = (int**) malloc(sizeof(int*) * size); 
	
	// Assigns each of the 'size' pointers in 'as', to the start of a row in the 'asentries' array.
	// There will be 'size' rows and 'size' columns, identified respectively by the ix-ths and jx-ths indices.
	for ( size_t ix = 0, jx = 0; ix < size; ++ix, jx+=size )
		  as[ix] = asentries + jx;
	
	// Initializes each element of the 'asentries' array to 0, by looping through the indices.
	for ( size_t ix = 0; ix < size; ++ix )
		  for ( size_t jx = 0; jx < size; ++jx )
			      as[ix][jx] = 0;

	printf("%d\n", as[0][0]);

	free(as);
	free(asentries);
}


void allocatingFragmentedMemory() { 
	int size = 10;
	
	// Allocates memory for the 'as' array of pointers.  
	int **as = (int**) malloc(sizeof(int*) * size);
	
	// Allocates memory for the rows to which each pointer of 'as' points to.
	for ( size_t ix = 0; ix < size; ++ix )
		as[ix] = (int*) malloc(sizeof(int) * size);
	
	// Initialize the elements of the 'as' array to 0.
	for ( size_t ix = 0; ix < size; ++ix )
		for ( size_t jx = 0; jx < size; ++jx )
			as[ix][jx] = 0;

	printf("%d\n", as[0][0]);
	
	// Frees the memory that was previously allocated with the first for loop, individually, for each row. 
	for ( size_t ix = 0; ix < size; ++ix )
		free(as[ix]);
	
	// Frees the memory allocated previously for the 'as' array of pointers.
	free(as);
}

/*
   In the first function, the memory for the whole 2 dimensional array is allocated through a single call.
   Allocating memory in this way, ensures that the memory will be in the form of a contiguous block.

   In the second function, instead, the memory is allocated individually for each row with a loop.
   This method of allocating memory, doesn't guarantee that the memory allocated will be a contiguous block.
*/
