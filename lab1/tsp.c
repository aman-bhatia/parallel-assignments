/* 
 * Purpose:  Use iterative depth-first search and OpenMP to solve an 
 *           instance of the travelling salesman problem.  
 *
 * Compile:  gcc -O3 -Wall -fopenmp -o tsp tsp.c
 * Usage:    ./tsp <thread count> <matrix_file>
 *
 * Input:    From a user-specified file, the number of cities
 *           followed by the costs of travelling between the
 *           cities organized as a matrix:  the cost of
 *           travelling from city i to city j is the ij entry.
 *           Costs are nonnegative ints.  Diagonal entries are 0.
 * Output:   The best tour found by the program and the cost
 *           of the tour.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <omp.h>
#include <limits.h>
#include <stdbool.h>

const int INFINITY = INT_MAX;
const int NO_CITY = -1;
const int FALSE = 0;
const int TRUE = 1;
const int MAX_STRING = 1000;

typedef int city_t;
typedef int cost_t;

typedef struct {
   city_t* cities; /* Cities in partial tour           */
   int count;      /* Number of cities in partial tour */
   cost_t cost;    /* Cost of partial tour             */
} tour_struct;

typedef tour_struct* tour_t;
#define City_count(tour) (tour->count)
#define Tour_cost(tour) (tour->cost)
#define Last_city(tour) (tour->cities[(tour->count)-1])
#define Tour_city(tour,i) (tour->cities[(i)])

typedef struct {
   tour_t* list;
   int list_sz;
   int list_alloc;
}  stack_struct;
typedef stack_struct* my_stack_t;

/*----------------- Stack Functions ----------------------*/


// initializes the stack, sets list_alloc to initial_size
void Stack_init(my_stack_t s, int initial_size){
   s->list_alloc = initial_size;
   s->list = (tour_t *) malloc(s->list_alloc * sizeof(tour_t));
   s->list_sz = 0;
}

// returns true if stack is empty, false otherwise
bool Empty_stack(my_stack_t s){
   return (s->list_sz==0);
}

// returns the top most element of the stack , and decrement stacks size by one
tour_t Pop(my_stack_t s){
   if (s->list_sz==0){
      printf("Error : Nothing to pop\n");
      exit(0);
   }
   tour_t last = s->list[s->list_sz - 1];
   (s->list_sz)--;
   return last;
}

// pushes an element to the top of the stack
void Push(my_stack_t s, tour_t t){
   if (s->list_sz == s->list_alloc){
      s->list = (tour_t *) realloc(s->list , 2*s->list_alloc*sizeof(tour_t));
      s->list_alloc *= 2;
   }
   s->list[s->list_sz] = t;
   (s->list_sz)++;
}

/*------------------------------------------------------------------*/

/*--------------------------Queue Datastructure--------------------------*/

typedef struct {
   tour_t data;
   struct node_struct* next;
} node_struct;
typedef node_struct* node_t;


typedef struct{
   int sz;
   node_t first,last;
} queue_struct;
typedef queue_struct* queue_t;


// pushes an element to the front of the queue
void Enqueue(queue_t q, tour_t t){
   node_t new_node = (node_t) malloc(sizeof(node_t));
   new_node->data = t;
   new_node->next = NULL;
   if (q->last==NULL){
      q->first = new_node;
      q->last = new_node;
   } else {
      q->last->next = (struct node_struct *)new_node;
      q->last = new_node;
   }
   q->sz++;
}

// returns last element of the queue, and decrement the size fo queue by one
node_t Dequeue(queue_t q){
   if (q->first==NULL)
      return NULL;
   node_t ret = q->first;
   q->first = (node_t)q->first->next;
   q->sz--;

   if (q->sz==0)
      q->last=NULL;
   return ret;
}

// Initializes the queue, sets initial size to zero and first and last to NULL
void QueueInit(queue_t q){
   q->sz=0;
   q->first = NULL;
   q->last = NULL;
}

/*------------------------------------------------------------------*/


/* Global Vars: */
int n;  /* Number of cities in the problem */
int thread_count;
cost_t* digraph;
#define Cost(city1, city2) (digraph[city1*n + city2])
city_t home_town = 0;
tour_t best_tour;
int init_tour_count;

void Usage(char* prog_name);
void Read_digraph(FILE* digraph_file);
void Print_digraph(void);

void Par_tree_search(void); // TODO: Implement this function

