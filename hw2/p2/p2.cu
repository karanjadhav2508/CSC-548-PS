/*
* Single Author info: kjadhav Karan Jadhav
* Group info: NA
* Last modified: 9/17/2017	
*/

#include <stdio.h>
#include <math.h>
#include "mytime.h"

#define THREADS 512
#define MAX_BLOCKS 64

// GPU kernel, we know: THREADS == blockDim.x
__global__ void integrate(int *n, int *blocks, double *gsum) {
  const unsigned int bid = blockDim.x * blockIdx.x + threadIdx.x;
  const unsigned int tid = threadIdx.x;
  double sum;
  int i, arr_len;
  __shared__ double ssum[THREADS];

  //Leibniz implementation
  sum = 0.0;
  for (i = bid; i < *n; i += blockDim.x * *blocks) {
    sum += pow(-1.0,(double)i) / (double)(2*i + 1);
  }
  ssum[tid] = sum * 4.0;
  // block reduction
  __syncthreads();
  //Keeping track of the size of the array we are reducing
  arr_len = blockDim.x;
  for (i = blockDim.x / 2; i > 0; i >>= 1) { /* per block */
    if (tid < i)
      ssum[tid] += ssum[tid + i];
      //handling cases where threads are not a power of two
      //if there is an element(in case of odd sized array) after the one that the last thread strode to, then add that element to the last thread index
      if(tid+1==i && tid+i+1<arr_len) {
	ssum[tid] += ssum[tid+i+1];
      }
    //continue keeping track of the input array on the next reduction iteration
    arr_len = i;
    __syncthreads();
  }
  if (tid == 0)
    gsum[blockIdx.x] = ssum[tid];
}

// number of threads must be a power of 2
__global__ static void global_reduce(int *n, int *blocks, double *gsum)
{
    __shared__ double ssum[THREADS];
    const unsigned int tid = threadIdx.x;
    unsigned int i, arr_len;

    ssum[tid] = gsum[tid];
    __syncthreads();
    arr_len = blockDim.x;
    for (i = blockDim.x / 2; i > 0; i >>= 1) { /* per block */
        if (tid < i)
           ssum[tid] += ssum[tid + i];
           //handling the case where number of blocks is not a power of 2(in global reduction, number of threads is the original number of blocks)
	   //same logic as block reduction earlier
	   if(tid+1==i && tid+i+1<arr_len) {
	      ssum[tid] += ssum[tid+i+1];
	   }
        arr_len = i;
        __syncthreads();
    }
    if (tid == 0)
      gsum[tid] = ssum[tid];
}

int main(int argc, char *argv[]) {
  int n, blocks;
  int *n_d, *blocks_d; // device copy
  double PI25DT = 3.141592653589793238462643;
  double pi;
  double *mypi_d; // device copy of pi
  struct timeval startwtime, endwtime, diffwtime;
  
  // Allocate memory on GPU
  cudaMalloc( (void **) &n_d, sizeof(int) * 1 );
  cudaMalloc( (void **) &blocks_d, sizeof(int) * 1 );
  cudaMalloc( (void **) &mypi_d, sizeof(double) * THREADS * MAX_BLOCKS );

  while (1) {
    printf("Enter the number of intervals: (0 quits) ");fflush(stdout);
    scanf("%d",&n);
    printf("Enter the number of blocks: (<=%d) ", MAX_BLOCKS);fflush(stdout);
    scanf("%d",&blocks);

    gettimeofday(&startwtime, NULL);
    if (n == 0 || blocks > MAX_BLOCKS)
      break;

    // copy from CPU to GPU
    cudaMemcpy( n_d, &n, sizeof(int) * 1, cudaMemcpyHostToDevice );
    cudaMemcpy( blocks_d, &blocks, sizeof(int) * 1, cudaMemcpyHostToDevice );

    integrate<<< blocks, THREADS >>>(n_d, blocks_d, mypi_d);
    if (blocks > 1)
      global_reduce<<< 1, blocks >>>(n_d, blocks_d, mypi_d);
    // copy back from GPU to CPU
    cudaMemcpy( &pi, mypi_d, sizeof(double) * 1, cudaMemcpyDeviceToHost );

    gettimeofday(&endwtime, NULL);
    MINUS_UTIME(diffwtime, endwtime, startwtime);
    printf("pi is approximately %.16f, Error is %.16f\n",
	   pi, fabs(pi - PI25DT));
    printf("wall clock time = %d.%06d\n",
	   diffwtime.tv_sec, diffwtime.tv_usec);
  }

  // free GPU memory
  cudaFree(n_d);
  cudaFree(blocks_d);
  cudaFree(mypi_d);

  return 0;
}
