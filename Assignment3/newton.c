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
int img_res;    // Resolution of image and degree of polynomial
int poly_deg;

// Thread variables
int thread_count;                // Number of threads
int ** roots_list;               // Pointer to pointer list for roots and iterations
int ** iter_list;
char * line_done;                // keeps track of which lines have been calculated
struct timespec sleep_time;
void* thread_args;               // Thread index
pthread_mutex_t line_done_mutex; // Mutex variable for threads
//

double *real_parts;              // List for real/imaginary-parts of the roots 
double *imag_parts;

int colorMapR[] = {255, 0, 0, 255, 255, 0, 255};    // Colors for each root
int colorMapG[] = {0, 255, 0, 145, 0, 125, 255};
int colorMapB[] = {0, 255, 0, 0, 255, 255, 0};

FILE *file_attractors;   // File-pointer for attractors file
FILE *file_convergence;  // File-pointer for convergence file
/* GLOBAL VARIABLES */


void compute_newton(int poly_deg, int *root_array, int *iter_array, int row_index, int img_res, double *real_parts, double *imag_parts);
void compute_roots(int poly_deg, double *real_parts, double *imag_parts);
void write_image(FILE *file_attractors, FILE *file_convergence, int *colorMapR, int *colorMapG, int *colorMapB, int *iterations, int *roots, int img_res);
void *compute_newton_thread(void *thread_args);
void *write_image_thread(void *thread_args);

int main(int argc, char const *argv[])
{   
    char *endptr1;
    char *endptr2;

    poly_deg = atoi(argv[3]);        // degree of the function: x^(deg) - 1

    if(argc != 4){
        printf("\nArgument count not equal to 4. Exiting.\n");
    }
    else {
        if(strncmp("-t", argv[1], 2) == 0){
            thread_count = strtol(argv[1]+2, &endptr1, 10);
            img_res = strtol(argv[2]+2, &endptr2, 10);
        }
        else{
            thread_count = strtol(argv[2]+2, &endptr1, 10);
            img_res = strtol(argv[1]+2, &endptr2, 10);
        }
    }
    
    real_parts = (double*) malloc(sizeof(double) * img_res);
    imag_parts = (double*) malloc(sizeof(double) * img_res);

		// Calculates the roots of the function f(x) = x^(deg) - 1
    compute_roots(poly_deg, real_parts, imag_parts);    

		// Initializing the .ppm file, creating the header
    char attractor_filename[27];	
    char convergence_filename[27];

    sprintf(attractor_filename, "newton_attractors_x%d.ppm", poly_deg);
    sprintf(convergence_filename, "newton_convergence_x%d.ppm", poly_deg);

    file_attractors = fopen(attractor_filename, "wb");
    file_convergence = fopen(convergence_filename, "wb");

    fprintf(file_attractors, "P6\n%d %d\n255\n", img_res, img_res);
    fprintf(file_convergence, "P6\n%d %d\n75\n", img_res, img_res);
    
    // Creates "roots_list" and "iter_list" for each of the rows in the image
    // so the threads can work locally on a row and save the results to these lists
    roots_list = (int**) malloc(sizeof(int*) * img_res);
    iter_list = (int**) malloc(sizeof(int*) * img_res);

    // allocates memory for check list full of zeros
    line_done = (char*)calloc(img_res, sizeof(char));     // Used in nanosleep function in write_image_thread
    sleep_time.tv_sec = 0;                             // Waits 0s + 10000ns
    sleep_time.tv_nsec = 10000;
    
    pthread_t* compute_threads = (pthread_t*)malloc(sizeof(pthread_t)*thread_count);   // Allocate memory for pointer to threads       
    for (int t_index = 0; t_index < thread_count; t_index ++) {
        int *thread_args = malloc(sizeof(int));
        *thread_args = t_index;
        pthread_create(&compute_threads[t_index], NULL, compute_newton_thread, (void*) thread_args);  // Creates compute-threads and executes compute_newton_thread
    }
    pthread_t write_thread;
    pthread_create(&write_thread, NULL, write_image_thread, NULL);   // Create write-thread
    
    int thread_ret;
    for (size_t t_index = 0; t_index < thread_count; t_index++) {
        if ((thread_ret = pthread_join(compute_threads[t_index], NULL))){
            printf("Error joining thread: %d\n", thread_ret);
            exit(1);
        }
    }
    thread_ret = pthread_join(write_thread, NULL);
    pthread_mutex_destroy(&line_done_mutex);
    
    fclose(file_attractors);
    fclose(file_convergence);
    
    return 0;
}

