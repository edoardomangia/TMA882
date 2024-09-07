#include <math.h>
#include <omp.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 600000
#define DISTANCE_COUNT 3465

// Function that reads a number of cells from the 'cells.txt' file and loads the
// coordinates in the 'chunk' 2D array. Each cell consists of three 16-bit
// integer coordinates.

void load_chunk(int16_t (*chunk)[3], size_t size, FILE *file) {
  char cell_chars[25]; // Chars for a single cell (3 coords)
  char coord_chars[7]; // Chars for a single coord

  for (size_t cell = 0; cell < size;
       cell++) { // Iterates through cells in CHUNK_SIZE
    if (fgets(cell_chars, sizeof(cell_chars), file) ==
        NULL) { // Read 25 chars of the file into 'cell_chars' (create a cell)
      perror("Error reading file");
      exit(EXIT_FAILURE);
    }
    for (size_t i = 0; i < 3; i++) { // Iterate through the 3 coords of a cell
      memcpy(coord_chars, &cell_chars[i * 8], 3);								// Copy the first part of the coord (3 chars)
      memcpy(coord_chars + 3, &cell_chars[i * 8 + 4], 3);       // Copy the second part of the coord (3 chars)
      coord_chars[6] = '\0'; // Null-terminate the coord string
      chunk[cell][i] = (int16_t)atoi(coord_chars); // Convert coords_chars, store in the 'chunk' 2D array
    }
  }
}

int16_t compute_distance(const int16_t *num_1, const int16_t *num_2) {
  float temp = sqrtf((num_1[0] - num_2[0]) * (num_1[0] - num_2[0]) +
                     (num_1[1] - num_2[1]) * (num_1[1] - num_2[1]) +
                     (num_1[2] - num_2[2]) * (num_1[2] - num_2[2]));
  return (int16_t)(temp / 10.0f); // Ensure division by float
}

// Function that computes the distances between all pairs of cells within a
// single chunk of data. It also counts how often each distance occurs.

void self_distance(int16_t (*chunk)[3], size_t size, size_t *count) {
  size_t i, j;
  int16_t distance;

#pragma omp parallel private(i, j, distance) shared(chunk, size, count)
  {
    size_t local_count[DISTANCE_COUNT] = {0};

#pragma omp for
    for (i = 0; i < size - 1; i++) {
      for (j = i + 1; j < size; j++) {
        distance = compute_distance(chunk[i], chunk[j]);
        if (distance < DISTANCE_COUNT) {
          local_count[distance] += 1;
        }
      }
    }

#pragma omp critical
    {
      for (i = 0; i < DISTANCE_COUNT; i++) {
        count[i] += local_count[i];
      }
    }
  }
}

void double_distance(int16_t (*chunk1)[3], int16_t (*chunk2)[3], size_t len_1,
                     size_t len_2, size_t *count) {
  size_t i, j;
  int16_t distance;

#pragma omp parallel private(i, j, distance)                                   \
    shared(chunk1, chunk2, len_1, len_2, count)
  {
    size_t local_count[DISTANCE_COUNT] = {0};

#pragma omp for
    for (i = 0; i < len_1; i++) {
      for (j = 0; j < len_2; j++) {
        distance = compute_distance(chunk1[i], chunk2[j]);
        if (distance < DISTANCE_COUNT) {
          local_count[distance] += 1;
        }
      }
    }

#pragma omp critical
    {
      for (i = 0; i < DISTANCE_COUNT; i++) {
        count[i] += local_count[i];
      }
    }
  }
}

int main(int argc, char *argv[]) {
  FILE *file = fopen("cells.txt", "r");
  if (file == NULL) {
    perror("Error opening file");
    return EXIT_FAILURE;
  }

  fseek(file, 0, SEEK_END);
  long file_size = ftell(file);
  if (file_size < 0) {
    perror("Error getting file size");
    fclose(file);
    return EXIT_FAILURE;
  }

  fseek(file, 0, SEEK_SET);

  int coords_count = file_size / 8;
  int cells_count = coords_count / 3;

  printf("Coordinates: %i\nCells: %i\n", coords_count, cells_count);

  size_t chunks_index =
      (cells_count + CHUNK_SIZE - 1) /
      CHUNK_SIZE; // How many full chunks given the cells_count
  size_t last_chunk_size =
      cells_count % CHUNK_SIZE; // Size of the eventual last non-full chunk

  int16_t(*chunk_1)[3] = (int16_t(*)[3])malloc(sizeof(int16_t[3]) * CHUNK_SIZE);
  int16_t(*chunk_2)[3] = (int16_t(*)[3])malloc(sizeof(int16_t[3]) * CHUNK_SIZE);
  if (chunk_1 == NULL || chunk_2 == NULL) {
    perror("Error allocating memory");
    fclose(file);
    return EXIT_FAILURE;
  }

  size_t *counter = (size_t *)calloc(DISTANCE_COUNT, sizeof(size_t));
  if (counter == NULL) {
    perror("Error allocating memory");
    fclose(file);
    free(chunk_1);
    free(chunk_2);
    return EXIT_FAILURE;
  }

  // Iterate through the chunks
  for (size_t chunk_out = 0; chunk_out < chunks_index; chunk_out++) {
    size_t chunk_size1 = (chunk_out == chunks_index - 1)
                             ? last_chunk_size
                             : CHUNK_SIZE; // Check if last chunk

    long cursor =
        chunk_out * CHUNK_SIZE *
        24; // Cursor to the start of chunk (index * size * chars in a cell)
    fseek(file, cursor, SEEK_SET);

    load_chunk(chunk_1, chunk_size1, file);
    self_distance(chunk_1, chunk_size1, counter);

    for (size_t chunk_in = chunk_out + 1; chunk_in < chunks_index; chunk_in++) {
      size_t chunk_size2 =
          (chunk_in == chunks_index - 1) ? last_chunk_size : CHUNK_SIZE;

      cursor = chunk_in * CHUNK_SIZE * 24;
      fseek(file, cursor, SEEK_SET);

      load_chunk(chunk_2, chunk_size2, file);
      double_distance(chunk_1, chunk_2, chunk_size1, chunk_size2, counter);
    }
  }

  for (size_t i = 0; i < DISTANCE_COUNT; ++i) {
    if (counter[i] != 0) {
      printf("%05.2f %lu\n", i / 100.0, counter[i]);
    }
  }

  fclose(file);
  free(chunk_1);
  free(chunk_2);
  free(counter);

  return 0;
}
