#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Function to get the current time in seconds
double get_time() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec + ts.tv_nsec * 1e-9;
}

int main() {
    size_t size = 1000000;
    size_t size_jump = 1000;
    double a = 2.5;

    // Allocate vectors on the heap
    double *x = (double *)malloc(size * sizeof(double));
    double *y = (double *)malloc(size * sizeof(double));
    size_t *p = (size_t *)malloc(size * sizeof(size_t));

    // Initialize vectors x and y
    for (size_t i = 0; i < size; ++i) {
        x[i] = 1.0;  // You can use random values or other initializations
        y[i] = 0.0;
    }

    // Benchmark iterations
    size_t iterations = 1000;

    // Option 1: Linear Initialization of p
    for (size_t ix = 0; ix < size; ++ix) {
        p[ix] = ix;
    }

    // Benchmark indirect addressing with linear initialization
    double start_time = get_time();
    for (size_t iter = 0; iter < iterations; ++iter) {
        for (size_t kx = 0; kx < size; ++kx) {
            size_t jx = p[kx];
            y[jx] += a * x[jx];
        }
    }
    double end_time = get_time();
    printf("Indirect Addressing (Linear) took %f seconds\n", end_time - start_time);

    // Option 2: Jump Initialization of p
    for (size_t jx = 0, kx = 0; jx < size_jump; ++jx) {
        for (size_t ix = jx; ix < size; ix += size_jump, ++kx) {
            p[ix] = kx;
        }
    }

    // Reset y
    for (size_t i = 0; i < size; ++i) {
        y[i] = 0.0;
    }

    // Benchmark indirect addressing with jump initialization
    start_time = get_time();
    for (size_t iter = 0; iter < iterations; ++iter) {
        for (size_t kx = 0; kx < size; ++kx) {
            size_t jx = p[kx];
            y[jx] += a * x[jx];
        }
    }
    end_time = get_time();
    printf("Indirect Addressing (Jump) took %f seconds\n", end_time - start_time);

    // Option 3: Direct Access without Indirect Addressing
    // Reset y
    for (size_t i = 0; i < size; ++i) {
        y[i] = 0.0;
    }

    // Benchmark direct access
    start_time = get_time();
    for (size_t iter = 0; iter < iterations; ++iter) {
        for (size_t jx = 0; jx < size; ++jx) {
            y[jx] += a * x[jx];
        }
    }
    end_time = get_time();
    printf("Direct Access took %f seconds\n", end_time - start_time);

    // Print a random element to prevent over-optimization
    printf("Random y[500] = %f\n\n", y[500]);

    // Free memory
    free(x);
    free(y);
    free(p);

    return 0;
}

