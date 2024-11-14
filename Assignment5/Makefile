.PHONY: all clean run

all: diffusion

diffusion: diffusion.c
	mpicc -o diffusion diffusion.c -lm

clean:
	rm -f diffusion

run: diffusion
	mpirun -n 4 ./diffusion -n20 -d0.02

