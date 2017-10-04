CC=mpicc

p1make: p1.o
	$(CC) -lm -O3 -o p1 p1.c
