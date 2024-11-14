// diffusion.c
// Compile with: mpicc diffusion.c -o diffusion -lm
// Run with: mpirun -n <num_processes> ./diffusion -n<num_iterations> -d<diffusion_constant>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <mpi.h>

// Function to initialize the grid from file
void initialize_grid(const char* filename, int width, int height, float* grid) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        perror("Failed to open init file");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    int x, y;
    float value;
    while (fscanf(fp, "%d %d %f", &x, &y, &value) == 3) {
        if (x >= 0 && x < width && y >= 0 && y < height) {
            grid[y * width + x] = value;
        }
    }
    fclose(fp);
}

// Function to perform a single diffusion step
void diffuse_step(int width, int height, float* current, float* next, float c) {
    for (int j = 1; j < height - 1; j++) {
        for (int i = 1; i < width - 1; i++) {
            int idx = j * width + i;
            float avg = (current[(j - 1) * width + i] + current[(j + 1) * width + i] +
                         current[j * width + (i - 1)] + current[j * width + (i + 1)]) / 4.0f;
            next[idx] = current[idx] + c * (avg - current[idx]);
        }
    }
}

int main(int argc, char** argv) {
    int width, height, num_iterations = 0;
    float diffusion_const = 0.0f;

    MPI_Init(&argc, &argv);
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            num_iterations = atoi(argv[i + 1]);
            i++;
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            diffusion_const = atof(argv[i + 1]);
            i++;
        } else {
            if (rank == 0) printf("Usage: mpirun -n <num_processes> %s -n<num_iterations> -d<diffusion_constant>\n", argv[0]);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
    }

    if (num_iterations <= 0 || diffusion_const <= 0.0f) {
        if (rank == 0) printf("Invalid arguments. Ensure num_iterations and diffusion_constant are positive.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Only the root process reads the grid dimensions from the "init" file
    if (rank == 0) {
        FILE* fp = fopen("init", "r");
        if (!fp || fscanf(fp, "%d %d", &width, &height) != 2) {
            if (rank == 0) printf("Failed to read width and height from init file.\n");
            fclose(fp);
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        fclose(fp);
    }

    // Broadcast the width and height to all processes
    MPI_Bcast(&width, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&height, 1, MPI_INT, 0, MPI_COMM_WORLD);

    int grid_size = width * height;
    float* current_grid = (float*)calloc(grid_size, sizeof(float));
    float* next_grid = (float*)calloc(grid_size, sizeof(float));

    // Initialize grid only on root
    if (rank == 0) {
        initialize_grid("init", width, height, current_grid);
    }

    // Broadcast the initial grid to all processes
    MPI_Bcast(current_grid, grid_size, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // Run diffusion steps
    for (int iter = 0; iter < num_iterations; iter++) {
        // Each process performs its own diffuse step
        diffuse_step(width, height, current_grid, next_grid, diffusion_const);

        // Synchronize the grids
        MPI_Allreduce(next_grid, current_grid, grid_size, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);

        // Swap grids
        float* temp = current_grid;
        current_grid = next_grid;
        next_grid = temp;
    }

    // Calculate average temperature
    double local_sum = 0.0;
    for (int i = 0; i < grid_size; i++) {
        local_sum += current_grid[i];
    }

    double global_sum;
    MPI_Reduce(&local_sum, &global_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    double average = global_sum / (grid_size * size);

    // Calculate average absolute difference
    double local_abs_diff_sum = 0.0;
    for (int i = 0; i < grid_size; i++) {
        local_abs_diff_sum += fabs(current_grid[i] - average);
    }

    double global_abs_diff_sum;
    MPI_Reduce(&local_abs_diff_sum, &global_abs_diff_sum, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    double avg_abs_diff = global_abs_diff_sum / (grid_size * size);

    // Output results on the root process
    if (rank == 0) {
        printf("average: %.6f\n", average);
        printf("average absolute difference: %.6f\n", avg_abs_diff);
    }

    // Cleanup
    free(current_grid);
    free(next_grid);

    MPI_Finalize();
    return 0;
}

