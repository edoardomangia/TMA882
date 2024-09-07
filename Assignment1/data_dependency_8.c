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

  // Unrolled 8, inline row sums computation
  for ( size_t ix = 0; ix < nrs; ++ix ) {
    double sum0 = 0.;
    double sum1 = 0.;
    double sum2 = 0.;
    double sum3 = 0.;
    double sum4 = 0.;
    double sum5 = 0.;
    double sum6 = 0.;
    double sum7 = 0.;
    
    size_t jx;
    for ( jx = 0; jx + 7 < ncs; jx += 8 ) {
      sum0 += matrix[ix][jx];
      sum1 += matrix[ix][jx + 1];
      sum2 += matrix[ix][jx + 2];
      sum3 += matrix[ix][jx + 3];
      sum4 += matrix[ix][jx + 4];
      sum5 += matrix[ix][jx + 5];
      sum6 += matrix[ix][jx + 6];
      sum7 += matrix[ix][jx + 7];
    }

    // Handle any remaining elements
    double remaining_sum = 0.;
    for ( ; jx < ncs; ++jx ) {
      remaining_sum += matrix[ix][jx];
    }

    row_sum_results[ix] = sum0 + sum1 + sum2 + sum3 + sum4 + sum5 + sum6 + sum7 + remaining_sum;
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