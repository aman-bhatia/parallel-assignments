/*2013CS10207_2013CS50294*/

#include <stdio.h>
#include <stdbool.h>
#include <omp.h>
#include "sudoku.h"

extern int thread_count;

bool result_found = false;

int*** allocateArray(int n){
	int ***a = malloc(sizeof(int**)*n);
	int i,j;
	for(i=0;i<n;i++){
		a[i] = malloc(sizeof(int*)*n);
		for(j=0;j<n;j++){
			a[i][j] = malloc(sizeof(int)*n);
		}
	}
	return a;
}

int** allocate2DArray(int n){
	int **a = malloc(sizeof(int*)*n);
	int i;
	for(i=0;i<n;i++){
		a[i] = malloc(sizeof(int)*n);
	}
	return a;
}

void freeArray(int ***a,int n){
	int i,j;
	for(i=0;i<n;i++){
		for(j=0;j<n;j++){
			free(a[i][j]);
		}
		free(a[i]);
	}
	free(a);
}

void free2DArray(int **a,int n){
	int i;
	for(i=0;i<n;i++){
		free(a[i]);
	}
	free(a);
}

void copyArray(int ***src, int ***dest, int n){
	int i,j,k;
	for(i=0;i<n;i++){
		for(j=0;j<n;j++){
			for(k=0;k<n;k++){
				dest[i][j][k] = src[i][j][k];
			}
		}
	}
}

void copy2DArray(int **src,int **dest,int n){
	int i,j;
	for(i=0;i<n;i++){
		for(j=0;j<n;j++){
			dest[i][j] = src[i][j];
		}
	}
}

bool checkNumInRow(int **grid,int row,int num){
	int col;
	for(col=0;col<SIZE;col++){
		if(grid[row][col]==num){
			return true;
		}
	}
	return false;
}

bool checkNumInCol(int **grid,int col,int num){
	int row;
	for(row=0;row<SIZE;row++){
		if(grid[row][col]==num){
			return true;
		}
	}
	return false;
}

bool checkNumInBox(int **grid,int boxStartRow,int boxStartCol,int num){
	int row,col;
	for(row=0;row<MINIGRIDSIZE;row++){
		for(col=0;col<MINIGRIDSIZE;col++){
			if(grid[boxStartRow+row][boxStartCol+col]==num){
				return true;
			}
		}
	}
	return false;
}


bool isValidNum(int **grid,int row,int col,int num){
	bool rowCheck = checkNumInRow(grid,row,num);
	bool colCheck = checkNumInCol(grid,col,num);
	bool boxCheck = checkNumInBox(grid,row-row%MINIGRIDSIZE,col-col%MINIGRIDSIZE,num);
	return (!rowCheck && !colCheck && !boxCheck);
}

bool getEmptyCell(int **grid,int* row,int* col){
	int i,j;
	for(i=0;i<SIZE;i++){
		for(j=0;j<SIZE;j++){
			if(grid[i][j]==0){ 	
				*row = i;
				*col = j;
				return true;
			}
		}
	}
	return false;
}

bool getMinimumEmptyCell(int **grid, int*** possibleValues, int* row,int* col){
	int i,j,k,count;
	int minPossibleValue = SIZE+1;
	bool flag=false;
	
	for(i=0;i<SIZE;i++){
		for(j=0;j<SIZE;j++){
			if(grid[i][j]==0){
				count=0;
				for(k=0;k<SIZE;k++){
					if(possibleValues[i][j][k]==1){
						count++;
					}
				}
				if(count<minPossibleValue){
					minPossibleValue = count;
					*row = i;
					*col = j;
					flag=true;
				} 	
			}
		}
	}
	return flag;
}

void initialisePossibleValues(int **grid,int*** possibleValues){
	int row,col,num;
	for(row=0;row<SIZE;row++){
		for(col=0;col<SIZE;col++){
			if(grid[row][col]==0){
				for(num=1;num<=SIZE;num++){
					if(isValidNum(grid,row,col,num)){
						possibleValues[row][col][num-1] = 1;
					}else{
						possibleValues[row][col][num-1] = 0;
					}
				}	
			}else{
				for(num=1;num<=SIZE;num++){
					if(num==grid[row][col]){
						possibleValues[row][col][num-1] = 1;
					}else{
						possibleValues[row][col][num-1] = 0;	
					}
				}	
			}
		}
	}
}

