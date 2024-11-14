#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CL/cl.h>
#include <math.h>

// OpenCL kernel as a string
const char* kernelSource = 
"__kernel void diffuse(__global const float* current, __global float* next, "
"                      const int width, const int height, const float c) {"
"    int i = get_global_id(0);"
"    int j = get_global_id(1);"
"    // Boundary cells remain 0"
"    if (i == 0 || j == 0 || i == width-1 || j == height-1) {"
"        next[j * width + i] = 0.0f;"
"        return;"
"    }"
"    // Calculate index"
"    int idx = j * width + i;"
"    // Neighbor indices"
"    int up = (j-1) * width + i;"
"    int down = (j+1) * width + i;"
"    int left = j * width + (i-1);"
"    int right = j * width + (i+1);"
"    // Compute average of neighbors"
"    float avg = (current[up] + current[down] + current[left] + current[right]) / 4.0f;"
"    // Update temperature"
"    next[idx] = current[idx] + c * (avg - current[idx]);"
"}";

int main(int argc, char** argv) {
    // Default values
    int num_iterations = 0;
    float diffusion_const = 0.0f;

    // Parse command line arguments
    for(int i =1; i < argc; i++) {
        if(strcmp(argv[i], "-n") == 0 && i+1 < argc) {
            num_iterations = atoi(argv[i+1]);
            i++;
        }
        else if(strcmp(argv[i], "-d") == 0 && i+1 < argc) {
            diffusion_const = atof(argv[i+1]);
            i++;
        }
        else {
            printf("Usage: %s -n<num_iterations> -d<diffusion_constant>\n", argv[0]);
            return 1;
        }
    }

    if(num_iterations <=0 || diffusion_const <=0.0f) {
        printf("Invalid arguments. Ensure num_iterations and diffusion_constant are positive.\n");
        return 1;
    }

    // Read input file "init"
    FILE* fp = fopen("init", "r");
    if(!fp) {
        perror("Failed to open init file");
        return 1;
    }

    int width, height;
    if(fscanf(fp, "%d %d", &width, &height) !=2) {
        printf("Failed to read width and height from init file.\n");
        fclose(fp);
        return 1;
    }

    // Allocate and initialize grids
    size_t grid_size = width * height;
    float* current_grid = (float*)calloc(grid_size, sizeof(float));
    float* next_grid = (float*)calloc(grid_size, sizeof(float));
    if(!current_grid || !next_grid) {
        printf("Failed to allocate memory for grids.\n");
        fclose(fp);
        return 1;
    }

    // Read initial values
    int x, y;
    float value;
    while(fscanf(fp, "%d %d %f", &x, &y, &value) ==3) {
        if(x >=0 && x < width && y >=0 && y < height) {
            current_grid[y * width + x] = value;
        }
    }
    fclose(fp);

    // Set boundary cells to 0
    for(int i=0; i < width; i++) {
        current_grid[i] = 0.0f; // Top row
        current_grid[(height-1)*width + i] = 0.0f; // Bottom row
    }
    for(int j=0; j < height; j++) {
        current_grid[j*width + 0] = 0.0f; // Left column
        current_grid[j*width + (width-1)] = 0.0f; // Right column
    }

    // Initialize OpenCL
    cl_int err;

    // Get platform
    cl_uint num_platforms;
    err = clGetPlatformIDs(0, NULL, &num_platforms);
    if(err != CL_SUCCESS || num_platforms ==0) {
        printf("Failed to find any OpenCL platforms.\n");
        return 1;
    }
    cl_platform_id* platforms = (cl_platform_id*)malloc(sizeof(cl_platform_id)*num_platforms);
    clGetPlatformIDs(num_platforms, platforms, NULL);

    // Get device
    cl_device_id device_id = NULL;
    for(cl_uint i=0; i < num_platforms; i++) {
        cl_uint num_devices;
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, 0, NULL, &num_devices);
        if(err == CL_SUCCESS && num_devices >0) {
            cl_device_id* devices = (cl_device_id*)malloc(sizeof(cl_device_id)*num_devices);
            clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_GPU, num_devices, devices, NULL);
            device_id = devices[0];
            free(devices);
            break;
        }
    }
    free(platforms);
    if(device_id == NULL) {
        printf("Failed to find a GPU device.\n");
        free(current_grid);
        free(next_grid);
        return 1;
    }

    // Create context
    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if(err != CL_SUCCESS) {
        printf("Failed to create OpenCL context.\n");
        free(current_grid);
        free(next_grid);
        return 1;
    }

    // Create command queue
    cl_command_queue queue = clCreateCommandQueue(context, device_id, 0, &err);
    if(err != CL_SUCCESS) {
        printf("Failed to create command queue.\n");
        clReleaseContext(context);
        free(current_grid);
        free(next_grid);
        return 1;
    }

    // Create program
    cl_program program = clCreateProgramWithSource(context, 1, &kernelSource, NULL, &err);
    if(err != CL_SUCCESS) {
        printf("Failed to create OpenCL program.\n");
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(current_grid);
        free(next_grid);
        return 1;
    }

    // Build program
    err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    if(err != CL_SUCCESS) {
        // Print build log
        size_t log_size;
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        char* log = (char*)malloc(log_size);
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, log_size, log, NULL);
        printf("Error in kernel:\n%s\n", log);
        free(log);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(current_grid);
        free(next_grid);
        return 1;
    }

    // Create kernel
    cl_kernel kernel = clCreateKernel(program, "diffuse", &err);
    if(err != CL_SUCCESS) {
        printf("Failed to create kernel.\n");
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(current_grid);
        free(next_grid);
        return 1;
    }

    // Create buffers
    cl_mem current_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                          sizeof(float)*grid_size, current_grid, &err);
    cl_mem next_buffer = clCreateBuffer(context, CL_MEM_READ_WRITE,
                                       sizeof(float)*grid_size, NULL, &err);
    if(err != CL_SUCCESS) {
        printf("Failed to create buffers.\n");
        clReleaseKernel(kernel);
        clReleaseProgram(program);
        clReleaseCommandQueue(queue);
        clReleaseContext(context);
        free(current_grid);
        free(next_grid);
        return 1;
    }

    // Set kernel arguments that don't change
    clSetKernelArg(kernel, 2, sizeof(int), &width);
    clSetKernelArg(kernel, 3, sizeof(int), &height);
    clSetKernelArg(kernel, 4, sizeof(float), &diffusion_const);

    // Define the global and local work sizes
    size_t global_work_size[2] = { (size_t)width, (size_t)height };

    // Perform iterations
    for(int iter=0; iter < num_iterations; iter++) {
        // Set kernel arguments for current and next buffers
        clSetKernelArg(kernel, 0, sizeof(cl_mem), &current_buffer);
        clSetKernelArg(kernel, 1, sizeof(cl_mem), &next_buffer);

        // Enqueue kernel
        err = clEnqueueNDRangeKernel(queue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);
        if(err != CL_SUCCESS) {
            printf("Failed to enqueue kernel at iteration %d.\n", iter);
            break;
        }

        // Wait for kernel to finish
        clFinish(queue);

        // Swap buffers
        cl_mem temp = current_buffer;
        current_buffer = next_buffer;
        next_buffer = temp;
    }

    // Read back the final grid
    clEnqueueReadBuffer(queue, current_buffer, CL_TRUE, 0, sizeof(float)*grid_size, current_grid, 0, NULL, NULL);

    // Compute average
    double sum =0.0;
    for(int i=0; i < grid_size; i++) {
        sum += current_grid[i];
    }
    double average = sum / grid_size;

    // Compute average absolute difference
    double abs_diff_sum =0.0;
    for(int i=0; i < grid_size; i++) {
        abs_diff_sum += fabs(current_grid[i] - average);
    }
    double avg_abs_diff = abs_diff_sum / grid_size;

    // Output results
    printf("average: %.6f\n", average);
    printf("average absolute difference: %.6f\n", avg_abs_diff);

    // Cleanup
    clReleaseMemObject(current_buffer);
    clReleaseMemObject(next_buffer);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
    free(current_grid);
    free(next_grid);

    return 0;
}

