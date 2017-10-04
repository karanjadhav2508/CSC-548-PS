/******************************************************************************
 * * FILE: mpi_hello.c
 * * DESCRIPTION:
 * *   MPI program that passes message from one process to the next, ending with the original sender.
 * * AUTHOR: Karan Jadhav
 * * LAST REVISED: 08/19/2017
 * * REVISED BY: Karan Jadhav
 *
 * Single Author info:
 * kjadhav Karan Jadhav
 *
 * ******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include "my_mpi.h"

#define		NUM_ITERS	10

int main (int argc, char *argv[])
{

  /* process information */
  int num_proc, rank;

  /* initialize MPI */
  MPI_Init(&argc, &argv);

  /* get the number of procs in the comm */
  MPI_Comm_size(MPI_COMM_WORLD, &num_proc);

  /* get my rank in the comm */
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  char *data;
  int num_chars;

  //iterate over all message sizes
  for(num_chars=32; num_chars<=2097152; num_chars=num_chars*2) {
    MPI_Alloc_mem(num_chars+1, MPI_INFO_NULL, &data);
    // First process sets the data/message to be sent
    if(rank==0) {
      memset(data,'x', num_chars);
      data[num_chars] = '\0';
    }
    //array to hold rtt values(except first iteration/message exchange)
    double rtt[NUM_ITERS-1];
    int i;
    double start,end;
    // measure rtt 10 times
    for(i=0; i<NUM_ITERS; i++) {
      if(rank==0 && i!=0) {
        start = MPI_Wtime();
      }
      //all processes except root wait to receive the message from the previous ranked process
      if(rank!=0) {
        MPI_Recv(data, num_chars+1, MPI_CHAR, rank-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      }
      //a process sends the message to the next one
      MPI_Send(data, num_chars+1, MPI_CHAR, (rank+1)%num_proc, 0, MPI_COMM_WORLD);
      //finally, the root process receives the message from the last process
      if(rank==0) {
        MPI_Recv(data, num_chars+1, MPI_CHAR, num_proc-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        if(i!=0) {
          end = MPI_Wtime();
          rtt[i-1] = (double)(end-start) / 1000.0;
        }
      }
    }
    MPI_Free_mem(data);
    //calculate avg_rtt and std_dev
    if(rank==0) {
      double avg_rtt = 0.0;
      for(i=0; i<NUM_ITERS-1; i++) {
        avg_rtt += rtt[i];
      }
      avg_rtt = avg_rtt/((double)NUM_ITERS-1);
      double std_dev = 0.0;
      for(i=0; i<NUM_ITERS-1; i++) {
        std_dev += fabs(avg_rtt-rtt[i]);
      }
      std_dev = sqrt(std_dev/(double)(NUM_ITERS-1));
      printf("%d %e %e\n", num_chars, avg_rtt, std_dev);
    }
  }

  /* graceful exit */
  MPI_Finalize();

}
