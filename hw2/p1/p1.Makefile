#######################
# Single Author info:
# kjadhav Karan Jadhav
#######################

all:
	mpicc -lm -O3 -o my_rtt my_rtt.c my_mpi.c
clean:
	rm -f my_rtt
