#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <complex.h>
#include <math.h>
#include <string.h>
#include <pthread.h>
#include <stddef.h>
#include <time.h>

/* GLOBAL VARIABLES */

// Image resolution and polynomial degree
int image_resolution;  
int polynomial_degree;

// Thread-related variables
int total_threads;                   
int **root_indices;               // Stores root indices for each pixel
int **iteration_counts;           // Stores iteration counts for convergence
char *rows_completed;             // Flags to indicate completed rows
struct timespec sleep_duration;   
void* thread_parameter;           
pthread_mutex_t completion_mutex; // Mutex for synchronizing access to shared data

// Real and imaginary parts of the polynomial roots
double *root_real_parts;          
double *root_imag_parts;

// Color maps for rendering roots
int color_map_red[] = {255, 0, 0, 255, 255, 0, 255};
int color_map_green[] = {0, 255, 0, 145, 0, 125, 255};
int color_map_blue[] = {0, 255, 0, 0, 255, 255, 0};

// File pointers for output images
FILE *attractors_file;   
FILE *convergence_file;  

/* GLOBAL VARIABLES END */

/* Function Definitions */

/**
 * Maps each pixel in a row to a complex number and applies Newton's method to find roots.
 * Records the root index and the number of iterations taken for each pixel.
 */
void perform_newton_iteration(int poly_degree, int *roots_output, int *iterations_output, int row, int img_res, double *real_roots, double *imag_roots){
    int iteration;
    bool continue_iterating;

    double origin_magnitude_sq;
    double delta_real;
    double delta_imag;
    double distance;
    
    double step_size = 1.0 / total_threads;  // Step size in column direction

    // Map row index to the imaginary part of the complex plane
    double imag_part = 2.0 - 4.0 * row / (double)img_res;	
    double inv_resolution = 1.0 / (double)img_res;		

    int root_index;    			// Index of the converged root
    int iteration_temp; // Temporary variable to store iteration count
    
    long long int divergence_threshold = 10000000000; // Threshold to detect divergence

    double complex z;
    double complex z_inverse;    // Inverse of z

    for(size_t col = 0; col < img_res; ++col){
        continue_iterating = true;
        iteration_temp = 0;
        root_index = 0;
        iteration = 0;

        // Initialize z based on pixel position
        z = -2.0 + 4.0 * ((double)col * inv_resolution) + imag_part * I; 

        while(continue_iterating)
        {   
            if(iteration > 500){    // Limit iterations to prevent infinite loops
                iteration_temp = iteration;
                continue_iterating = false;
                break;
            }

            // Calculate the squared magnitude of z
            origin_magnitude_sq = creal(z) * creal(z) + cimag(z) * cimag(z); 
            
            if(origin_magnitude_sq < 0.000001){ // If z is very close to the origin
                iteration_temp = iteration;
                continue_iterating = false;
                break;
            }
            
            if(fabs(origin_magnitude_sq - 1.0) < 0.001){ // If z is close to the unit circle
                for(size_t r = 0; r < poly_degree; ++r){
                    // Calculate distance to each root
                    delta_real = creal(z) - real_roots[r];
                    delta_imag = cimag(z) - imag_roots[r];
                    distance = delta_real * delta_real + delta_imag * delta_imag;    

                    // Check for convergence or divergence
                    if((distance < 0.000001) || (creal(z) > divergence_threshold) || (cimag(z) > divergence_threshold)){ 
                        root_index = r;				 // Store converged root index
                        iteration_temp = iteration; // Store iteration count
                        continue_iterating = false;
                        break;
                    }
                }
            }

            if(continue_iterating){        // Proceed if not yet converged  
                switch(poly_degree)
                {
                case 1: // For degree 1, the root is always at z = 1
                    z = 1.0;	
                    break;
                
                case 2: // Apply Newton's method for z^2 - 1
                    z = 0.5 * ((creal(z) / origin_magnitude_sq - (cimag(z) / origin_magnitude_sq) * I) + z);
                    break;

                case 5: // Apply Newton's method for z^5 - 1
                    z_inverse = creal(z) / origin_magnitude_sq - (cimag(z) / origin_magnitude_sq) * I;    
                    z = 0.2 * cpow(z_inverse, 4) + 0.8 * z;
                    break;
                
                case 7: // Apply Newton's method for z^7 - 1
                    z_inverse = creal(z) / origin_magnitude_sq - (cimag(z) / origin_magnitude_sq) * I;    
                    z = (1.0 / 7.0) * cpow(z_inverse, 6) + (6.0 / 7.0) * z;
                    break;

                default:
                    break;
                }
            }
            ++iteration;
        }
        roots_output[col] = root_index;      		// Store root index for this pixel
        iterations_output[col] = iteration_temp;	// Store iteration count for this pixel
    }
}