// Maps each pixel (column) in the image to a complex number
// Applies Newton's method to find the roots of the polynomial for each pixel, checks for convergence or divergence
// For each pixel, records the index of the root to which it converged and the number of iteration taken
void compute_newton(int poly_deg, int *root_array, int *iter_array, int row_index, int img_res, double *real_parts, double *imag_parts){
    int iter;       			
    bool continue_step;		

    double origin;
    double delta_re;
    double delta_im;
    double distance;
    
    double step_size = 1 / thread_count;      // Step size column direction

    double img_part_row = 2 - 4*row_index/(double)img_res;	// Mapping of row index to the imaginary part 
    double inv_res = 1/(double)img_res;											// Precompute inverse of resolution 

    int root_idx;    			// Variable to store the root-index...
    int iter_placeholder; // ...and temporarily iteration count
    
    long long int large_value_limit = 10000000000; // Threshold to detect divergence

    double complex z;
    double complex z_inverse;    // Inverse of z

    for(size_t col_index = 0; col_index < img_res; ++col_index){
        continue_step = true;
        iter_placeholder = 0;
        root_idx = 0;
        iter = 0;
        z = -2 + 4*((double) col_index*inv_res) + img_part_row*I; // Initializes x based on the pixel position

        while(continue_step)
        {   
            if(iter > 500){    // Limits iterations to 500
                iter_placeholder = iter;
                continue_step = false;
                break;
            }
            origin = creal(z)*creal(z) + cimag(z)*cimag(z); // Calculate the distance to origin (squared)
            if(origin < 0.000001){													// If x is close to the origin, stops
                iter_placeholder = iter;
                continue_step = false;
                break;
            }
            
            if(origin - 1 < 0.001){					// Checks if z is close to the unit circle
                for(size_t root = 0; root < poly_deg; ++root){
                    delta_re = creal(z) - real_parts[root];              // Difference in real part
                    delta_im = cimag(z) - imag_parts[root]; 						 // Difference in imaginary part
                    distance = delta_re*delta_re + delta_im*delta_im;    // Calculate distance to the root (squared)
                		
										// Checks if the point has converged to the current root or if it's diverging 
                    if((distance < 0.000001) || (creal(z) > large_value_limit) || (cimag(z) > large_value_limit)){ 
                        root_idx = root;				 // Stores the index of the converged root
                        iter_placeholder = iter; // Stores the number of iterations it took to convergence
                        continue_step = false;
                        break;
                    }
                }
            }
            if(continue_step){        // Only proceed if the point hasn't converged  
                switch(poly_deg)
                {
                case 1: 		// For degree 1, the root is always at z = 1
                    z = 1;	
                    break;
                
                case 2:			// ...use the Newton's formula for z^2 - 1
                    z = 0.5*((creal(z)/origin - (cimag(z)/origin)*I) + z);
                    break;

                case 5:			// ...calculate 1/z and use it in the Newton's method step
                    z_inverse = creal(z)/origin - (cimag(z)/origin)*I;    
                    z = 0.2*(z_inverse*z_inverse*z_inverse*z_inverse) + 0.8*z;
                    break;
                
                case 7:			// ...
                    z_inverse = creal(z)/origin - (cimag(z)/origin)*I;    // calculate 1/z
                    z = 0.14285714*(z_inverse*z_inverse*z_inverse*z_inverse*z_inverse*z_inverse) + 0.85714286*z;
                    break;

                default:
                    break;
                }
            }
            ++iter;
        }
        root_array[col_index] = root_idx;      		// Stores the root index for this pixel
        iter_array[col_index] = iter_placeholder;	// Stores the iteration count for this pixel
    }
}

// Calculates the roots of a polynomial of the form z^n - 1
void compute_roots(int poly_deg, double *real_parts, double *imag_parts){
    for(size_t i = 0; i < poly_deg; ++i){
        real_parts[i] = cos(2*M_PI * i/poly_deg);
        imag_parts[i] = sin(2*M_PI * i/poly_deg);
    }
}

