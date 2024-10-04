#include "omp.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DISTANCE_COUNT 3465

int main(int argc, char const *argv[]) {

  // TODO
  int num_threads = atoi(argv[1] + 2);

  FILE *file = fopen("cells", "r");
	
  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  printf("File size: %li\n", file_size);
	
  int cells_count = file_size / 24;
  printf("Cells count: %i\n", cells_count);
	
  float coord[cells_count][3];

  fseek(file, 0, SEEK_SET);

  float x;
  float y;
  float z;

  for (size_t i = 0; i < cells_count; i++) {
    fscanf(file, "%f %f %f", &x, &y, &z);
    coord[i][0] = x;
    coord[i][1] = y;
    coord[i][2] = z;
  }
	
	/*
  for (size_t i = 0; i < cells_count; i++) {
    for (size_t j = 0; j < 3; j++) {
      printf("%f ", coord[i][j]);
    }
    printf("\n");
  }
	*/

  fseek(file, 0, SEEK_SET);

  unsigned long distances[DISTANCE_COUNT] = {0};

  size_t cell_1;
  size_t cell_2;

  omp_set_num_threads(num_threads);

#pragma omp parallel shared(coord, cells_count)
  {
    // distances_local array is local for each thread
		unsigned int distances_local[DISTANCE_COUNT] = {0};

#pragma omp for private(cell_1, cell_2) schedule(dynamic, 30)
    for (cell_1 = 0; cell_1 < (cells_count - 1); cell_1++) {
      float x1 = coord[cell_1][0];
      float y1 = coord[cell_1][1];
      float z1 = coord[cell_1][2];

      for (cell_2 = (cell_1 + 1); cell_2 < cells_count; cell_2++) {
        float dis_x = x1 - coord[cell_2][0];
        float dis_y = y1 - coord[cell_2][1];
        float dis_z = z1 - coord[cell_2][2];
        ++distances_local[(unsigned int)(sqrtf((dis_x * dis_x + 
																								dis_y * dis_y + 
																								dis_z * dis_z)) 
																											* 100)];
      }
    }

#pragma omp critical
    {
      for (size_t i = 0; i < DISTANCE_COUNT; i++) 
        distances[i] += distances_local[i];
    }
  }

  for (size_t i = 0; i < DISTANCE_COUNT; i++) {
    if (distances[i] >= 1) 
      printf("%05.2f %ld\n", ((float)i) / 100, distances[i]); 
  } 
	fclose(file);
	return 0;
}