/**
 * Calculates the roots of the polynomial z^n - 1 and stores their real and imaginary parts.
 */
void calculate_polynomial_roots(int poly_degree, double *real_roots, double *imag_roots){
    for(size_t i = 0; i < poly_degree; ++i){
        real_roots[i] = cos(2 * M_PI * i / poly_degree);
        imag_roots[i] = sin(2 * M_PI * i / poly_degree);
    }
}

/**
 * Writes the computed data to the attractors and convergence PPM image files.
 */
void generate_output_images(FILE *attractors, FILE *convergence, int *colorR, int *colorG, int *colorB, int *iterations, int *roots, int img_res){
    char pixel_buffer[img_res * 3];			// Buffer to store RGB data for a row 
    
    size_t root_idx = 0;							// Index to traverse pixel data

    // Assign RGB values based on converged roots
    for (size_t pixel = 0; pixel < img_res * 3; pixel += 3){
        pixel_buffer[pixel] = colorR[roots[root_idx]];			// Red
        pixel_buffer[pixel + 1] = colorG[roots[root_idx]];	// Green 
        pixel_buffer[pixel + 2] = colorB[roots[root_idx]];	// Blue
        ++root_idx;
    }
    fwrite(pixel_buffer, sizeof(char), img_res * 3, attractors);

    root_idx = 0;
    
    // Assign RGB values based on iteration counts
    for (size_t pixel = 0; pixel < img_res * 3; pixel += 3){
        unsigned char color_value = (iterations[root_idx] > 255) ? 255 : (unsigned char)iterations[root_idx];
        pixel_buffer[pixel] = color_value;				// Red
        pixel_buffer[pixel + 1] = color_value;			// Green
        pixel_buffer[pixel + 2] = color_value;			// Blue
        ++root_idx;
    }
    fwrite(pixel_buffer, sizeof(char), img_res * 3, convergence);
}

/**
 * Thread function to perform Newton's method computations.
 * Each thread handles a subset of rows based on its thread index.
 */
void *newton_thread_function(void *args) {
    int thread_id = *((int*)args);	// Extract thread index from arguments
    free(args);									

    // Iterate over assigned rows based on thread ID
    for (size_t row = thread_id; row < image_resolution; row += total_threads) {  
        
        int *current_roots = (int*)malloc(sizeof(int) * image_resolution);    // Allocate memory for root indices
        int *current_iterations = (int*)malloc(sizeof(int) * image_resolution);			// Allocate memory for iteration counts
        
        perform_newton_iteration(polynomial_degree, current_roots, current_iterations, row, image_resolution, root_real_parts, root_imag_parts);
        
        // Lock mutex before updating shared data
        pthread_mutex_lock(&completion_mutex);    
        root_indices[row] = current_roots;			 // Store root indices
        iteration_counts[row] = current_iterations;	 // Store iteration counts
        rows_completed[row] = 1;									 // Mark row as completed 
        pthread_mutex_unlock(&completion_mutex);	 // Unlock mutex 
    }
    return NULL;
}

/**
 * Thread function to write the computed image data to the output files.
 */
void *image_writer_thread(void* args){
    char *local_completion_flags = (char*)calloc(image_resolution, sizeof(char)); // Local copy of completion flags

    for (size_t row = 0; row < image_resolution; ) {			// Iterate through each row
        pthread_mutex_lock(&completion_mutex);		
        if (rows_completed[row] != 0) {							// If row is completed
            memcpy(local_completion_flags, rows_completed, image_resolution * sizeof(char));
        }
        pthread_mutex_unlock(&completion_mutex);
        
        if (local_completion_flags[row] == 0) {				// If current row not ready
            nanosleep(&sleep_duration, NULL);				// Pause briefly
            continue;														
        }
        
        // Write all consecutive ready rows
        for (; row < image_resolution && local_completion_flags[row] != 0; row++) {	
            int* roots_row = root_indices[row];	// Retrieve root indices for the row
            int* iterations_row = iteration_counts[row];		// Retrieve iteration counts for the row

            // Write the row's pixel data to both output files
            generate_output_images(attractors_file, convergence_file, color_map_red, color_map_green, color_map_blue, iterations_row, roots_row, image_resolution);
            
            // Free memory allocated for the row
            free(roots_row);     
            free(iterations_row);     
        }
    }
    free(local_completion_flags);
    return NULL;
}


