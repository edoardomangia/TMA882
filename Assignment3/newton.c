#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <stdbool.h>
#include <threads.h>

/* Constants */
#define MAX_ITER 128
#define CONVERGE_THRESHOLD 1e-3
#define DIVERGE_THRESHOLD 1e10

/* Structure to hold thread data */
typedef struct {
    int thread_id;
    int num_threads;
    int res;
    int deg;
    double real_min;
    double real_max;
    double imag_min;
    double imag_max;
    double *roots_real;
    double *roots_imag;
    unsigned char *attractors;   // Output buffer for attractors
    unsigned char *convergence;  // Output buffer for convergence
} thread_data_t;

/* Structure to hold RGB colors */
typedef struct {
    unsigned char r;
    unsigned char g;
    unsigned char b;
} color_t;

/* Function to convert HSV to RGB */
static inline color_t hsv_to_rgb(double h, double s, double v) {
    double c = v * s;
    double h_prime = fmod(h / 60.0, 6.0);
    double x = c * (1.0 - fabs(fmod(h_prime, 2.0) - 1.0));
    double m = v - c;
    double r_, g_, b_;

    if (0 <= h_prime && h_prime < 1) {
        r_ = c; g_ = x; b_ = 0;
    } else if (1 <= h_prime && h_prime < 2) {
        r_ = x; g_ = c; b_ = 0;
    } else if (2 <= h_prime && h_prime < 3) {
        r_ = 0; g_ = c; b_ = x;
    } else if (3 <= h_prime && h_prime < 4) {
        r_ = 0; g_ = x; b_ = c;
    } else if (4 <= h_prime && h_prime < 5) {
        r_ = x; g_ = 0; b_ = c;
    } else {
        r_ = c; g_ = 0; b_ = x;
    }

    color_t color;
    color.r = (unsigned char)((r_ + m) * 255.0);
    color.g = (unsigned char)((g_ + m) * 255.0);
    color.b = (unsigned char)((b_ + m) * 255.0);
    return color;
}

/* Function to compute roots of x^d -1 */
void compute_roots(int deg, double *roots_real, double *roots_imag) {
    for(int k = 0; k < deg; ++k){
        double angle = 2.0 * M_PI * k / deg;
        roots_real[k] = cos(angle);
        roots_imag[k] = sin(angle);
    }
}

/* Fast power function for complex numbers */
static inline void complex_pow(int deg, double zr, double zi, double *result_r, double *result_i) {
    double r = zr;
    double i = zi;
    double res_r = 1.0;
    double res_i = 0.0;

    for(int e = 0; e < deg; ++e){
        double temp_r = res_r * r - res_i * i;
        double temp_i = res_r * i + res_i * r;
        res_r = temp_r;
        res_i = temp_i;
    }
    *result_r = res_r;
    *result_i = res_i;
}

/* Thread function */
int thread_func(void *arg){
    thread_data_t *data = (thread_data_t*) arg;
    int thread_id = data->thread_id;
    int num_threads = data->num_threads;
    int res = data->res;
    int deg = data->deg;
    double real_min = data->real_min;
    double real_max = data->real_max;
    double imag_min = data->imag_min;
    double imag_max = data->imag_max;
    double *roots_real = data->roots_real;
    double *roots_imag = data->roots_imag;
    unsigned char *attractors = data->attractors;
    unsigned char *convergence = data->convergence;

    double real_step = (real_max - real_min) / (double)(res - 1);
    double imag_step = (imag_max - imag_min) / (double)(res - 1);

    // Determine the range of rows this thread will process
    for(int y = thread_id; y < res; y += num_threads){
        double zi = imag_max - y * imag_step; // y axis inverted
        for(int x = 0; x < res; ++x){
            double zr = real_min + x * real_step;
            double z_r = zr;
            double z_i = zi;

            int iter = 0;
            int root_found = deg; // Initialize to 'diverged'

            while(iter < MAX_ITER){
                // Compute z^deg
                double z_pow_r, z_pow_i;
                complex_pow(deg, z_r, z_i, &z_pow_r, &z_pow_i);

                // f(z) = z^deg - 1
                double f_r = z_pow_r - 1.0;
                double f_i = z_pow_i;

                // Compute f'(z) = deg * z^(deg-1)
                double z_pow_prev_r, z_pow_prev_i;
                complex_pow(deg - 1, z_r, z_i, &z_pow_prev_r, &z_pow_prev_i);
                double df_r = deg * z_pow_prev_r;
                double df_i = deg * z_pow_prev_i;

                // Compute |f'(z)|^2
                double df_mag_sq = df_r * df_r + df_i * df_i;
                if(df_mag_sq == 0.0){
                    break; // Avoid division by zero
                }

                // Compute f(z)/f'(z)
                double ratio_r = (f_r * df_r + f_i * df_i) / df_mag_sq;
                double ratio_i = (f_i * df_r - f_r * df_i) / df_mag_sq;

                // Update z: z = z - f(z)/f'(z)
                z_r -= ratio_r;
                z_i -= ratio_i;

                // Check convergence to any root
                bool converged = false;
                for(int k = 0; k < deg; ++k){
                    double dr = z_r - roots_real[k];
                    double di = z_i - roots_imag[k];
                    double dist_sq = dr * dr + di * di;
                    if(dist_sq < CONVERGE_THRESHOLD * CONVERGE_THRESHOLD){
                        root_found = k;
                        converged = true;
                        break;
                    }
                }
                if(converged){
                    break;
                }

                // Check divergence conditions
                double mag_sq = z_r * z_r + z_i * z_i;
                if(mag_sq < CONVERGE_THRESHOLD * CONVERGE_THRESHOLD || 
                   fabs(z_r) > DIVERGE_THRESHOLD || 
                   fabs(z_i) > DIVERGE_THRESHOLD){
                    root_found = deg; // 'diverged'
                    break;
                }

                iter++;
            }

            // Store results
            attractors[y * res + x] = (unsigned char)root_found;
            convergence[y * res + x] = (iter >= MAX_ITER) ? 255 : (unsigned char)(255.0 * iter / MAX_ITER);
        }
    }
    return 0;
}

