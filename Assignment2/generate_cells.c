#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_POINTS 100000
#define FILENAME "cells_long"
#define COORD_MIN -10.0
#define COORD_MAX 10.0

// Function to generate a random double between min and max
double random_coordinate() {
    return COORD_MIN + ((double)rand() / (double)RAND_MAX) * (COORD_MAX - COORD_MIN);
}

int main() {
    FILE *file = fopen(FILENAME, "w");
    if (!file) {
        printf("Error opening file.\n");
        return 1;
    }

    srand(time(NULL));

    for (int i = 0; i < NUM_POINTS; i++) {
        double x = random_coordinate();
        double y = random_coordinate();
        double z = random_coordinate();

        fprintf(file, "%+07.3f %+07.3f %+07.3f\n", x, y, z);
    }

    fclose(file);

    printf("Coordinates generated and saved to %s\n", FILENAME);
    return 0;
}

