/* Single Author info:
 * kjadhav Karan Jadhav 
 */

#include<stdio.h>
#include<string.h>
#include<stdlib.h>
#include<dirent.h>
#include<math.h>
#include<stddef.h>
#include "mpi.h"

#define MAX_WORDS_IN_CORPUS 32
#define MAX_FILEPATH_LENGTH 16
#define MAX_WORD_LENGTH 16
#define MAX_DOCUMENT_NAME_LENGTH 8
#define MAX_STRING_LENGTH 64

typedef char word_document_str[MAX_STRING_LENGTH];

typedef struct o {
	char word[32];
	char document[8];
	int wordCount;
	int docSize;
	int numDocs;
	int numDocsWithWord;
} obj;

typedef struct w {
	char word[32];
	int numDocsWithWord;
	int currDoc;
} u_w;

static int myCompare (const void * a, const void * b)
{
    return strcmp (a, b);
}

int main(int argc , char *argv[]){

	//MPI custom data type for struct obj
	MPI_Datatype temp_obj, MPI_obj;
	MPI_Datatype type_obj[6] = {MPI_CHAR, MPI_CHAR, MPI_INT, MPI_INT, MPI_INT, MPI_INT};
	int blocklen_obj[6] = {32, 8, 1, 1, 1, 1};
	MPI_Aint disp_obj[6] = {offsetof(obj,word), offsetof(obj,document), offsetof(obj,wordCount), offsetof(obj,docSize), offsetof(obj,numDocs), offsetof(obj,numDocsWithWord)};
	MPI_Aint lb_obj, extent_obj;

	//MPI custom data type for struct u_w
	MPI_Datatype temp_u_w, MPI_u_w;
	MPI_Datatype type_u_w[3] = {MPI_CHAR, MPI_INT, MPI_INT};
	int blocklen_u_w[3] = {32, 1, 1};
	MPI_Aint disp_u_w[3] = {offsetof(u_w, word), offsetof(u_w, numDocsWithWord), offsetof(u_w, currDoc)};
	MPI_Aint lb_u_w, extent_u_w;

	//Standard MPI information : number of processes, rank etc.
	/* process information */
	int num_proc, rank;
	/* initialize MPI */
	MPI_Init(&argc, &argv);
	/* get the number of procs in the comm */
	MPI_Comm_size(MPI_COMM_WORLD, &num_proc);
	/* get my rank in the comm */
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	//num of workers(excluding 0)
	int numWorkers = num_proc - 1;

	DIR* files;
	struct dirent* file;
	int i,j;
	int numDocs, docSize, contains;
	char filename[MAX_FILEPATH_LENGTH], word[MAX_WORD_LENGTH], document[MAX_DOCUMENT_NAME_LENGTH];
	
	// Will hold all TFIDF objects for all documents
	obj TFIDF[MAX_WORDS_IN_CORPUS];
	int TF_idx = 0;
	
	// Will hold all unique words in the corpus and the number of documents with that word
	u_w unique_words[MAX_WORDS_IN_CORPUS];
	int uw_idx = 0;
	
	// Will hold the final strings that will be printed out
	word_document_str strings[MAX_WORDS_IN_CORPUS];
	
	//Creating MPI custom datatype based off u_w
	MPI_Type_create_struct(3, blocklen_u_w, disp_u_w, type_u_w, &temp_u_w);
	MPI_Type_get_extent(temp_u_w, &lb_u_w, &extent_u_w);
	MPI_Type_create_resized(temp_u_w, lb_u_w, extent_u_w, &MPI_u_w);
	MPI_Type_commit(&MPI_u_w);

	//Creating MPI custom data type based off obj
	MPI_Type_create_struct(6, blocklen_obj, disp_obj, type_obj, &temp_obj);
	MPI_Type_get_extent(temp_obj, &lb_obj, &extent_obj);
        MPI_Type_create_resized(temp_obj, lb_obj, extent_obj, &MPI_obj);
	MPI_Type_commit(&MPI_obj);

	//Rank 0 reads directory and counts number of documents
	if(rank==0) {
		//Count numDocs
		if((files = opendir("input")) == NULL){
			printf("Directory failed to open\n");
			exit(1);
		}
		while((file = readdir(files))!= NULL){
			// On linux/Unix we don't want current and parent directories
			if(!strcmp(file->d_name, "."))	 continue;
			if(!strcmp(file->d_name, "..")) continue;
			numDocs++;
		}
	}

	//Broadcast from 0 to all other ranks(workers) the number of documents
	MPI_Bcast(&numDocs, 1, MPI_INT, 0, MPI_COMM_WORLD);
	if(numDocs < numWorkers) {
		if(rank==0)
			printf("Number of docs lesser than number of workers.\n");
		exit(0);
	}

	//All other ranks(workers) read a document corresponding to their rank and do so iteratively over all documents, taking a stride=number of workers, to avoid clashes
	//Do the standard TFIDF, unique_words calculation that was done in the serial program
	if(rank != 0) {
		for(i = rank; i <= numDocs; i += numWorkers) {
			sprintf(document, "doc%d", i);
			sprintf(filename,"input/%s",document);
			FILE* fp = fopen(filename, "r");
			if(fp == NULL){
                        	printf("Error Opening File: %s\n", filename);
                        	exit(0);
                	}
		
			// Get the document size
			docSize = 0;
			while((fscanf(fp,"%s",word))!= EOF)
				docSize++;
			
			// For each word in the document
			fseek(fp, 0, SEEK_SET);
			while((fscanf(fp,"%s",word))!= EOF){
                        	contains = 0;

				// If TFIDF array already contains the word@document, just increment wordCount and break
				for(j=0; j<TF_idx; j++) {
                                	if(!strcmp(TFIDF[j].word, word) && !strcmp(TFIDF[j].document, document)){
                                        	contains = 1;
                                        	TFIDF[j].wordCount++;
                                        	break;
                                	}
                        	}
				//If TFIDF array does not contain it, make a new one with wordCount=1
				if(!contains) {
                                	strcpy(TFIDF[TF_idx].word, word);
                                	strcpy(TFIDF[TF_idx].document, document);
                                	TFIDF[TF_idx].wordCount = 1;
                                	TFIDF[TF_idx].docSize = docSize;
                                	TFIDF[TF_idx].numDocs = numDocs;
                                	TF_idx++;
                        	}

				contains = 0;
                        	// If unique_words array already contains the word, just increment numDocsWithWord
                        	for(j=0; j<uw_idx; j++) {
                                	if(!strcmp(unique_words[j].word, word)){
                                        	contains = 1;
                                        	if(unique_words[j].currDoc != i) {
                                                	unique_words[j].numDocsWithWord++;
                                                	unique_words[j].currDoc = i;
                                        	}
                                        	break;
                                	}
                        	}
				// If unique_words array does not contain it, make a new one with numDocsWithWord=1
                        	if(!contains) {
                                	strcpy(unique_words[uw_idx].word, word);
                                	unique_words[uw_idx].numDocsWithWord = 1;
                               		unique_words[uw_idx].currDoc = i;
                                	uw_idx++;
                        	}
			}
			fclose(fp);
		}
		//Communicate count of TFIDF that has data followed by the actual data
		MPI_Send(&TF_idx, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Send(&TFIDF, TF_idx, MPI_obj, 0, 0, MPI_COMM_WORLD);
		//Communicate count of unique_words that has data followed by the actual data
		MPI_Send(&uw_idx, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
		MPI_Send(&unique_words, uw_idx, MPI_u_w, 0, 0, MPI_COMM_WORLD);
		
	}
	// Rank 0, received the count of followed by the actual data of TFIDF and unique_words from each process
	else {
		int worker_TF_idx, worker_uw_idx;
		obj worker_TFIDF[MAX_WORDS_IN_CORPUS];
		u_w worker_unique_words[MAX_WORDS_IN_CORPUS];
		for(i=1; i<num_proc; i++) {
			MPI_Recv(&worker_TF_idx, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(&worker_TFIDF, worker_TF_idx, MPI_obj, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(&worker_uw_idx, 1, MPI_INT, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			MPI_Recv(&worker_unique_words, worker_uw_idx, MPI_u_w, i, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
			//TFIDF : data is simple put into 0's TFIDF array
			for(j=0; j<worker_TF_idx; j++) {
				strcpy(TFIDF[TF_idx].word, worker_TFIDF[j].word);
                                strcpy(TFIDF[TF_idx].document, worker_TFIDF[j].document);
                                TFIDF[TF_idx].wordCount = worker_TFIDF[j].wordCount;
                                TFIDF[TF_idx].docSize = worker_TFIDF[j].docSize;
                                TFIDF[TF_idx].numDocs = worker_TFIDF[j].numDocs;
                                TF_idx++;	
			}
			//unique_word : each element received from each process is checked if it exists in 0's unique_words array and actioned accordingly
			for(j=0; j<worker_uw_idx; j++) {
				contains = 0;
				for(int k=0; k<uw_idx; k++) {
                                	if(!strcmp(unique_words[k].word, worker_unique_words[j].word)){
                                        	contains = 1;
                                                unique_words[k].numDocsWithWord += worker_unique_words[j].numDocsWithWord;
                                                unique_words[k].numDocsWithWord += worker_unique_words[j].numDocsWithWord;
                                        	break;
                                	}
                        	}
				if(!contains) {
					strcpy(unique_words[uw_idx].word, worker_unique_words[j].word);
                                	unique_words[uw_idx].numDocsWithWord = worker_unique_words[j].numDocsWithWord;
                                	uw_idx++;
				}				
			}
		}

		printf("-------------TF Job-------------\n");
        	for(j=0; j<TF_idx; j++)
                	printf("%s@%s\t%d/%d\n", TFIDF[j].word, TFIDF[j].document, TFIDF[j].wordCount, TFIDF[j].docSize);

		for(i=0; i<TF_idx; i++) {
                	for(j=0; j<uw_idx; j++) {
                        	if(!strcmp(TFIDF[i].word, unique_words[j].word)) {
                                	TFIDF[i].numDocsWithWord = unique_words[j].numDocsWithWord;
                                	break;
                        	}
                	}
        	}

		printf("------------IDF Job-------------\n");
        	for(j=0; j<TF_idx; j++)
               		printf("%s@%s\t%d/%d\n", TFIDF[j].word, TFIDF[j].document, TFIDF[j].numDocs, TFIDF[j].numDocsWithWord);

		for(j=0; j<TF_idx; j++) {
                	double TF = 1.0 * TFIDF[j].wordCount / TFIDF[j].docSize;
                	double IDF = log(1.0 * TFIDF[j].numDocs / TFIDF[j].numDocsWithWord);
                	double TFIDF_value = TF * IDF;
                	sprintf(strings[j], "%s@%s\t%.16f", TFIDF[j].document, TFIDF[j].word, TFIDF_value);
        	}

		qsort(strings, TF_idx, sizeof(char)*MAX_STRING_LENGTH, myCompare);
        	FILE* fp = fopen("output.txt", "w");
        	if(fp == NULL){
                	printf("Error Opening File: output.txt\n");
                	exit(0);
        	}
        	for(i=0; i<TF_idx; i++)
                	fprintf(fp, "%s\n", strings[i]);
        	fclose(fp);

	}

	MPI_Finalize();

	return 0;
}
