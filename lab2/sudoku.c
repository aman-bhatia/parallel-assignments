/*2013CS10207_2013CS50294*/

#include <stdio.h>
#include <stdbool.h>
#include <omp.h>
#include "sudoku.h"

extern int thread_count;

bool result_found = false;

char*** allocateArray(int n){
	char ***a = malloc(sizeof(char**)*n);
	int i,j;
	for(i=0;i<n;i++){
		a[i] = malloc(sizeof(char*)*n);
		for(j=0;j<n;j++){
			a[i][j] = malloc(sizeof(char)*(n+1));
		}
	}
	return a;
}

char** allocate2DArray(int n){
	char **a = malloc(sizeof(char*)*n);
	int i;
	for(i=0;i<n;i++){
		a[i] = malloc(sizeof(char)*n);
	}
	return a;
}

void freeArray(char ***a,int n){
	int i,j;
	for(i=0;i<n;i++){
		for(j=0;j<n;j++){
			free(a[i][j]);
		}
		free(a[i]);
	}
	free(a);
}

void free2DArray(char **a,int n){
	int i;
	for(i=0;i<n;i++){
		free(a[i]);
	}
	free(a);
}

void copyArray(char ***src, char ***dest, int n){
	int i,j,k;
	for(i=0;i<n;i++){
		for(j=0;j<n;j++){
			for(k=0;k<n+1;k++){
				dest[i][j][k] = src[i][j][k];
			}
		}
	}
}

void copy2DArray(char **src,char **dest,int n){
	int i,j;
	for(i=0;i<n;i++){
		for(j=0;j<n;j++){
			dest[i][j] = src[i][j];
		}
	}
}

//---------------------------Board Functions-------------------//

typedef struct {
	char** grid;
	char*** possibleValues;
} board;
typedef board* board_ptr;

board_ptr boardAlloc(){
	board_ptr b = malloc(sizeof(board));
	return b;
}

void freeBoard(board_ptr b){
	free2DArray(b->grid,SIZE);
	freeArray(b->possibleValues,SIZE);
	free(b);
}

//---------------------------------------------------------------//

bool isValidNum(char **grid,int row,int col,char num){

	int boxStartRow = row-row%MINIGRIDSIZE;
	int boxStartCol = col-col%MINIGRIDSIZE;
	
	int i;
	for(i=0;i<SIZE;i++){
		if ((grid[row][i]==num) || (grid[i][col]==num) || (grid[boxStartRow+(i/MINIGRIDSIZE)][boxStartCol+(i%MINIGRIDSIZE)]==num)) {
			return false;
		}
	}
	return true;
}

