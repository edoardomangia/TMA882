#include <stdio.h>
#include <stdlib.h>

int main() {

  int size = 10;

  int choiceAllocationMethod;
  int choiceFileFormat;

  // Asks the user whether the matrix should be allocated in a single block of
  // memory or multiple fragmented ones.
  printf("Choose the allocation method:\n");
  printf("1. Contiguous Memory Allocation\n");
  printf("2. Segmented Memory Allocation\n");
  printf("Enter your choice (1 or 2): ");
  scanf("%d", &choiceAllocationMethod);

  int **as = NULL;
  int *asentries = NULL;

  if (choiceAllocationMethod == 1) {

    // Creates the matrix, allocating contiguous memory.
    asentries = (int *)malloc(sizeof(int) * size * size);
    if (asentries == NULL) {
      printf("Memory allocation failed for asentries.\n");
      return 1;
    }

    as = (int **)malloc(sizeof(int *) * size);
    if (as == NULL) {
      printf("Memory allocation failed for as.\n");
      free(asentries);  // Free the previous allocation
      return 1;
    }

    for (size_t ix = 0, jx = 0; ix < size; ++ix, jx += size)
      as[ix] = asentries + jx;

    // Initializes the values of the matrix to be the product of the ix-th row
    // and jx-th column.
    for (size_t ix = 0; ix < size; ++ix)
      for (size_t jx = 0; jx < size; ++jx)
        as[ix][jx] = ix * jx;

  } else if (choiceAllocationMethod == 2) {

    // Creates the matrix, allocating fragmented memory.
    as = (int **)malloc(sizeof(int *) * size);
    if (as == NULL) {
      printf("Memory allocation failed for as.\n");
      return 1;
    }

    for (size_t ix = 0; ix < size; ++ix) {
      as[ix] = (int *)malloc(sizeof(int) * size);
      if (as[ix] == NULL) {
        printf("Memory allocation failed for row %zu.\n", ix);
        // Free previously allocated rows
        for (size_t j = 0; j < ix; ++j)
          free(as[j]);
        free(as);
        return 1;
      }
    }

    for (size_t ix = 0; ix < size; ++ix)
      for (size_t jx = 0; jx < size; ++jx)
        as[ix][jx] = ix * jx;

  } else {
    printf("Invalid choice.\n");
    return 1;
  }

  // Prints the matrix in the terminal.
  printf("The matrix that will be written will have this aspect: \n");

  for (size_t ix = 0; ix < size; ++ix) {
    for (size_t jx = 0; jx < size; ++jx) {
      printf("%d ", as[ix][jx]);
    }
    printf("\n");
  }

  // Asks the user if he wants to write in the .dat or .txt file.
  printf("Choose the file format:\n");
  printf("1. Binary format (.dat)\n");
  printf("2. Text format (.txt)\n");
  printf("Enter your choice (1 or 2): ");
  scanf("%d", &choiceFileFormat);

  if (choiceFileFormat == 1) { // Opens the matrix.dat in writing binary mode.

    FILE *file = fopen("matrix.dat", "wb");

    if (file == NULL) {
      printf("There was an error opening the file.\n");
      return -1;
    }

    // Writes the matrix in the matrix.dat file.
    if (choiceAllocationMethod ==  1) { // If it was allocated as contiguous memory
      fwrite(asentries, sizeof(int), size * size, file);
    } else if (choiceAllocationMethod == 2) { // If it was allocated as fragmented memory.
      for (size_t ix = 0; ix < size; ++ix)
        fwrite(as[ix], sizeof(int), size, file);
    }

    fclose(file);

  } else if (choiceFileFormat == 2) { // Opens the matrix.txt in writing mode

    FILE *file = fopen("matrix.txt", "w");

    if (file == NULL) {
      printf("There was an error opening the file.\n");
      return -1;
    }

    // Writes the matrix in the matrix.txt file.
    for (size_t ix = 0; ix < size; ++ix) {
      for (size_t jx = 0; jx < size; ++jx) {
        fprintf(file, "%d ", as[ix][jx]);
      }
      fprintf(file, "\n");
    }

    fclose(file);

  } else {
    printf("Invalid choice.\n");
    return 1; 
  }

  // Frees the allocated memory before re-reading the file.
  if (choiceAllocationMethod == 1) { // If it was allocated as contiguous memory
    free(as);
    free(asentries);
  } else if (choiceAllocationMethod == 2) { // If it was allocated as fragmented memory
    for (size_t ix = 0; ix < size; ++ix)
      free(as[ix]);
    free(as);
  }

  // Reallocate memory before reading the file
  if (choiceAllocationMethod == 1) {
    asentries = (int *)malloc(sizeof(int) * size * size);
    if (asentries == NULL) {
      printf("Memory reallocation failed for asentries.\n");
      return 1;
    }

    as = (int **)malloc(sizeof(int *) * size);
    if (as == NULL) {
      printf("Memory reallocation failed for as.\n");
      free(asentries);
      return 1;
    }

    for (size_t ix = 0, jx = 0; ix < size; ++ix, jx += size)
      as[ix] = asentries + jx;

  } else if (choiceAllocationMethod == 2) {
    as = (int **)malloc(sizeof(int *) * size);
    if (as == NULL) {
      printf("Memory reallocation failed for as.\n");
      return 1;
    }

    for (size_t ix = 0; ix < size; ++ix) {
      as[ix] = (int *)malloc(sizeof(int) * size);
      if (as[ix] == NULL) {
        printf("Memory reallocation failed for row %zu.\n", ix);
        for (size_t j = 0; j < ix; ++j)
          free(as[j]);
        free(as);
        return 1;
      }
    }
  }

  // Reopening the file for reading
  if (choiceFileFormat == 1) {
    FILE *file = fopen("matrix.dat", "rb");
    if (file == NULL) {
      printf("There was an error opening the file for reading.\n");
      return -1;
    }

    if (choiceAllocationMethod == 1) {
      fread(asentries, sizeof(int), size * size, file);
    } else if (choiceAllocationMethod == 2) {
      for (size_t ix = 0; ix < size; ++ix)
        fread(as[ix], sizeof(int), size, file);
    }

    fclose(file);

  } else if (choiceFileFormat == 2) {
    FILE *file = fopen("matrix.txt", "r");
    if (file == NULL) {
      printf("There was an error opening the file for reading.\n");
      return -1;
    }

    if (choiceAllocationMethod == 1) {
      for (size_t ix = 0; ix < size; ++ix)
        for (size_t jx = 0; jx < size; ++jx)
          fscanf(file, "%d", &as[ix][jx]);
    } else if (choiceAllocationMethod == 2) {
      for (size_t ix = 0; ix < size; ++ix)
        for (size_t jx = 0; jx < size; ++jx)
          fscanf(file, "%d", &as[ix][jx]);
    }

    fclose(file);
  }

  // Verifying the matrix
  int isCorrect = 1;

  for (size_t ix = 0; ix < size && isCorrect; ++ix) {
    for (size_t jx = 0; jx < size && isCorrect; ++jx) {
      if (as[ix][jx] != ix * jx) {
        isCorrect = 0;
        printf("Error at [%zu][%zu]: expected %zu, got %d\n", ix, jx, ix * jx,
               as[ix][jx]);
      }
    }
  }

  if (isCorrect) {
    printf("The matrix is correct.\n");
  } else {
    printf("The matrix is incorrect.\n");
  }

  // Frees the allocated memory after verification.
  if (choiceAllocationMethod == 1) {
    free(as);
    free(asentries);
  } else if (choiceAllocationMethod == 2) {
    for (size_t ix = 0; ix < size; ++ix)
      free(as[ix]);
    free(as);
  }

  return 0;
}

/*
   This program, when it's run, will ask the user the method of allocating
   memory when creating and writing the matrix to the 'matrix.dat' file.

   If the 'Contiguous Memory Allocation' is chosen, the memory addresses of each
   element of the array will be sequential.

   On the contrary, if 'Segmented Memory Allocation' is chosen, the memory
   addresses of each element of the array will be scattered and not necessarily
   sequential.

   This becomes evident when the program later reads the 'matrix.dat' file.

   If the memory was allocated in a single block, the program will read the
   array sequentially and will print the matrix as intended, with the
   initialized values in ascending order, row-first.

   If the memory was allocated segmented, the program will read and print the
   array sequentially but the values printed won't be necessarily the values
   pertaining to the matrix and if they are, they could not be in ascending
   order, row-first as intended. The values printed, among the ones of the
   matrix, will be the ones stored in some random memory addresses found when
   reading the file.
 */