/* Main Function */
int main(int argc, char const *argv[])
{   
    char *endptr_thread;
    char *endptr_resolution;

    // Parse polynomial degree from command-line arguments
    polynomial_degree = atoi(argv[3]);        

    if(argc != 4){
        printf("\nIncorrect number of arguments. Expected 3 arguments. Exiting.\n");
        return EXIT_FAILURE;
    }
    else {
        if(strncmp("-t", argv[1], 2) == 0){
            total_threads = strtol(argv[1]+2, &endptr_thread, 10);
            image_resolution = strtol(argv[2]+2, &endptr_resolution, 10);
        }
        else{
            total_threads = strtol(argv[2]+2, &endptr_thread, 10);
            image_resolution = strtol(argv[1]+2, &endptr_resolution, 10);
        }
    }

    // Allocate memory for root real and imaginary parts
    root_real_parts = (double*) malloc(sizeof(double) * image_resolution);
    root_imag_parts = (double*) malloc(sizeof(double) * image_resolution);

    // Calculate the roots of the polynomial f(z) = z^degree - 1
    calculate_polynomial_roots(polynomial_degree, root_real_parts, root_imag_parts);    

    // Initialize output file names
    char attractors_filename[50];	
    char convergence_filename[50];

    sprintf(attractors_filename, "newton_attractors_x%d.ppm", polynomial_degree);
    sprintf(convergence_filename, "newton_convergence_x%d.ppm", polynomial_degree);

    // Open output files in binary write mode
    attractors_file = fopen(attractors_filename, "wb");
    convergence_file = fopen(convergence_filename, "wb");

    // Write PPM headers
    fprintf(attractors_file, "P6\n%d %d\n255\n", image_resolution, image_resolution);
    fprintf(convergence_file, "P6\n%d %d\n75\n", image_resolution, image_resolution);
    
    // Allocate memory for root and iteration lists per row
    root_indices = (int**) malloc(sizeof(int*) * image_resolution);
    iteration_counts = (int**) malloc(sizeof(int*) * image_resolution);

    // Initialize row completion flags
    rows_completed = (char*)calloc(image_resolution, sizeof(char));     
    sleep_duration.tv_sec = 0;                             // Sleep for 0 seconds
    sleep_duration.tv_nsec = 10000;                        // and 10,000 nanoseconds
    
    // Allocate memory for compute threads
    pthread_t* compute_threads = (pthread_t*)malloc(sizeof(pthread_t) * total_threads);   
    
    // Create compute threads
    for (int thread_id = 0; thread_id < total_threads; thread_id++) {
        int *thread_arg = malloc(sizeof(int));
        *thread_arg = thread_id;
        pthread_create(&compute_threads[thread_id], NULL, newton_thread_function, (void*) thread_arg);  
    }
    
    // Create image writer thread
    pthread_t writer_thread;
    pthread_create(&writer_thread, NULL, image_writer_thread, NULL);   
    
    // Join compute threads
    int join_status;
    for (size_t thread_idx = 0; thread_idx < total_threads; thread_idx++) {
        if ((join_status = pthread_join(compute_threads[thread_idx], NULL))){
            printf("Error joining thread: %d\n", join_status);
            exit(EXIT_FAILURE);
        }
    }
    
    // Join writer thread
    join_status = pthread_join(writer_thread, NULL);
    
    // Clean up mutex
    pthread_mutex_destroy(&completion_mutex);
    
    // Close output files
    fclose(attractors_file);
    fclose(convergence_file);
    
    // Free allocated memory
    free(root_real_parts);
    free(root_imag_parts);
    free(root_indices);
    free(iteration_counts);
    free(rows_completed);
    free(compute_threads);
    
    return EXIT_SUCCESS;
}

