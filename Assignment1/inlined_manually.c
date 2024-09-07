// No declaration nor definition of 'mul_cpx'. The multiplication is done with a
// for loop directly in the main function.

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

  size_t num_iter = 500000;
  clock_t start, end;

  start = clock();

  for (int iter = 0; iter < num_iter; iter++) {
    for (int i = 0; i < N; ++i) {
      as_re[i] = bs_re[i] * cs_re[i] - bs_im[i] * cs_im[i];
      as_im[i] = bs_re[i] * cs_im[i] + bs_im[i] * cs_re[i];
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
