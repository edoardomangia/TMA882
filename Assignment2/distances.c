#include <math.h>
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHUNK_SIZE 600000



void load_chunk(int16_t (*chunk)[3], size_t size, FILE* file) {
	char cell_chars[25];	// Chars for single cell (3 coords)
	char coord_chars[7];	// Chars for single coord

	for (size_t cell = 0; cell < size; cell++) {	// Iterates through cells in CHUNK_SIZE 
		fgets(cell_chars, 25, file);								// Gets the 25 chars of a cell 
		for (size_t i = 0; i < 3; i++) {
			memcpy(coord_chars, &cell_chars[i * 8], 3);					// Takes the coord_chars 
			memcpy(coord_chars + 3, &cell_chars[i * 8 + 4], 3);	
			coord_chars[6] = '\0';
			chunk[cell][i] = (int16_t)atoi(coord_chars);				// Converts and stores coord_chars in the chunk
		}
	}
}


inline int16_t compute_distance(int16_t *num_1, int16_t *num_2) {
	float temp = sqrt((num_1[0] - num_2[0]) * (num_1[0] - num_2[0])
			            + (num_1[1] - num_2[1]) * (num_1[1] - num_2[1])
								  + (num_1[2] - num_2[2]) * (num_1[2] - num_2[2]));
  return (int16_t)(temp / 10);
}


void self_distance(int16_t (*chunk)[3], size_t size, size_t *count) {
	for (size_t i = 0; i < size - 1; i++) {
		for (size_t j = i + 1; j < size; j++) {
			int16_t distance = compute_distance(chunk[i], chunk[j]);
			count[distance] += 1;
		}
	}
}


void double_distance(int16_t (*chunk1)[3], int16_t (*chunk2)[3], size_t len_1, size_t len_2, size_t *count) {
	for (size_t i = 0; i < len_1; i++) {
	  for (size_t j = 0; j < len_2; j++) {
	    int16_t distance = compute_distance(chunk1[i], chunk2[j]);
	    count[distance] += 1;
		}
	}
}


int main(int argc, char *argv[]) {

  FILE *file = fopen("cells.txt", "r");

  fseek(file, 0, SEEK_END);

  long file_size = ftell(file);

  int coords_count = file_size / 8;
  int cells_count = coords_count / 3;

  printf("Coordinates: %i\nPoints: %i \n", coords_count, cells_count);
	
	long cursor = ftell(file);	// Position of the cursor in the file
	size_t chunk_index;					// Index of the chunk we are considering
	size_t last_chunk_size = cells_count % CHUNK_SIZE;	// Size of the last spare chunk	
	
	chunks_index = (last_chunk_size == 0) ? cells_count / CHUNK_SIZE : cells_count / CHUNK_SIZE + 1; // Counts how many chnks
	
	// size_t chunk_size = CHUNK_SIZE;
	
	int16_t (*chunk_1)[3] = (int16_t (*)[3])malloc(sizeof(int16_t[3]) * CHUNK_SIZE);	// Memory for a cell
	int16_t (*chunk_2)[3] = (int16_t (*)[3])malloc(sizeof(int16_t[3]) * CHUNK_SIZE);  // Memory for a cell
	
	fseek(file, 0, SEEK_SET);

	

	for (size_t chunk_out = 0; chunk_out < chunks_index; chunk++) {	// Iterates the chunks
		
		size_t chunk_size1;				
		chunk_size1 = CHUNK_SIZE;			    			// Size of the first chunk 
		
		if (chunk_out == (chunk_index - 1)) {		// If it's the last spare chunk
			chunk_size1 = last_chunk_size;
		}
		
		cursor = chunk_out * CHUNK_SIZE * 24;		// Moves the cursor to the start of the next chunk
		fseek(file, cursor, SEEK_SET);

		load_chunk(chunk1, chunk_size1, file);
		
		self_distance(chunk1, chunk_size1, counter);

		for (size_t chunk_in = chunk_out + 1; chunk_in < chunks_count; chunk_in++) {
			size_t chunk_size2;				
			chunk_size2 = CHUNK_SIZE;								// Size of the first chunk 
			if (chunk_in == (chunk_index - 1)) {		// If it's the last spare chunk
				chunk_size2 = last_chunk_size;
			}

			load_chunk(chunk2, chunk_size2, file);
		
			double_distance(chunk1, chunk2, chunk_size1, chunk_size2, counter);
		}
	}

  for (size_t i = 0; i < 3465; ++i) {
  	if (counter[i] != 0) {
    	printf("%05.2f %lu\n", i/100.0, counter[i]);
    }
  }

	fclose(file);
	
	free(chunk_1);
	free(chunk_2);
	return 0;
}