bool getEmptyCell(char **grid,int* row,int* col){
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

bool getMinimumEmptyCell(board_ptr b, int* row,int* col){
	int i,j,k;
	int minPossibleValue = SIZE+1;
	bool flag=false;
	
	for(i=0;i<SIZE;i++){
		for(j=0;j<SIZE;j++){
			if(b->grid[i][j]==0){
				if(b->possibleValues[i][j][0]<minPossibleValue){
					minPossibleValue = b->possibleValues[i][j][0];
					*row = i;
					*col = j;
					flag=true;
				} 	
			}
		}
	}
	return flag;
}

void initialisePossibleValues(board_ptr b){
	int row,col,num;
	for(row=0;row<SIZE;row++){
		for(col=0;col<SIZE;col++){
			if(b->grid[row][col]==0){
				b->possibleValues[row][col][0] = 0;
				for(num=1;num<=SIZE;num++){
					if (isValidNum(b->grid,row,col,num)){
						b->possibleValues[row][col][num] = 1;
						b->possibleValues[row][col][0]++;
					} else{
						b->possibleValues[row][col][num] = 0;
					}
				}	
			}else{
				b->possibleValues[row][col][0] = 1;
				for(num=1;num<=SIZE;num++){
					b->possibleValues[row][col][num] = (num==b->grid[row][col]);
				}	
			}
		}
	}
}

void updatePossibleValues(board_ptr b, int row,int col){
	
	int i;
	int boxStartRow = row - row%MINIGRIDSIZE;
	int boxStartCol = col -  col%MINIGRIDSIZE;

	char num = b->grid[row][col];

	for(i=0;i<SIZE;i++){
		// update ith row element
		if(b->grid[row][i]==0 && b->possibleValues[row][i][num]==1){
			b->possibleValues[row][i][num] = 0;
			b->possibleValues[row][i][0]--;
		}

		//update ith column element
		if(b->grid[i][col]==0 && b->possibleValues[i][col][num]==1){
			b->possibleValues[i][col][num] = 0;
			b->possibleValues[i][col][0]--;
		}

		// update (i/MINIGRIDSIZE) , (i%MINIGRIDSIZE) box element
		int r = boxStartRow + (i/MINIGRIDSIZE);
		int c = boxStartCol + (i%MINIGRIDSIZE);
		if(b->grid[r][c]==0 && b->possibleValues[r][c][num]==1){
			b->possibleValues[r][c][num] = 0;
			b->possibleValues[r][c][0]--;
		}
	}

	// update the row,col cell
	b->possibleValues[row][col][0] = 1;
	for(num=1;num<=SIZE;num++){
		b->possibleValues[row][col][num] = (num==b->grid[row][col]);
	}
}

bool loneRangerOnRow(board_ptr b,int row){
	
	int col;
	int count,index;
	char num;

	for(num=1;num<=SIZE;num++){
		count=0;
		for(col=0;col<SIZE;col++){
			if(b->possibleValues[row][col][num]==1){
				if (b->grid[row][col] != 0) break;
				count++;
				index = col;
			}
			if (count > 1)
				break;
		}
		if(count==1){
			b->grid[row][index]=num;
			updatePossibleValues(b, row,index);
			return true;	
		}
	}

	return false;
}

bool loneRangerOnCol(board_ptr b,int col){

	int row;
	int count,index;
	char num;

	for(num=1;num<=SIZE;num++){
		count=0;
		for(row=0;row<SIZE;row++){
			if(b->possibleValues[row][col][num]==1){
				if (b->grid[row][col] != 0) break;
				count++;
				index = row;
			}
			if (count > 1)
				break;
		}
		if(count==1){
			b->grid[index][col]=num;
			updatePossibleValues(b, index,col);
			return true;
		}
	}

	return false;
}

bool loneRangerOnBox(board_ptr b,int boxStartRow,int boxStartCol){
	
	int row,col;
	int count,rowIndex,colIndex;
	char num;

	for(num=1;num<=SIZE;num++){
		count=0;
		for(row=0;row<MINIGRIDSIZE;row++){
			for(col=0;col<MINIGRIDSIZE;col++){
				if(b->possibleValues[row+boxStartRow][col+boxStartCol][num]==1){
				if (b->grid[row+boxStartRow][col+boxStartCol] != 0) break;
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
			b->grid[rowIndex+boxStartRow][colIndex+boxStartCol]=num;
			updatePossibleValues(b, rowIndex+boxStartRow,colIndex+boxStartCol);
			return true;
		}
	}

	return false;
}

bool loneRanger(board_ptr b){

	bool toReturn = false;

	int row;
	for(row=0;row<SIZE;row++){
		if(loneRangerOnRow(b, row)){
			toReturn = true;
			// return true;
		}
	}

	int col;
	for(col=0;col<SIZE;col++){
		if(loneRangerOnCol(b, col)){
			toReturn = true;
			// return true;
		}
	}

	for(row=0;row<SIZE;row=row+MINIGRIDSIZE){
		for(col=0;col<SIZE;col=col+MINIGRIDSIZE){
			if(loneRangerOnBox(b, row,col)){
				toReturn = true;
				// return true;
			}
		}
	}
	
	return toReturn;
}


bool eliminationOnRow(board_ptr b,int row){
	
	bool toReturn=false;
	int col;
	char num;
	
	for(col=0;col<SIZE;col++){
		if(b->grid[row][col]==0){
			if(b->possibleValues[row][col][0]==1){
				for(num=1;num<=SIZE;num++){
					if (b->possibleValues[row][col][num]==1){
						b->grid[row][col]=num;
						updatePossibleValues(b, row,col);
						toReturn = true;
						break;
					}
				}
			}	
		}
	}
	return toReturn;
}

bool eliminationOnCol(board_ptr b,int col){
	
	bool toReturn=false;
	int row;
	char num;
	
	for(row=0;row<SIZE;row++){
		if(b->grid[row][col]==0){
			if(b->possibleValues[row][col][0]==1){
				for(num=1;num<=SIZE;num++){
					if(b->possibleValues[row][col][num]==1){
						b->grid[row][col]=num;
						updatePossibleValues(b, row,col);
						toReturn=true;
						break;
					}
				}
			}
		}
	}
	return toReturn;
}

bool eliminationOnBox(board_ptr b,int boxStartRow,int boxStartCol){
	
	bool toReturn=false;
	int row,col;
	char num;

	for(row=0;row<MINIGRIDSIZE;row++){
		for(col=0;col<MINIGRIDSIZE;col++){
			if(b->grid[row+boxStartRow][col+boxStartCol]==0){
				if(b->possibleValues[row+boxStartRow][col+boxStartCol][0]==1){
					for(num=1;num<=SIZE;num++){
						if(b->possibleValues[row+boxStartRow][col+boxStartCol][num]==1){
							b->grid[row+boxStartRow][col+boxStartCol]=num;
							updatePossibleValues(b, row+boxStartRow,col+boxStartCol);
							toReturn=true;
							break;
						}
					}
				}
			}
		}
	}
	return toReturn;
}

bool elimination(board_ptr b){
	
	bool toReturn = false;

	int row;
	for(row=0;row<SIZE;row++){
		if(eliminationOnRow(b, row)){
			toReturn = true;
		}
	}

	int col;
	for(col=0;col<SIZE;col++){
		if(eliminationOnCol(b, col)){
			toReturn = true;
		}
	}

	for(row=0;row<SIZE;row=row+MINIGRIDSIZE){
		for(col=0;col<SIZE;col=col+MINIGRIDSIZE){
			if(eliminationOnBox(b, row,col)){
				toReturn = true;
			}
		}
	}

	return toReturn;
}

void applyHeuristics(board_ptr b){

	while(true){
		while(elimination(b));
		if(!loneRanger(b))
			break;
	}	
}


/*----------------- Stack Functions ----------------------*/

typedef struct {
	board_ptr* board_list;
	int list_sz;
	int list_alloc;
} stack_struct;
typedef stack_struct* my_stack_t;


// initializes the stack, sets list_alloc to initial_size
void Stack_init(my_stack_t s, int initial_size){
	s->list_alloc = initial_size;
	s->board_list = (board_ptr*) malloc(s->list_alloc * sizeof(board_ptr));
	s->list_sz = 0;
}

// returns true if stack is empty, false otherwise
bool Empty_stack(my_stack_t s){
	return (s->list_sz==0);
}

// returns the top most element of the stack , and decrement stacks size by one
board_ptr Pop(my_stack_t s){
	if (s->list_sz==0){
		printf("Error : Nothing to pop\n");
		exit(0);
		// return NULL;
	}
	board_ptr last = s->board_list[s->list_sz - 1];
	(s->list_sz)--;
	return last;
}

int Size(my_stack_t s){
	return s->list_sz;
}

// pushes an element to the top of the stack
void Push(my_stack_t s, board_ptr b){
	if (s->list_sz == s->list_alloc){
		s->board_list = (board_ptr*) realloc(s->board_list , 2*s->list_alloc*sizeof(board_ptr));
		s->list_alloc *= 2;
	}
	s->board_list[s->list_sz] = b;
	(s->list_sz)++;
}

int** char2int(char** charGrid, int** intGrid, int n){
	int i,j;
	for(i=0;i<n;i++){
		for(j=0;j<n;j++){
			intGrid[i][j] = charGrid[i][j];
		}
	}
	return intGrid;
}

char** int2char(int** intGrid, char** charGrid, int n){
	int i,j;
	for(i=0;i<n;i++){
		for(j=0;j<n;j++){
			charGrid[i][j] = intGrid[i][j];
		}
	}
	return charGrid;
}

/*----------------------------------------------------------*/

int** solveSudoku(int **grid){

	if (SIZE==9 || SIZE==16) thread_count=1;
	else if (SIZE==25) thread_count=16;
	else if (SIZE==36) thread_count=25;

	// make copy of original grid
	char** givenGrid=allocate2DArray(SIZE);
	givenGrid = int2char(grid,givenGrid,SIZE);

	// allocate board for given grid
	board_ptr b = boardAlloc();
	b->grid = givenGrid;
	b->possibleValues=allocateArray(SIZE);
	initialisePossibleValues(b);

	applyHeuristics(b);

	int r,c;
	if(!getMinimumEmptyCell(b,&r,&c)){
		return char2int(b->grid,grid,SIZE);		// solution found
	}

	if (b->possibleValues[r][c][0]==0){
		return grid;		// no solution exist
	}

	my_stack_t board_stack = malloc(sizeof(stack_struct));
	Stack_init(board_stack,thread_count);

	Push(board_stack,b);

	bool idle[thread_count];
	int i;
	for(i=0;i<thread_count;i++){
		idle[i] = false;
	}

	char** resultCharGrid=allocate2DArray(SIZE);

	bool all_threads_idle = false;
	#pragma omp parallel num_threads(thread_count) default(none) shared(resultCharGrid,grid,board_stack,idle,all_threads_idle,result_found)
	{
		while(!all_threads_idle){

			if (result_found) break;

			board_ptr b_thrd = NULL;
			#pragma omp critical
			{
				if(!Empty_stack(board_stack))
					b_thrd = Pop(board_stack);
			}

			if (b_thrd == NULL){
				idle[omp_get_thread_num()] = true;
			}else{
				idle[omp_get_thread_num()] = false;
				
				applyHeuristics(b_thrd);

				int row,col;
				if(!getMinimumEmptyCell(b_thrd,&row,&col)){
					#pragma omp critical
					{
						result_found = true;
						copy2DArray(b_thrd->grid,resultCharGrid,SIZE);		// solution found
					}
				}
				
				if (result_found) break;

				if (b_thrd->possibleValues[row][col][0]==0){
					freeBoard(b_thrd);								// no solution exist
					continue;
				}



				int num;
				for(num=SIZE;num >=1;num--){
					if(b_thrd->possibleValues[row][col][num]==1){
						
						board_ptr new_b = boardAlloc();
						new_b->grid = allocate2DArray(SIZE);
						copy2DArray(b_thrd->grid,new_b->grid,SIZE);

						new_b->possibleValues = allocateArray(SIZE);
						copyArray(b_thrd->possibleValues,new_b->possibleValues,SIZE);

						new_b->grid[row][col] = num;
						updatePossibleValues(new_b,row,col);
						
						#pragma omp critical
						{
							Push(board_stack,new_b);
						}
					}
				}

			freeBoard(b_thrd);
			}

			#pragma omp master
			{
				bool false_found = false;
				int j;
				for(j=0;j<omp_get_num_threads();j++){
					if (idle[j] == false){
						false_found = true;
						break;
					}
				}
				if(false_found) all_threads_idle = false;
				else all_threads_idle = true;
			}
		}
	}
	return char2int(resultCharGrid,grid,SIZE);
}