// Writes the results to 2 PPM image files 
void write_image(FILE *file_attractors, FILE *file_convergence, int *colorMapR, int *colorMapG, int *colorMapB, int *iterations, int *roots, int img_res){
    char pixel_buffer[img_res*3];			// Temporary buffer to store pixel color data for a row 
    
		size_t root_idx = 0;							//  

		// Sets the RGB values based on the root the pixel converged to 
    for (size_t pixel_idx = 0; pixel_idx < img_res*3; pixel_idx+=3){
        pixel_buffer[pixel_idx] = colorMapR[roots[root_idx]];					// Red
        pixel_buffer[pixel_idx+1] = colorMapG[roots[root_idx]];				// Green 
        pixel_buffer[pixel_idx+2] = colorMapB[roots[root_idx]];				// Blue
        ++root_idx;
    }
    fwrite(pixel_buffer, sizeof(char), img_res*3, file_attractors);

    root_idx = 0;
		
		// Sets the RGB valuse based on the number of iterations 
    for (size_t pixel_idx = 0; pixel_idx < img_res*3; pixel_idx+=3){
        pixel_buffer[pixel_idx] = iterations[root_idx];								// Red
        pixel_buffer[pixel_idx+1] = iterations[root_idx];							// Green
        pixel_buffer[pixel_idx+2] = iterations[root_idx];							// Blue
        ++root_idx;
    }
    fwrite(pixel_buffer, sizeof(char), img_res*3, file_convergence);
}

// Runs the Newton's method computation for a specific thread
// Each thread calculates the method for specific rows of the image, assigned based on the thread's index 
// The results are stored in shared arrays, with proper synchronization
void *compute_newton_thread(void *thread_args) {
    int offset = *((int*)thread_args);	// Extract thread index from the arguments
    free(thread_args);									

		// The starting row for each thread is determined by its thread index 
    for (size_t row_idx = offset; row_idx < img_res; row_idx += thread_count) {  
        
        int *roots_result = (int*)malloc(sizeof(int) * img_res);    // Memory for root results for the current row
        int *iter_result = (int*)malloc(sizeof(int) * img_res);			// Memory for iteration... 
        
        compute_newton(poly_deg, roots_result, iter_result, row_idx, img_res, real_parts, imag_parts);
        
        pthread_mutex_lock(&line_done_mutex);    // Locks the mutex before updating the shared data
        roots_list[row_idx] = roots_result;			 // Stores the results in the global root array
        iter_list[row_idx] = iter_result;				 //	...in the global iteration array
        line_done[row_idx] = 1;									 // Marks the row as calculated 
        pthread_mutex_unlock(&line_done_mutex);	 // Unlocks the mutex 
    }
    return NULL;
}

// Write-function for write-thread
void *write_image_thread(void* thread_args){
    char *line_done_local = (char*)calloc(img_res, sizeof(char)); // Tracks which rows have been fully processed

		for (size_t idx = 0; idx < img_res; ) {			// Iterates through each row of the image 
        pthread_mutex_lock(&line_done_mutex);		
        if (line_done[idx] != 0) {							// If already marked as done
            memcpy(line_done_local, line_done, img_res * sizeof(char));
        }
        pthread_mutex_unlock(&line_done_mutex);
        
        if (line_done_local[idx] == 0) {				// If current row not ready
            nanosleep(&sleep_time, NULL);				// The thread pauses 
            continue;														
        }
        for (; idx < img_res && line_done_local[idx] != 0; idx++) {	// Thread iterates over rows that are ready 
            int* root_row = roots_list[idx];	// For each completed row, retrieves root_rows from roots_list
            int* iter_row = iter_list[idx];		// ...

						// Writes the row's pixel data to the 2 output files 
            write_image(file_attractors, file_convergence, colorMapR, colorMapG, colorMapB, iter_row, root_row, img_res);
            free(root_row);     // Free memory for line idx roots in compute_newton_thread
            free(iter_row);     // Free memory for line idx iterations in compute_newton_thread
        }
    }
    return NULL;
}

