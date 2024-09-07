#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void generate_random_coordinate(char *buffer) {
  int sign = rand() % 2 == 0 ? 1 : -1;
  int integer_part = rand() % 11; // 0 to 10 (since 10 is included in the range)
  int fractional_part = rand() % 1000; // 0 to 999
  double coordinate = sign * (integer_part + fractional_part / 1000.0);
  sprintf(buffer, "%+07.3f", coordinate);
}

void generate_coordinates_file(int num_cells, const char *filename) {
  FILE *file = fopen(filename, "w");
  if (file == NULL) {
    perror("Error opening file");
    exit(EXIT_FAILURE);
  }

  char coord1[10], coord2[10], coord3[10];
  for (int i = 0; i < num_cells; i++) {
    generate_random_coordinate(coord1);
    generate_random_coordinate(coord2);
    generate_random_coordinate(coord3);
    fprintf(file, "%s %s %s\n", coord1, coord2, coord3);
  }

  fclose(file);
}

int main() {
  srand(time(NULL)); // Seed the random number generator with current time
  generate_coordinates_file(10, "cells.txt");
  return 0;
}
