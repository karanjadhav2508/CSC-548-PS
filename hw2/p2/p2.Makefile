CC=nvcc

all:
	$(CC) p2.cu -o p2 -O3 -lm -Wno-deprecated-gpu-targets

clean:
	rm -f p2
