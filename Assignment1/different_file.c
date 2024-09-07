// This file contains only the declaration of the 'mul_cpx' function.
// The definition is in different_file_mul.c, which is passed as an argument when compiling (see Makefile_Inlining).

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Only the definiton of 'mul_cpx'.
void mul_cpx(double *a_re, double *a_im, double *b_re, double *b_im,
             double *c_re, double *c_im);

int main() {

  int N = 30000;
  double *as_re = (double *)malloc(N * sizeof(double));
  double *as_im = (double *)malloc(N * sizeof(double));
  double *bs_re = (double *)malloc(N * sizeof(double));
  double *bs_im = (double *)malloc(N * sizeof(double));
  double *cs_re = (double *)malloc(N * sizeof(double));
  double *cs_im = (double *)malloc(N * sizeof(double));

  srand(time(NULL));
  for (int i = 0; i < N; ++i) {
    bs_re[i] = (double)rand() / RAND_MAX;
    bs_im[i] = (double)rand() / RAND_MAX;
    cs_re[i] = (double)rand() / RAND_MAX;
    cs_im[i] = (double)rand() / RAND_MAX;
  }

  /*
		for (int i = 0; i < size; i++) {
		bs_re[i] = i * 0.1; // Example: Simple linear sequence
		bs_im[i] = i * 0.2; // Example: Simple linear sequence
		cs_re[i] = i * 0.3; // Example: Simple linear sequence
		cs_im[i] = i * 0.4; // Example: Simple linear sequence
		}
  */

  size_t num_iter = 500000;
  clock_t start, end;

  start = clock();

  for (int iter = 0; iter < num_iter; iter++) {
    for (int i = 0; i < N; ++i) {
      mul_cpx(&as_re[i], &as_im[i], &bs_re[i], &bs_im[i], &cs_re[i], &cs_im[i]);
    }
  }

  end = clock();

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
