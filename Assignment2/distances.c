#include "omp.h"
#include <math.h>
#include <stdint.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// A GLOBAL VAR FOR USER DEFINE BATCH_SIZE
/* We are allowed to use 5 MiB = 5 * 1024^2 bytes of memory.
Through our implementation, Type SHORT is used to store our data and there are two blocks stored in the memory to calculate distances at the same time.
As a result, we calculate the size of each block in the following way, 
BATCH_SIZE = 5 * 1024^2 / (2 * 3 * 2)
, where the first 2 is the size of SHORT, 3 means every point is three dimentional and the second 2 is the num of blocks.
The result is around 436906.
For simplicity and space for other variables, which is relatively small compared with the data, we take it as 400000.
*/
#define BATCH_SIZE 400000

void load_batch(int16_t (*batch)[3], size_t size, FILE* file)
{
    char per_line[25]; // for storing a line's info
    char temp_str[7]; // for string to integer
    for (size_t line = 0; line < size; line++) {
        fgets(per_line, 25, file);
        for (size_t ix = 0; ix < 3; ++ix) {
            memcpy(temp_str, &per_line[ix * 8], 3);
            memcpy(temp_str+3, &per_line[ix * 8 + 4], 3);
            temp_str[6] = '\0';
            batch[line][ix] = (int16_t)atoi(temp_str);
        }
    }
}

inline int16_t calculate_distance(int16_t *num_1, int16_t *num_2)
{
    float temp = sqrt((num_1[0] - num_2[0]) * (num_1[0] - num_2[0])
        + (num_1[1] - num_2[1]) * (num_1[1] - num_2[1])
        + (num_1[2] - num_2[2]) * (num_1[2] - num_2[2]));
    return (int16_t)(temp / 10);
}

void self_distance(int16_t (*batch)[3], size_t len, size_t *count)
{
    size_t ix, jx;
    int16_t distance;

    #pragma omp parallel for \
        default(none) private(ix, jx, distance) \
        shared(batch, len) reduction(+:count[:3465])

    for (ix = 0; ix < len - 1; ix++) {
        for (jx = ix + 1; jx < len; jx++) {
            distance = calculate_distance(batch[ix], batch[jx]);
            count[distance] += 1;
        }
    }
}

void double_distance(int16_t (*batch_1)[3], int16_t (*batch_2)[3], size_t len_1, size_t len_2, size_t *count)
{
    size_t ix, jx;
    int16_t distance;
    
    #pragma omp parallel for \
        default(none) private(ix, jx, distance) \
        shared(batch_1, batch_2, len_1, len_2) reduction(+:count[:3465])

    for (ix = 0; ix < len_1; ix++) {
        for (jx = 0; jx < len_2; jx++) {
            distance = calculate_distance(batch_1[ix], batch_2[jx]);
            count[distance] += 1;
        }
    }
}

int main(int argc, char* argv[])
{
    // counter vector for tracking counted numbers
    size_t counter[3465] = {0};
    
    // read command line arguments
    int n_threads = 1;
    char* ptr = strchr(argv[1], 't');
    if (ptr) {
        n_threads = strtol(++ptr, NULL, 10);
    }
    omp_set_num_threads(n_threads);

    // open the file
    char filename[] = "cells";
    FILE* file;
    file = fopen(filename, "r");
    if (file == NULL) {
        printf("error opening file\n");
        return -1;
    }
    
    // Check file size and number of lines
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    size_t line_num = (file_size+1)/24;

    long file_cursor = ftell(file); // for store the current file position
    size_t batch_num;
    size_t last_batch_size = line_num % BATCH_SIZE;
    batch_num = (last_batch_size == 0) ? line_num/BATCH_SIZE : line_num/BATCH_SIZE + 1;
    size_t batch_size = BATCH_SIZE;

    // allocate memory for storing read lines
    
    int16_t (*batch_1)[3] = (int16_t (*)[3])malloc(batch_size * sizeof(int16_t[3]));
    int16_t (*batch_2)[3] = (int16_t (*)[3])malloc(batch_size * sizeof(int16_t[3]));
    fseek(file, 0, SEEK_SET); // to the start of the file
    
    // Outer loop
    for (size_t batch_out = 0; batch_out < batch_num; batch_out++) {
        size_t batch_size1;
        // check if it is the last batch
        batch_size1 = batch_size;
        if (batch_out == (batch_num - 1)) {
            batch_size1 = last_batch_size;
        }

        file_cursor = batch_out * batch_size * 24;
        fseek(file, file_cursor, SEEK_SET);

        // prepare batch_1 for outer loop
        load_batch(batch_1, batch_size1, file);

        self_distance(batch_1, batch_size1, counter);

//        file_cursor = (batch_out+1) * batch_size * 24;
//        fseek(file, file_cursor, SEEK_SET);
        
        // Inner loop
        for (size_t batch_in = batch_out + 1; batch_in < batch_num; batch_in++) {
            size_t batch_size2;
            // check if it is the last batch
            batch_size2 = batch_size;
            if (batch_in == (batch_num - 1)) {
                batch_size2 = last_batch_size;
            }

            load_batch(batch_2, batch_size2, file);

            double_distance(batch_1, batch_2, batch_size1, batch_size2, counter);

        } // end of inner loop
    } // end of outerloop
    
    for (size_t ix = 0; ix < 3465; ++ix) {
        if (counter[ix] != 0) {
            printf("%05.2f %lu\n", ix/100.0, counter[ix]);
        }
    }

    // close the file
    fclose(file);
    // free memory
    free(batch_1);
    free(batch_2);

    return 0;
}
