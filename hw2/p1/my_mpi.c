/******************************************************************************
 * * FILE: my_mpi.c
 * * DESCRIPTION:
 * *   Custom implementation of MPI alternative, using shared memory.
 * * AUTHOR: Karan Jadhav
 * * LAST REVISED: 09/21/2017
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
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/time.h>
#include "my_mpi.h"

#define	ROOT	0
#define PORT	0

int proc_rank;
int numproc;
char *host;
char *host_file;
char *port_file;
char *recv_host_file;
char *recv_port_file;

char *receiver_host;
int receiver_port;

//Server declarations
int sockfd, newsockfd;
socklen_t clilen, len;
struct sockaddr_in serv_addr, cli_addr;

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

//Create host/port file names for the current process and the one it'll send to
void gen_host_port_file_name(char *rank) {
	//self host file
	host_file = malloc(sizeof(char) * 15);
	strcpy(host_file, rank);
	strcat(host_file, "_host.txt");
	//self port file
	port_file = malloc(sizeof(char) * 15);
        strcpy(port_file, rank);
        strcat(port_file, "_port.txt");
	//recv host file
	recv_host_file = malloc(sizeof(char) * 15);
        char recv_h[3];
        sprintf(recv_h, "%d", (proc_rank+1)%numproc);
        strcpy(recv_host_file, recv_h);
        strcat(recv_host_file, "_host.txt");
	//recv port file
	recv_port_file = malloc(sizeof(char) * 15);
        char recv_p[3];
        sprintf(recv_p, "%d", (proc_rank+1)%numproc);
        strcpy(recv_port_file, recv_p);
        strcat(recv_port_file, "_port.txt");
}

//write process's hostname to its host file
void write_host() {
	FILE *f = fopen(host_file, "w");
	if(f==NULL)
		error(host);
	fprintf(f, host);
	fclose(f);
}

//write the port on which the process will listen(recv) to its port file
void write_port(int port) {
	FILE *f = fopen(port_file, "w");
	if(f==NULL)
		error("writing port failed");
	fprintf(f, "%d", port);
	fclose(f);
}

//read the receiving process's hostname from its host file
void get_recv_host() {
	FILE *f = fopen(recv_host_file, "r");
	if(f==NULL)
		error(recv_host_file);
	char *recv_host = malloc(sizeof(char)*5);
	if(recv_host==NULL)
		error("recv_host malloc failed");
	if(fgets(recv_host, 5, f)==NULL) {
		fclose(f);
		error("null returned on fgets get_recv_host()");
	}
	receiver_host = recv_host;
	fclose(f);
	remove(recv_host_file);
}

//read the receiving process's port from its host file
void get_recv_port() {
	FILE *f = fopen(recv_port_file, "r");
	if(f==NULL)
		error(recv_port_file);
	char *recv_port = malloc(sizeof(char)*7);
	if(recv_port==NULL)
		error("recv_port malloc failed");
	if(fgets(recv_port, 7, f)==NULL) {
		fclose(f);
		free(recv_port);
		error("null return on fgets get_recv_port()");
	}
	receiver_port = atoi(recv_port);
	free(recv_port);
	fclose(f);
	remove(recv_port_file);
}

void get_recv_host_port() {
	get_recv_host();
	get_recv_port();
}


//setup the listening socket for the process to listen(recv) on
void setup_server() {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
                error("ERROR opening socket");
        bzero((char *) &serv_addr, sizeof(serv_addr));

        serv_addr.sin_family = AF_INET;
        serv_addr.sin_addr.s_addr = INADDR_ANY;
        serv_addr.sin_port = htons(PORT);

        while(1) {
                if (bind(sockfd, (struct sockaddr *) &serv_addr,
                    sizeof(serv_addr)) < 0) {
                        sleep(2);
                }
                else {
                        break;
                }
        }

        listen(sockfd,1);
        clilen = sizeof(cli_addr);
	struct sockaddr_in sin;
	int len;
	len = sizeof(sin);
        if(getsockname(sockfd, (struct sockaddr *)&sin, &len) == -1)
                error("unable to get socketname\n");
        write_port(ntohs(sin.sin_port));
}

/*
 * Set process's rank, number of processes and hostname
 * Generate process's and its receiving process's host/port file name
 * write process's host to its host file
 * setup process's listening socket
 * sleep for 3 seconds(experimental) to allow all processes to finish Init()
 */
int MPI_Init(int *argc, char ***argv) {
	proc_rank = atoi((*argv)[1]);
	numproc = atoi((*argv)[2]);
	host = (*argv)[3];
	gen_host_port_file_name((*argv)[1]);
	write_host();
	setup_server();
	sleep(3);
	get_recv_host_port();
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {

	int sockfd_cli;
	struct sockaddr_in serv_addr_cli;
	struct hostent *server;

	sockfd_cli = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd_cli < 0)
                error("ERROR opening socket");
        server = gethostbyname(receiver_host);
        if (server == NULL) {
                fprintf(stderr,"ERROR, no such host\n");
                exit(0);
        }
        bzero((char *) &serv_addr_cli, sizeof(serv_addr_cli));
        serv_addr_cli.sin_family = AF_INET;
        bcopy((char *)server->h_addr,
                (char *)&serv_addr_cli.sin_addr.s_addr,
                server->h_length);
        serv_addr_cli.sin_port = htons(receiver_port);
	
	for(int i=0; i<5;i++) {
		if(connect(sockfd_cli,(struct sockaddr *) &serv_addr_cli,sizeof(serv_addr_cli)) < 0) {
			if(i==4)
				error("failed to connect");
			sleep(1);
		} else {
			break;
		}
	}

	int total = 0;
	while(total < count) {
		total += write(sockfd_cli,buf,count);
	}
    	close(sockfd_cli);
    	return 0;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) {

     	newsockfd = accept(sockfd,
                 (struct sockaddr *) &cli_addr,
                 &clilen);
     	if (newsockfd < 0)
         	error("ERROR on accept");

	int total = 0;
	while(total < count) {
		total += read(newsockfd,buf,count);
	}
     	close(newsockfd);
     	return 0;
}

int MPI_Comm_size(MPI_Comm comm, int *size) {
	*size = numproc;
}

int MPI_Comm_rank(MPI_Comm comm, int *rank) {
	*rank = proc_rank;
}

int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, void *baseptr) {
	*((void**)baseptr) = malloc(size);
}

int MPI_Free_mem(void *base) {
	free(base);
}

// Close listening socket and free allocated string mems
int MPI_Finalize(void) {
	close(sockfd);
	free(host_file);
	free(port_file);
	free(recv_host_file);
	free(recv_port_file);	
	free(receiver_host);
}

double MPI_Wtime() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (double)(tv.tv_sec*1e6 + tv.tv_usec);
}
