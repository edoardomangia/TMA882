// This file contains the declaration and definition of 'mul_cpx' aswell as the 'main'.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Function that computes the product of 2 complex number 'b' and 'c' and stores
// it in 'a'.
void mul_cpx(double *a_re, double *a_im, double *b_re, double *b_im,
             double *c_re, double *c_im) {
  *a_re = (*b_re) * (*c_re) - (*b_im) * (*c_im);
  *a_im = (*b_re) * (*c_im) + (*b_im) * (*c_re);
}

int main() {

  // Generates the 6 vectors of doubles, each of size 30000.
  int N = 30000;
  double *as_re = (double *)malloc(N * sizeof(double));
  double *as_im = (double *)malloc(N * sizeof(double));
  double *bs_re = (double *)malloc(N * sizeof(double));
  double *bs_im = (double *)malloc(N * sizeof(double));
  double *cs_re = (double *)malloc(N * sizeof(double));
  double *cs_im = (double *)malloc(N * sizeof(double));

  // Generates random 'b' and 'c' entries.

  srand(time(NULL));
  for (int i = 0; i < N; ++i) {
    bs_re[i] = (double)rand() / RAND_MAX;
    bs_im[i] = (double)rand() / RAND_MAX;
    cs_re[i] = (double)rand() / RAND_MAX;
    cs_im[i] = (double)rand() / RAND_MAX;
  }

  /*
		for (int i = 0; i < N; i++) {
		bs_re[i] = i * 0.1; // Example: Simple linear sequence
		bs_im[i] = i * 0.2; // Example: Simple linear sequence
		cs_re[i] = i * 0.3; // Example: Simple linear sequence
		cs_im[i] = i * 0.4; // Example: Simple linear sequence
		}
  */

  // Multiplies the random 'b' and 'c' entries with 'mul_cpx'.
  size_t n_iter = 500000;
  clock_t start, end;

  start = clock();
  
	for (int iter = 0; iter < n_iter; ++iter) {
    for (int i = 0; i < N; ++i) {
      mul_cpx(&as_re[i], &as_im[i], &bs_re[i], &bs_im[i], &cs_re[i], &cs_im[i]);
    }
  }

  end = clock();

  // Print a random 'a' result to the terminal.
  int random_index = rand() % N;
  printf("as[%d] = (%f, %f)\n", random_index, as_re[random_index],
         as_im[random_index]);

  printf("Elapsed time: %f secs\n", (double)(end - start) / CLOCKS_PER_SEC);

  free(as_re);
  free(as_im);
  free(bs_re);
  free(bs_im);
  free(cs_re);
  free(cs_im);

  return 0;
}
