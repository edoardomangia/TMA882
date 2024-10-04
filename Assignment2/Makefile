.PHONY: all clean run

all: distances3
		./distances3

distances: distances3.c
		gcc -o distances3 distances3.c -O3 -fopenmp -lm

clean:
		rm -f *.o distances3	

run: distances3
		./distances3