void updatePossibleValues(int **grid,int*** possibleValues, int row,int col){
	
	int i,j,num;

	// update row
	for(i=0;i<SIZE;i++){
		if(grid[row][i]==0){
			for(num=1;num<=SIZE;num++){
				if(isValidNum(grid,row,i,num)){
					possibleValues[row][i][num-1] = 1;
				}else{
					possibleValues[row][i][num-1] = 0;
				}
			}
		}else{
			for(num=1;num<=SIZE;num++){
				if(num==grid[row][i]){
					possibleValues[row][i][num-1] = 1;
				}else{
					possibleValues[row][i][num-1] = 0;	
				}
			}	
		}
	}

	// update column
	for(i=0;i<SIZE;i++){
		if(grid[i][col]==0){
			for(num=1;num<=SIZE;num++){
				if(isValidNum(grid,i,col,num)){
					possibleValues[i][col][num-1] = 1;
				}else{
					possibleValues[i][col][num-1] = 0;
				}
			}
		}else{
			for(num=1;num<=SIZE;num++){
				if(num==grid[i][col]){
					possibleValues[i][col][num-1] = 1;
				}else{
					possibleValues[i][col][num-1] = 0;	
				}
			}	
		}
	}

	// uppdate grid
	int boxStartRow = row - row%MINIGRIDSIZE;
	int boxStartCol = col -  col%MINIGRIDSIZE;
	for(i=boxStartRow;i<boxStartRow+MINIGRIDSIZE;i++){
		for(j=boxStartCol;j<boxStartCol+MINIGRIDSIZE;j++){
			if(grid[i][j]==0){
				for(num=1;num<=SIZE;num++){
					if(isValidNum(grid,i,j,num)){
						possibleValues[i][j][num-1] = 1;
					}else{
						possibleValues[i][j][num-1] = 0;
					}
				}
			}else{
				for(num=1;num<=SIZE;num++){
					if(num==grid[i][j]){
						possibleValues[i][j][num-1] = 1;
					}else{
						possibleValues[i][j][num-1] = 0;	
					}
				}	
			}
		}
	}
}

bool loneRangerOnRow(int **grid, int*** possibleValues,int row){
	
	int col,num;
	int count,index;
	
	for(num=1;num<=SIZE;num++){
		count=0;
		for(col=0;col<SIZE;col++){
			if(possibleValues[row][col][num-1]==1){
				count++;
				index = col;
			}
			if (count > 1)
				break;
		}
		if(count==1){
			if(grid[row][index]==0){
				grid[row][index]=num;
				updatePossibleValues(grid,possibleValues, row,index);
				return true;	
			}
		}
	}

	return false;
}

bool loneRangerOnCol(int **grid, int*** possibleValues,int col){

	int row,num;
	int count,index;
	
	for(num=1;num<=SIZE;num++){
		count=0;
		for(row=0;row<SIZE;row++){
			if(possibleValues[row][col][num-1]==1){
				count++;
				index = row;
			}
			if (count > 1)
				break;
		}
		if(count==1){
			if(grid[index][col]==0){
				grid[index][col]=num;
				updatePossibleValues(grid,possibleValues, index,col);
				return true;
			}
		}
	}

	return false;
}

bool loneRangerOnBox(int **grid, int*** possibleValues,int boxStartRow,int boxStartCol){
	
	int row,col,num;
	int count,rowIndex,colIndex;
	
	for(num=1;num<=SIZE;num++){
		count=0;
		for(row=0;row<MINIGRIDSIZE;row++){
			for(col=0;col<MINIGRIDSIZE;col++){
				if(possibleValues[row+boxStartRow][col+boxStartCol][num-1]==1){
					count++;
					rowIndex = row;
					colIndex = col;
				}
				if (count > 1)
					break;
			}
			if (count > 1)
				break;	
		}
		if(count==1){
			if(grid[rowIndex+boxStartRow][colIndex+boxStartCol]==0){
				grid[rowIndex+boxStartRow][colIndex+boxStartCol]=num;
				updatePossibleValues(grid,possibleValues, rowIndex+boxStartRow,colIndex+boxStartCol);
				return true;
			}
		}
	}

	return false;
}

bool loneRanger(int **grid ,int*** possibleValues){

	bool toReturn = false;

	int row;
	for(row=0;row<SIZE;row++){
		if(loneRangerOnRow(grid,possibleValues, row)){
			toReturn = true;
		}
	}

	int col;
	for(col=0;col<SIZE;col++){
		if(loneRangerOnCol(grid,possibleValues, col)){
			toReturn = true;
		}
	}

	for(row=0;row<SIZE;row=row+MINIGRIDSIZE){
		for(col=0;col<SIZE;col=col+MINIGRIDSIZE){
			if(loneRangerOnBox(grid,possibleValues, row,col)){
				toReturn = true;
			}
		}
	}
	
	return toReturn;
}