/* Main function */
int main(int argc, char *argv[]){
    if(argc < 4){
        fprintf(stderr, "Usage: %s -t<num_threads> -l<resolution> <degree>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int num_threads = 1;
    int res = 1000;
    int deg = 3;

    /* Parse command line arguments */
    for(int i =1; i < argc -1; ++i){
        if(strncmp(argv[i], "-t", 2) ==0){
            num_threads = atoi(argv[i]+2);
            if(num_threads <1){
                fprintf(stderr, "Number of threads must be at least 1.\n");
                return EXIT_FAILURE;
            }
        }
        else if(strncmp(argv[i], "-l", 2) ==0){
            res = atoi(argv[i]+2);
            if(res <1){
                fprintf(stderr, "Resolution must be at least 1.\n");
                return EXIT_FAILURE;
            }
        }
        else{
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            return EXIT_FAILURE;
        }
    }

    /* Last argument is degree */
    deg = atoi(argv[argc -1]);
    if(deg <1 || deg >=10){
        fprintf(stderr, "Degree must be between 1 and 9.\n");
        return EXIT_FAILURE;
    }

    /* Compute roots */
    double *roots_real = malloc(sizeof(double) * deg);
    double *roots_imag = malloc(sizeof(double) * deg);
    if(!roots_real || !roots_imag){
        fprintf(stderr, "Memory allocation failed for roots.\n");
        free(roots_real);
        free(roots_imag);
        return EXIT_FAILURE;
    }
    compute_roots(deg, roots_real, roots_imag);

    /* Allocate output buffers */
    size_t total_pixels = (size_t)res * res;
    unsigned char *attractors = malloc(sizeof(unsigned char) * total_pixels);
    unsigned char *convergence = malloc(sizeof(unsigned char) * total_pixels);
    if(!attractors || !convergence){
        fprintf(stderr, "Memory allocation failed for output buffers.\n");
        free(roots_real);
        free(roots_imag);
        if(attractors) free(attractors);
        if(convergence) free(convergence);
        return EXIT_FAILURE;
    }

    /* Initialize output buffers */
    memset(attractors, 0, sizeof(unsigned char) * total_pixels);
    memset(convergence, 0, sizeof(unsigned char) * total_pixels);

    /* Prepare thread data */
    thrd_t *threads = malloc(sizeof(thrd_t) * num_threads);
    thread_data_t *thread_data = malloc(sizeof(thread_data_t) * num_threads);
    if(!threads || !thread_data){
        fprintf(stderr, "Memory allocation failed for threads.\n");
        free(roots_real);
        free(roots_imag);
        free(attractors);
        free(convergence);
        if(threads) free(threads);
        if(thread_data) free(thread_data);
        return EXIT_FAILURE;
    }

    for(int i=0; i < num_threads; ++i){
        thread_data[i].thread_id = i;
        thread_data[i].num_threads = num_threads;
        thread_data[i].res = res;
        thread_data[i].deg = deg;
        thread_data[i].real_min = -2.0;
        thread_data[i].real_max = 2.0;
        thread_data[i].imag_min = -2.0;
        thread_data[i].imag_max = 2.0;
        thread_data[i].roots_real = roots_real;
        thread_data[i].roots_imag = roots_imag;
        thread_data[i].attractors = attractors;
        thread_data[i].convergence = convergence;

        if(thrd_create(&threads[i], thread_func, &thread_data[i]) != thrd_success){
            fprintf(stderr, "Failed to create thread %d\n", i);
            // Cleanup
            for(int j=0; j < i; ++j){
                thrd_join(threads[j], NULL);
            }
            free(roots_real);
            free(roots_imag);
            free(attractors);
            free(convergence);
            free(threads);
            free(thread_data);
            return EXIT_FAILURE;
        }
    }

    /* Wait for threads to finish */
    for(int i=0; i < num_threads; ++i){
        thrd_join(threads[i], NULL);
    }

    /* Generate colors for roots */
    color_t *root_colors = malloc(sizeof(color_t) * (deg +1)); // +1 for divergence
    if(!root_colors){
        fprintf(stderr, "Memory allocation failed for root colors.\n");
        free(roots_real);
        free(roots_imag);
        free(threads);
        free(thread_data);
        free(attractors);
        free(convergence);
        return EXIT_FAILURE;
    }

    for(int k=0; k < deg; ++k){
        double hue = 360.0 * k / deg;
        root_colors[k] = hsv_to_rgb(hue, 1.0, 1.0);
    }
    // Color for divergence (black)
    root_colors[deg].r = 0;
    root_colors[deg].g = 0;
    root_colors[deg].b = 0;

    /* Open PPM files in binary mode */
    char attractor_filename[50];
    char convergence_filename[50];
    sprintf(attractor_filename, "newton_attractors_x%d.ppm", deg);
    sprintf(convergence_filename, "newton_convergence_x%d.ppm", deg);

    FILE *f_attractor = fopen(attractor_filename, "wb");
    FILE *f_convergence = fopen(convergence_filename, "wb");
    if(!f_attractor || !f_convergence){
        fprintf(stderr, "Failed to open output files.\n");
        free(roots_real);
        free(roots_imag);
        free(threads);
        free(thread_data);
        free(attractors);
        free(convergence);
        free(root_colors);
        if(f_attractor) fclose(f_attractor);
        if(f_convergence) fclose(f_convergence);
        return EXIT_FAILURE;
    }

    /* Write PPM headers (P6 - binary) */
    fprintf(f_attractor, "P6\n%d %d\n255\n", res, res);
    fprintf(f_convergence, "P6\n%d %d\n255\n", res, res);

    /* Prepare buffer for row data */
    size_t row_size = (size_t)res * 3; // RGB per pixel
    unsigned char *attractor_row = malloc(sizeof(unsigned char) * row_size);
    unsigned char *convergence_row = malloc(sizeof(unsigned char) * row_size);
    if(!attractor_row || !convergence_row){
        fprintf(stderr, "Memory allocation failed for PPM rows.\n");
        fclose(f_attractor);
        fclose(f_convergence);
        free(roots_real);
        free(roots_imag);
        free(threads);
        free(thread_data);
        free(attractors);
        free(convergence);
        free(root_colors);
        if(attractor_row) free(attractor_row);
        if(convergence_row) free(convergence_row);
        return EXIT_FAILURE;
    }

    /* Write pixel data */
    for(int y=0; y < res; ++y){
        for(int x=0; x < res; ++x){
            int idx = y * res + x;
            int root_idx = attractors[idx];
            color_t color;
            if(root_idx >=0 && root_idx < deg){
                color = root_colors[root_idx];
            }
            else{
                color = root_colors[deg]; // Diverged
            }
            attractor_row[x * 3]     = color.r;
            attractor_row[x * 3 +1]  = color.g;
            attractor_row[x * 3 +2]  = color.b;

            // Convergence: grayscale
            unsigned char gray = convergence[idx];
            convergence_row[x * 3]     = gray;
            convergence_row[x * 3 +1]  = gray;
            convergence_row[x * 3 +2]  = gray;
        }
        // Write the entire row at once
        fwrite(attractor_row, sizeof(unsigned char), row_size, f_attractor);
        fwrite(convergence_row, sizeof(unsigned char), row_size, f_convergence);
    }

    /* Cleanup */
    fclose(f_attractor);
    fclose(f_convergence);
    free(roots_real);
    free(roots_imag);
    free(threads);
    free(thread_data);
    free(attractors);
    free(convergence);
    free(root_colors);
    free(attractor_row);
    free(convergence_row);

    return EXIT_SUCCESS;
}

