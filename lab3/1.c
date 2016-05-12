#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <mpi.h>

int totalNumbers = 1000000;

// given input "size", returns an array of size "size" containing random values
int* generateRand(int size){
	int* array = (int*)malloc(sizeof(int) * size);
	int i;
	for (i=0;i<size;i++){
		array[i] = 1+(rand() % 10);
	}
	return array;
}

// prints the input array of size n
void printArray(int* array, int n){
	int i;
	for(i=0;i<n;i++){
		printf("%d\t", array[i] );
	}
	printf("\n\n");
}


int main(int argc, char** argv){

	srand(time(NULL));

	MPI_Init(NULL,NULL);
	
	int worldSize;
	MPI_Comm_size(MPI_COMM_WORLD,&worldSize);

	int rank;
	MPI_Comm_rank(MPI_COMM_WORLD,&rank);

	// scattered array is the local copy of the data for each process
	int scatteredArray[(totalNumbers/worldSize)];
	memset(scatteredArray,0, sizeof(scatteredArray));

	int i;
	int numbersInOneIteration = totalNumbers/10;
	int numbersPerProcess = numbersInOneIteration / worldSize;
	for(i=0;i<10;i++){
		int* array = NULL;
		if (rank==0) array = generateRand(numbersInOneIteration);

		MPI_Scatter(array,numbersPerProcess,MPI_INT,
					scatteredArray + i*(numbersPerProcess), numbersPerProcess, MPI_INT,
					0, MPI_COMM_WORLD);
		free(array);
	}

	MPI_Finalize();
}