bool eliminationOnRow(int **grid, int*** possibleValues,int row){
	
	bool toReturn=false;
	int col,num;
	int numPossibleValuesForCell;
	int possibleNum;
	
	for(col=0;col<SIZE;col++){
		if(grid[row][col]==0){
			numPossibleValuesForCell = 0;
			for(num=1;num<=SIZE;num++){
				if(possibleValues[row][col][num-1]==1){
					numPossibleValuesForCell++;
					possibleNum=num;
				}
				if (numPossibleValuesForCell > 1)
					break;
			}
			if(numPossibleValuesForCell==1){
				grid[row][col]=possibleNum;
				updatePossibleValues(grid,possibleValues, row,col);
				toReturn = true;
			}	
		}
	}
	return toReturn;
}

bool eliminationOnCol(int **grid, int*** possibleValues,int col){
	
	bool toReturn=false;
	int row,num;
	int numPossibleValuesForCell;
	int possibleNum;
	
	for(row=0;row<SIZE;row++){
		if(grid[row][col]==0){
			numPossibleValuesForCell = 0;
			for(num=1;num<=SIZE;num++){
				if(possibleValues[row][col][num-1]==1){
					numPossibleValuesForCell++;
					possibleNum=num;
				}
				if (numPossibleValuesForCell > 1)
					break;
			}
			if(numPossibleValuesForCell==1){
				grid[row][col]=possibleNum;
				updatePossibleValues(grid,possibleValues, row,col);
				toReturn=true;
			}	
		}
	}
	return toReturn;
}

bool eliminationOnBox(int **grid, int*** possibleValues,int boxStartRow,int boxStartCol){
	
	bool toReturn=false;
	int row,col,num;
	int numPossibleValuesForCell;
	int possibleNum;

	for(row=0;row<MINIGRIDSIZE;row++){
		for(col=0;col<MINIGRIDSIZE;col++){
			if(grid[row+boxStartRow][col+boxStartCol]==0){
				numPossibleValuesForCell=0;
				for(num=1;num<=SIZE;num++){
					if(possibleValues[row+boxStartRow][col+boxStartCol][num-1]==1){
						numPossibleValuesForCell++;
						possibleNum=num;
					}
					if (numPossibleValuesForCell > 1)
						break;
				}
				if(numPossibleValuesForCell==1){
					grid[row+boxStartRow][col+boxStartCol]=possibleNum;
					updatePossibleValues(grid,possibleValues, row+boxStartRow,col+boxStartCol);
					toReturn=true;
				}	
			}
		}
	}
	return toReturn;
}

bool elimination(int **grid,int*** possibleValues){
	
	bool toReturn = false;

	int row;
	for(row=0;row<SIZE;row++){
		if(eliminationOnRow(grid,possibleValues, row)){
			toReturn = true;
		}
	}

	int col;
	for(col=0;col<SIZE;col++){
		if(eliminationOnCol(grid,possibleValues, col)){
			toReturn = true;
		}
	}

	for(row=0;row<SIZE;row=row+MINIGRIDSIZE){
		for(col=0;col<SIZE;col=col+MINIGRIDSIZE){
			if(eliminationOnBox(grid,possibleValues, row,col)){
				toReturn = true;
			}
		}
	}

	return toReturn;
}

void applyHeuristics(int **grid,int*** possibleValues){

	while(true){
		while(elimination(grid,possibleValues));
		if(!loneRanger(grid,possibleValues))
			break;
	}	
}


/*----------------- Stack Functions ----------------------*/

typedef struct {
	int*** grid_list;
	int list_sz;
	int list_alloc;
} stack_struct;
typedef stack_struct* my_stack_t;


// initializes the stack, sets list_alloc to initial_size
void Stack_init(my_stack_t s, int initial_size){
	s->list_alloc = initial_size;
	s->grid_list = (int***) malloc(s->list_alloc * sizeof(int**));
	s->list_sz = 0;
}

// returns true if stack is empty, false otherwise
bool Empty_stack(my_stack_t s){
	return (s->list_sz==0);
}

// returns the top most element of the stack , and decrement stacks size by one
int** Pop(my_stack_t s){
	if (s->list_sz==0){
		printf("Error : Nothing to pop\n");
		exit(0);
	}
	int** last = s->grid_list[s->list_sz - 1];
	(s->list_sz)--;
	return last;
}

int Size(my_stack_t s){
	return s->list_sz;
}

// pushes an element to the top of the stack
void Push(my_stack_t s, int** grid){
	if (s->list_sz == s->list_alloc){
		s->grid_list = (int***) realloc(s->grid_list , 2*s->list_alloc*sizeof(int**));
		s->list_alloc *= 2;
	}
	s->grid_list[s->list_sz] = grid;
	(s->list_sz)++;
}

/*----------------------------------------------------------*/