void Set_init_tours(int my_rank, int* my_first_tour_p,
      int* my_last_tour_p);

void Print_tour(int my_rank, tour_t tour, char* title);
int  Best_tour(tour_t tour); 
void Update_best_tour(tour_t tour);
void Copy_tour(tour_t tour1, tour_t tour2);
void Add_city(tour_t tour, city_t);
void Remove_last_city(tour_t tour);
int  Feasible(tour_t tour, city_t city);
int  Visited(tour_t tour, city_t city);
void Init_tour(tour_t tour, cost_t cost);
tour_t Alloc_tour(my_stack_t avail);
void Free_tour(tour_t tour, my_stack_t avail);

/*------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   FILE* digraph_file;
   double start, finish;

   if (argc != 3) Usage(argv[0]);
   thread_count = strtol(argv[1], NULL, 10);
   if (thread_count <= 0) {
      fprintf(stderr, "Thread count must be positive\n");
      Usage(argv[0]);
   }
   digraph_file = fopen(argv[2], "r");
   if (digraph_file == NULL) {
      fprintf(stderr, "Can't open %s\n", argv[2]);
      Usage(argv[0]);
   }
   Read_digraph(digraph_file);
   fclose(digraph_file);
#  ifdef DEBUG
   Print_digraph();
#  endif   

   best_tour = Alloc_tour(NULL);
   Init_tour(best_tour, INFINITY);
#  ifdef DEBUG
   Print_tour(-1, best_tour, "Best tour");
   printf("City count = %d\n",  City_count(best_tour));
   printf("Cost = %d\n\n", Tour_cost(best_tour));
#  endif

   start = omp_get_wtime();

   // allocate a queue
   queue_t queue = (queue_t) malloc(sizeof(queue_struct));
   QueueInit(queue);

   // allocate a initial tour which has only visited hometown
   tour_t t = Alloc_tour(NULL);
   Init_tour(t,0);

   // explore initially in bfs fashion to get partial tours
   Enqueue(queue,t);
   while (queue->sz > 0){

      // check whether we have sufficient elements in queue to distribute among threads      
      if (queue->sz >= thread_count)
         break;

      tour_t curr_tour = Dequeue(queue)->data;
      // Print_tour(-1,curr_tour,"aman");
      // printf("-----\n");

      if (City_count(curr_tour) == n){
         if (Best_tour(curr_tour))
            Update_best_tour(curr_tour);
      } else {
         int nbr;
         for (nbr = n-1; nbr >=1; nbr--){
            if (Feasible(curr_tour,nbr)) {
               Add_city(curr_tour,nbr);
               tour_t tour_cp = Alloc_tour(NULL);
               Copy_tour(curr_tour,tour_cp);
               Enqueue(queue,tour_cp);
               Remove_last_city(curr_tour);
            }
         }
      }
      Free_tour(curr_tour,NULL);
   }

   // whether we need to run parallel code or are we already done
	if (queue->sz >= thread_count){

      // make array of thread stacks
      my_stack_t * th_stacks = (my_stack_t *) malloc(thread_count * sizeof(my_stack_t));
      
      // initialize those stacks
      int i;
      for (i=0;i<thread_count;i++){
         th_stacks[i] = malloc(sizeof(stack_struct));
         Stack_init(th_stacks[i],4);
      }

      // push partial tours on those stacks
      i=0;
      while(queue->sz > 0){
         Push(th_stacks[i],Dequeue(queue)->data);
         i = (i+1)%thread_count;
      }

      int tid;
		#pragma omp parallel private(tid) num_threads(thread_count)
      {
        
      tid = omp_get_thread_num();

      // explore individual stacks in dfs fashion
      my_stack_t th_stack = th_stacks[tid];

		while (!Empty_stack(th_stack)){
   		tour_t curr_tour = Pop(th_stack);

   		if (City_count(curr_tour) == n){
   		      Update_best_tour(curr_tour);
   		}else {
   		   int nbr;
   		   for (nbr = n-1; nbr >=1; nbr--){
   		      if (Feasible(curr_tour,nbr)) {
   		         Add_city(curr_tour,nbr);
   		         tour_t tour_cp = Alloc_tour(NULL);
   		         Copy_tour(curr_tour,tour_cp);
   		         Push(th_stack,tour_cp);
   		         Remove_last_city(curr_tour);
   		      }
   		   }
   		}
   		Free_tour(curr_tour,NULL);
   	}
      }
   }
   
   finish = omp_get_wtime();
   
   Print_tour(-1, best_tour, "Best tour");
   printf("Cost = %d\n", best_tour->cost);
   printf("Time = %e seconds\n", finish-start);

   free(best_tour->cities);
   free(best_tour);
   free(digraph);
   return 0;
}  /* main */

