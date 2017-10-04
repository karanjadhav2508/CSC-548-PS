/******************************************************************************
 * * FILE: my_mpi.h
 * * DESCRIPTION:
 * *   Header file for custom MPI functions implementation.
 * * AUTHOR: Karan Jadhav
 * * LAST REVISED: 09/19/2017
 * * REVISED BY: Karan Jadhav
 *
 * Single Author info:
 * kjadhav Karan Jadhav
 *
 * ******************************************************************************/

#define MPI_COMM_WORLD		0
#define MPI_INFO_NULL		0
#define MPI_CHAR		1
#define MPI_STATUS_IGNORE	0


typedef int MPI_Comm;
typedef int MPI_Aint;
typedef int MPI_Info;
typedef int MPI_Datatype;
typedef int MPI_Status;

int MPI_Init(int *argc, char ***argv);
int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status);
int MPI_Comm_size(MPI_Comm comm, int *size);
int MPI_Comm_rank(MPI_Comm comm, int *rank);
int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr);
int MPI_Free_mem(void *base);
double MPI_Wtime(void);
int MPI_Finalize(void);
