#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main() {
  size_t nrs = 1000, ncs = 1000;

  // Allocate memory for the matrix
  double **matrix = malloc(sizeof(double *) * nrs);

  for (size_t ix = 0; ix < nrs; ++ix)
    matrix[ix] = malloc(sizeof(double) * ncs);

  // Initialize the matrix with the value 1
  for (size_t ix = 0; ix < nrs; ++ix) {
    for (size_t jx = 0; jx < ncs; ++jx) {
      matrix[ix][jx] = 1.0;
    }
  }

  // Allocate memory for row and column sums
  double *row_sum_results = malloc(sizeof(double) * nrs);
  double *col_sum_results = malloc(sizeof(double) * ncs);

  // Unrolled 4 with array, inline row sums computation
  for ( size_t ix = 0; ix < nrs; ++ix ) {
    double sum[4] = {0., 0., 0., 0.};
    
    size_t jx;
    for ( jx = 0; jx + 3 < ncs; jx += 4 ) {
      sum[0] += matrix[ix][jx];
      sum[1] += matrix[ix][jx + 1];
      sum[2] += matrix[ix][jx + 2];
      sum[3] += matrix[ix][jx + 3];
    }

    // Handle any remaining elements
    double remaining_sum = 0.;
    for ( ; jx < ncs; ++jx ) {
      remaining_sum += matrix[ix][jx];
    }

    // Sum the accumulated results from the array
    row_sum_results[ix] = sum[0] + sum[1] + sum[2] + sum[3] + remaining_sum;
  }

  /*  
  // Inline column sums computation
  for (size_t jx = 0; jx < ncs; ++jx) {
    double sum = 0.;
    for (size_t ix = 0; ix < nrs; ++ix) {
      sum += matrix[ix][jx];
    }
    col_sum_results[jx] = sum;
  }
  */
  
  // Print first 10 row sums for verification
  printf("Row sums:\n");
  for (size_t ix = 0; ix < 10; ++ix)
    printf("%zu: %f\n", ix, row_sum_results[ix]);

  /*
  // Print first 10 column sums for verification
  printf("\nColumn sums:\n");
  for (size_t jx = 0; jx < 10; ++jx)
    printf("%zu: %f\n", jx, col_sum_results[jx]);
  */

  // Seed the random number generator
  srand((unsigned int)time(NULL));

  // Print a random element from the sums to avoid over-simplification
  size_t random_row_index = rand() % nrs;
  size_t random_col_index = rand() % ncs;
  printf("\nRandom row sum element [%zu]: %f\n", random_row_index,
         row_sum_results[random_row_index]);
  printf("Random column sum element [%zu]: %f\n", random_col_index,
         col_sum_results[random_col_index]);

  // Free allocated memory
  free(row_sum_results);
  free(col_sum_results);
  for (size_t ix = 0; ix < nrs; ++ix) {
    free(matrix[ix]);
  }
  free(matrix);

  return 0;
}