/*------------------------------------------------------------------
 * Function:  Init_tour
 * Purpose:   Initialize the data members of allocated tour
 * In args:   
 *    cost:   initial cost of tour
 * Global in:
 *    n:      number of cities in TSP
 * Out arg:   
 *    tour
 */
void Init_tour(tour_t tour, cost_t cost) {
   int i;

   tour->cities[0] = 0; // hometown added as a starting point
   for (i = 1; i <= n; i++) {
      tour->cities[i] = NO_CITY;
   }
   tour->cost = cost;
   tour->count = 1;
}  /* Init_tour */


/*------------------------------------------------------------------
 * Function:  Usage
 * Purpose:   Inform user how to start program and exit
 * In arg:    prog_name
 */
void Usage(char* prog_name) {
   fprintf(stderr, "usage: %s <thread_count> <digraph file>\n", prog_name);
   exit(0);
}  /* Usage */

/*------------------------------------------------------------------
 * Function:  Read_digraph
 * Purpose:   Read in the number of cities and the digraph of costs
 * In arg:    digraph_file
 * Globals out:
 *    n:        the number of cities
 *    digraph:  the matrix file
 */
void Read_digraph(FILE* digraph_file) {
   int i, j;

   fscanf(digraph_file, "%d", &n);
   if (n <= 0) {
      fprintf(stderr, "Number of vertices in digraph must be positive\n");
      exit(-1);
   }
   digraph = malloc(n*n*sizeof(cost_t));

   for (i = 0; i < n; i++)
      for (j = 0; j < n; j++) {
         fscanf(digraph_file, "%d", &digraph[i*n + j]);
         if (i == j && digraph[i*n + j] != 0) {
            fprintf(stderr, "Diagonal entries must be zero\n");
            exit(-1);
         } else if (i != j && digraph[i*n + j] <= 0) {
            fprintf(stderr, "Off-diagonal entries must be positive\n");
            fprintf(stderr, "diagraph[%d,%d] = %d\n", i, j, digraph[i*n+j]);
            exit(-1);
         }
      }
}  /* Read_digraph */


/*------------------------------------------------------------------
 * Function:  Print_digraph
 * Purpose:   Print the number of cities and the digraphrix of costs
 * Globals in:
 *    n:        number of cities
 *    digraph:  digraph of costs
 */
void Print_digraph(void) {
   int i, j;

   printf("Order = %d\n", n);
   printf("Matrix = \n");
   for (i = 0; i < n; i++) {
      for (j = 0; j < n; j++)
         printf("%2d ", digraph[i*n+j]);
      printf("\n");
   }
   printf("\n");
}  /* Print_digraph */


/*------------------------------------------------------------------
 * Function:    Best_tour
 * Purpose:     Determine whether addition of the hometown to the 
 *              n-city input tour will lead to a best tour.
 * In arg:
 *    tour:     tour visiting all n cities
 * Ret val:
 *    TRUE if best tour, FALSE otherwise
 */
int Best_tour(tour_t tour) {
   cost_t cost_so_far = Tour_cost(tour);
   city_t last_city = Last_city(tour);

   if (cost_so_far + Cost(last_city, home_town) < Tour_cost(best_tour))
      return TRUE;
   else
      return FALSE;
}  /* Best_tour */


/*------------------------------------------------------------------
 * Function:    Update_best_tour
 * Purpose:     Replace the existing best tour with the input tour +
 *              hometown
 * In arg:
 *    tour:     tour that's visited all n-cities
 * Global out:
 *    best_tour:  the current best tour
 */
void Update_best_tour(tour_t tour) {
   #pragma omp critical
   {
      if (Best_tour(tour)) {
         Copy_tour(tour, best_tour);
         Add_city(best_tour, home_town);
         // printf("cost=%d\n",Tour_cost(best_tour));
      }
   }
}  /* Update_best_tour */


/*------------------------------------------------------------------
 * Function:   Copy_tour
 * Purpose:    Copy tour1 into tour2
 * In arg:
 *    tour1
 * Out arg:
 *    tour2
 */
