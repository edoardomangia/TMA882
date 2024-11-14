.PHONY: all clean run

all: diffusion
	./diffusion

diffusion: diffusion.c
	gcc -o diffusion diffusion.c -O2 -lOpenCL -lm

clean:
	rm -f *.o diffusion

run: diffusion
	./diffusion

