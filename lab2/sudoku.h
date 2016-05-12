#include <stdbool.h>
#include <stdlib.h>

#define SIZE 25
#define MINIGRIDSIZE 5

int **readInput(char *);
int isValid(int **, int **);
int **solveSudoku(int **);				

// void initialisePossibleValues(int**,int***);
// void updatePossibleValues(int**,int***,int ,int );

// // array utilitiy functions
// int*** allocateArray(int);
// void freeArray(int ***, int);
// void copyArray(int ***, int ***, int);

// // sudoku utitility functions
// bool isValidNum(int** ,int ,int ,int);
// bool getEmptyCell(int **, int *, int *);			
// bool getMinimumEmptyCell(int **,int***, int *, int *);			
// // bool isSolutionExist(int **);					

// // heuristics functions
// bool loneRangerOnRow(int **, int***,int);
// bool loneRangerOnCol(int **, int***,int);
// bool loneRangerOnBox(int **, int***,int, int);
// bool loneRanger(int **,int***);

// bool eliminationOnRow(int **, int***,int);
// bool eliminationOnCol(int **, int***,int);
// bool eliminationOnBox(int **, int***,int, int);
// bool elimination(int**,int***);

// void applyHeuristics(int **,int***); // can be parallelized