void Copy_tour(tour_t tour1, tour_t tour2) {

   memcpy(tour2->cities, tour1->cities, (n+1)*sizeof(city_t));
   tour2->count = tour1->count;
   tour2->cost = tour1->cost;
}  /* Copy_tour */

/*------------------------------------------------------------------
 * Function:  Add_city
 * Purpose:   Add city to the end of tour
 * In arg:
 *    city
 * In/out arg:
 *    tour
 * Note: This should only be called if tour->count >= 1.
 */
void Add_city(tour_t tour, city_t new_city) {
   city_t old_last_city = Last_city(tour);
   tour->cities[tour->count] = new_city;
   (tour->count)++;
   tour->cost += Cost(old_last_city,new_city);
}  /* Add_city */

/*------------------------------------------------------------------
 * Function:  Remove_last_city
 * Purpose:   Remove last city from end of tour
 * In/out arg:
 *    tour
 * Note:
 *    Function assumes there are at least two cities on the tour --
 *    i.e., the hometown in tour->cities[0] won't be removed.
 */
void Remove_last_city(tour_t tour) {
   city_t old_last_city = Last_city(tour);
   city_t new_last_city;
   
   tour->cities[tour->count-1] = NO_CITY;
   (tour->count)--;
   new_last_city = Last_city(tour);
   tour->cost -= Cost(new_last_city,old_last_city);
}  /* Remove_last_city */

/*------------------------------------------------------------------
 * Function:  Feasible
 * Purpose:   Check whether nbr could possibly lead to a better
 *            solution if it is added to the current tour.  The
 *            function checks whether nbr has already been visited
 *            in the current tour, and, if not, whether adding the
 *            edge from the current city to nbr will result in
 *            a cost less than the current best cost.
 * In args:   All
 * Global in:
 *    best_tour
 * Return:    TRUE if the nbr can be added to the current tour.
 *            FALSE otherwise
 */
int Feasible(tour_t tour, city_t city) {
   city_t last_city = Last_city(tour);

   if (!Visited(tour, city) && 
        Tour_cost(tour) + Cost(last_city,city) < Tour_cost(best_tour))
      return TRUE;
   else
      return FALSE;
}  /* Feasible */


/*------------------------------------------------------------------
 * Function:   Visited
 * Purpose:    Use linear search to determine whether city has already
 *             been visited on the current tour.
 * In args:    All
 * Return val: TRUE if city has already been visited.
 *             FALSE otherwise
 */
int Visited(tour_t tour, city_t city) {
   int i;

   for (i = 0; i < City_count(tour); i++)
      if ( Tour_city(tour,i) == city ) return TRUE;
   return FALSE;
}  /* Visited */


/*------------------------------------------------------------------
 * Function:  Print_tour
 * Purpose:   Print a tour
 * In args:   All
 * Notes:      
 * 1.  Copying the tour to a string makes it less likely that the 
 *     output will be broken up by another process/thread
 * 2.  Passing a negative value for my_rank will cause the rank
 *     to be omitted from the output
 */
void Print_tour(int my_rank, tour_t tour, char* title) {
   int i;
   char string[MAX_STRING];

   if (my_rank >= 0)
      sprintf(string, "Th %d > %s %p: ", my_rank, title, tour);
   else
      sprintf(string, "%s = ", title);
   for (i = 0; i < City_count(tour); i++)
      sprintf(string + strlen(string), "%d ", Tour_city(tour,i));
   printf("%s\n", string);
}  /* Print_tour */


/*------------------------------------------------------------------
 * Function:  Alloc_tour
 * Purpose:   Allocate memory for a tour and its members
 * In/out arg:
 *    avail:  stack storing unused tours
 * Global in: n, number of cities
 * Ret val:   Pointer to a tour_struct with storage allocated for its
 *            members
 */
tour_t Alloc_tour(my_stack_t avail) {
   tour_t tmp;

   if (avail == NULL || Empty_stack(avail)) {
      tmp = malloc(sizeof(tour_struct));
      tmp->cities = malloc((n+1)*sizeof(city_t));
      return tmp;
   } else {
      return Pop(avail);
   }
}  /* Alloc_tour */

/*------------------------------------------------------------------
 * Function:  Free_tour
 * Purpose:   Free a tour
 * In/out arg:
 *    avail
 * Out arg:   
 *    tour
 */
void Free_tour(tour_t tour, my_stack_t avail) {
   if (avail == NULL) {
      free(tour->cities);
      free(tour);
   } else {
      Push(avail, tour);
   }
}  /* Free_tour */

