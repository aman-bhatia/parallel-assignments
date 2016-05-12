#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <mpi.h>

enum order {asc,desc};
#define comp(odr,x,y) ((odr) == asc ? ((x)<=(y)) : ((x)>(y)) )

int cmpasc (const void * a, const void * b) {
	return ( *(int*)a > *(int*)b );
}

int cmpdesc (const void * a, const void * b) {
	return ( *(int*)a <= *(int*)b );
}

int totalNumbers = (1<<20);
int worldSize;

int* generateRand(int size){
	int* array = (int*)malloc(sizeof(int) * size);
	int i;
	for (i=0;i<size;i++){
		array[i] = 1+(rand() % 10);
	}
	return array;
}

void printArray(int* array, int n){
	int i;
	for(i=0;i<n;i++){
		printf("%d, ", array[i] );
	}
	printf("\n\n");
}

void bitonicMerge(int* array1, int* array2, int n, enum order sortingOrder, int sameOrder){
	int k;
	for (k=0;k<n;k++){
		int index2 = (sameOrder) ? (n-k-1) : (k);
		if (!(comp(sortingOrder,array1[k],array2[index2]))) {
			int temp = array1[k];
			array1[k] = array2[index2];
			array2[index2] = temp;
		}
	}
}

void verify(int* a, int* b, int n){
	int i;
	for(i=0;i<n;i++){
		if(a[i] != b[i]){
			printf("Fail\n");
			break;
		}
	}
	printf("Pass\n");
}

void getLocalData(int rank, int* localArray, int localArraySize){
	memset(localArray,0, sizeof(localArray));
	int numIter = 1;
	int numbersInOneIteration = totalNumbers/numIter;
	int numbersPerProcess = numbersInOneIteration / worldSize;
	
	int i;
	for(i=0;i<numIter;i++){
		int* array = NULL;
		if (rank==0){
			array = generateRand(numbersInOneIteration);
		}

		MPI_Scatter(array,numbersPerProcess,MPI_INT,
					localArray + i*(numbersPerProcess), numbersPerProcess, MPI_INT,
					0, MPI_COMM_WORLD);
		free(array);
	}
}

int main(int argc, char** argv){
	srand(time(NULL));
	
	MPI_Init(NULL,NULL);
	
	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);
	MPI_Comm_size(MPI_COMM_WORLD,&worldSize);
	
	int localArraySize = (totalNumbers/worldSize);
	int* localArray = (int*)malloc(sizeof(int) * localArraySize);
	getLocalData(rank,localArray,localArraySize);

	double time_start;
	if(rank==0) time_start = MPI_Wtime();

	qsort(localArray, localArraySize, sizeof(int), (rank%2==0) ? (cmpasc) : (cmpdesc));
	
	MPI_Request req1;
	MPI_Request req2;
	MPI_Status stat1;
	MPI_Status stat2;
	enum order odr;
	int odr_mask = 2;
	int i,j;
	int* partnerArray = (int*) malloc(sizeof(int) * localArraySize);
	for (i=0;i<log2(worldSize);i++,odr_mask=odr_mask<<1){
		for (j=0;j<=i;j++){
			int partner_rank = rank ^ (1<<(i-j));
			((rank & odr_mask) == 0) ? (odr = asc) : (odr = desc);
			if (rank < partner_rank){
				MPI_Irecv(partnerArray,localArraySize,MPI_INT, partner_rank,0,MPI_COMM_WORLD,&req1);
				MPI_Isend(localArray, localArraySize, MPI_INT, partner_rank,0,MPI_COMM_WORLD,&req2);
				MPI_Wait(&req1,&stat1);
				MPI_Wait(&req2,&stat2);
				bitonicMerge(localArray, partnerArray,localArraySize,odr,(j>0) ? (1) : (0));
			} else {
				MPI_Isend(localArray, localArraySize, MPI_INT, partner_rank,0,MPI_COMM_WORLD,&req1);
				MPI_Irecv(partnerArray,localArraySize,MPI_INT, partner_rank,0,MPI_COMM_WORLD,&req2);
				MPI_Wait(&req1,&stat1);
				MPI_Wait(&req2,&stat2);
				bitonicMerge(partnerArray, localArray,localArraySize,odr,(j>0) ? (1) : (0));
			}
			qsort(localArray, localArraySize, sizeof(int),(odr==asc) ? (cmpasc) : (cmpdesc));
		}
	}

	free(localArray);
	free(partnerArray);

	MPI_Barrier(MPI_COMM_WORLD);
	if(rank==0) printf("Time taken : %f\n", MPI_Wtime() - time_start);

	MPI_Finalize();
}