void initParallelSolver(my_stack_t s, int num_threads){

	while(Size(s) < num_threads){
		int** grid;
		if (!Empty_stack(s))
			grid = Pop(s);
		else return;
		// printf("hjcbkjdbcksjbcjsbdcj\n\n");
		int row,col;

		if(!getEmptyCell(grid,&row,&col)){
			return;
		}
		// printf("r,c : %d,%d\n",row,col);
		int num;
		for(num=1;num <=SIZE;num++){
			// printf("num : %d\n",num);
			if(isValidNum(grid,row,col,num)){
				grid[row][col] = num;
				
				int** new_grid = allocate2DArray(SIZE);
				copy2DArray(grid,new_grid,SIZE);

				Push(s,new_grid);
				// printf("Stack Size : %d\n",Size(s));
			}
		}
		free2DArray(grid,SIZE);
	}
}

bool isSolutionExist(int** grid,int*** possibleValues){

	if (result_found) return false;

	int row,col;
	if(!getMinimumEmptyCell(grid,possibleValues,&row,&col)){
		return true;
	}

	int num;

	// make copy of possibleValues
	int*** origPossibleValues = allocateArray(SIZE);
	copyArray(possibleValues,origPossibleValues,SIZE);

	int** originalGrid = allocate2DArray(SIZE);
	copy2DArray(grid,originalGrid,SIZE);
				
	for(num=1;num <=SIZE;num++){
		if(possibleValues[row][col][num-1]==1){
			grid[row][col] = num;			
			
			updatePossibleValues(grid,possibleValues, row,col);

			applyHeuristics(grid,possibleValues);
			
			if(isSolutionExist(grid,possibleValues)){
				free2DArray(originalGrid,SIZE);
				freeArray(origPossibleValues,SIZE);
				return true;
			}
			
			// restore possibleValues
			copy2DArray(originalGrid,grid,SIZE);
			copyArray(origPossibleValues,possibleValues,SIZE);
			grid[row][col] = 0;
		}
	}

	free2DArray(originalGrid,SIZE);
	freeArray(origPossibleValues,SIZE);
	return false;	
}

int** solveSudoku(int **grid){
	int*** possibleValues=allocateArray(SIZE);
	initialisePossibleValues(grid, possibleValues);
	applyHeuristics(grid,possibleValues); // can be parallelized
	
	int r,c;
	if(!getMinimumEmptyCell(grid, possibleValues,&r,&c)){
		return grid;
	}
	
	// printf("\n************************GRID AFTER APPLYING HEURISTICS***********************\n");
	// int i,j;
	// for (i=0;i<SIZE;i++){
	// 	for (j=0;j<SIZE;j++)
	// 		printf("%d ",grid[i][j]);
	// 	printf("\n");
	// }
	// printf("*********************************************************\n\n");


	// printf("NUM THREADS : %d\n",thread_count);
	my_stack_t grid_stack = malloc(sizeof(stack_struct));
	Stack_init(grid_stack,thread_count);

	int** temp_grid = allocate2DArray(SIZE);
	copy2DArray(grid,temp_grid,SIZE);
	Push(grid_stack,temp_grid);

	initParallelSolver(grid_stack, thread_count);

	// printf("After init parallel solver Stack size : %d\n",Size(grid_stack));

	if (Size(grid_stack) == 0)
		return grid;

	while(Size(grid_stack) < thread_count){
		Push(grid_stack,grid_stack->grid_list[Size(grid_stack)-1]);
	}

	int** result_grid = allocate2DArray(SIZE);

	#pragma omp parallel num_threads(thread_count) shared(grid_stack)
	{
		int tid = omp_get_thread_num();
		int n_thrds = omp_get_num_threads();

		int** thread_grid;
		int*** thread_possibleValues = allocateArray(SIZE);
		#pragma omp critical
		{
			thread_grid = Pop(grid_stack);
		}	
		
		#pragma omp barrier

		initialisePossibleValues(thread_grid,thread_possibleValues);
		bool thread_result = isSolutionExist(thread_grid,thread_possibleValues);
		// printf("tid, thread_result : %d,%d\n",tid,thread_result);
		if (thread_result){
			#pragma omp critical
			{
				result_found = true;
				copy2DArray(thread_grid,result_grid,SIZE);
			}
		}

		if(tid==n_thrds-1){
			while(!Empty_stack(grid_stack)){
				// printf("injabcjab");
				thread_grid = Pop(grid_stack);
				initialisePossibleValues(thread_grid,thread_possibleValues);
				bool thread_result = isSolutionExist(thread_grid,thread_possibleValues);
				if (thread_result){
					#pragma omp critical
					{
						result_found = true;
						copy2DArray(thread_grid,result_grid,SIZE);
					}
				}
			}
		}
	}

	// bool isSol = isSolutionExist(int** grid);
	return result_grid;
}
