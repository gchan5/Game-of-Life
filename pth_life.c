/* File:     pth_life.c
 * Author:	 Gilbert Chan
 * Purpose:  Parallel implementation of John Conway's Game of Life.  
 *			 The game is ``played'' on a board or *world* consisting of
 *           a rectangular grid with m rows and n columns.  Each cell  
 *           in the grid is ``alive'' or ``dead.'' An initial generation 
 *           (generation 0) is either entered by the user or generated  
 *           using a random number generator.  
 *
 *           Subsequent generations are computed according to the 
 *           following rules:
 *
 *              - Any live cell with fewer than two live neighbors 
 *              dies, as if caused by under-population.
 *              - Any live cell with two or three live neighbors 
 *              lives on to the next generation.
 *              - Any live cell with more than three live neighbors 
 *              dies, as if by over-population.
 *              - Any dead cell with exactly three live neighbors 
 *              becomes a live cell, as if by reproduction.
 *
 *           Updates take place all at once.
 * 
 * Compile:  gcc -g -Wall -o pth_life pth_life.c -lpthread
 * Run:      ./pth_life <r> <c> <m> <n> <max> <'i'|'g'>
 *              r = number of rows of threads
 *              c = number of cols of threads
 *				    m = number of rows in the world 
 *				    n = number of columns in the world
 *              max = maximum number of generations the program should compute
 *              'i' = user will enter the initial world (generation 0) on stdin
 *              'g' = the program should use a random number generator to
 *         				generate the initial world.
 *
 * Input:    If command line has the "input" char ('i'), the first
 *              generation.  Each row should be entered on a separate
 *              line of input.  Live cells should be indicated with
 *              a capital 'X', and dead cells with a blank, ' '.
 *           If command line had the "generate" char ('g'), the
 *              probability that a cell will be alive.
 *
 * Output:   The initial world (generation 0) and the world after
 *           each subsequent generation up to and including
 *           generation = max_gen.  If all of the cells die,
 *           the program will terminate.
 *
 * Notes:
 * 1.  This implementation uses a "toroidal world" in which the
 *     the last row of cells is adjacent to the first row, and
 *     the last column of cells is adjacent to the first.
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#define LIVE 1
#define DEAD 0 
#define LIVE_IO 'X'
#define DEAD_IO ' '
#define MAX_TITLE 1000
#define BARRIER_COUNT 1000 

/* Global Variables */
int     thread_count;
int     m, n, r, s, BREAK;
int     live_count;
int     curr_gen = 0, max_gens;
int     *w1, *w2;
int     barrier_thread_count = 0;
pthread_mutex_t barrier_mutex;
pthread_cond_t ok_to_proceed;

/* Serial Functions */
void Usage(char prog_name[]);
void Read_world(char prompt[], int w1[], int m, int n);
void Gen_world(char prompt[], int w1[], int m, int n);
void Print_world(char title[], int w1[]);
int Count_nbhrs(int* w1, int m, int n, int i, int j);

/* Parellel Function */
void* Play_life(void* rank);
void *Barrier(void* rank);

int main(int argc, char* argv[]){
   char       ig;
   long       thread;
   pthread_t* thread_handles;
   char title[MAX_TITLE];

   if (argc != 7) Usage(argv[0]);
   thread_count = strtol(argv[1], NULL, 10) * strtol(argv[2], NULL, 10);
   m = strtol(argv[3], NULL, 10);
   n = strtol(argv[4], NULL, 10);
   r = strtol(argv[1], NULL, 10);
   s = strtol(argv[2], NULL, 10);
   max_gens = strtol(argv[5], NULL, 10);
   ig = argv[6][0];

   thread_handles = malloc(m*n*sizeof(pthread_t));
   w1 = malloc(m*n*sizeof(int));
   w2 = malloc(m*n*sizeof(int));

   pthread_mutex_init(&barrier_mutex, NULL);
   pthread_cond_init(&ok_to_proceed, NULL);

   if (ig == 'i') 
      Read_world("Enter generation 0", w1, m, n);
   else
      Gen_world("What's the probability that a cell is alive?", w1, m, n);

   printf("\n");
   sprintf(title, "Generation %d:", curr_gen);
   Print_world(title, w1);

   for (thread = 0; thread < thread_count; thread++)
   pthread_create(&thread_handles[thread], NULL,
      Play_life, (void*) thread);

   for (thread = 0; thread < thread_count; thread++)
   pthread_join(thread_handles[thread], NULL);

   if(curr_gen < max_gens) printf("There are no more live cells\n");

   pthread_mutex_destroy(&barrier_mutex);
   pthread_cond_destroy(&ok_to_proceed);
   free(w1);
   free(w2);
   free(thread_handles);

   return 0;
}

/*---------------------------------------------------------------------
 * Function:   Usage
 * Purpose:    Show user how to start the program and quit
 * In arg:     prog_name
 */
 void Usage(char prog_name[]) {
   fprintf(stderr, "usage: %s <r> <c> <m> <n> <max> <i|g>\n", prog_name);
   fprintf(stderr, "    r   = number of rows of threads\n");
   fprintf(stderr, "    c   = number of cols of threads\n");
   fprintf(stderr, "    m   = number of rows in the world\n");
   fprintf(stderr, "    n   = number of columns in the world\n");
   fprintf(stderr, "    max = max number of generations\n");
   fprintf(stderr, "    i   = user will enter generation 0\n");
   fprintf(stderr, "    g   = program should generate generation 0\n");
   exit(0);
}  /* Usage */

/*---------------------------------------------------------------------
 * Function:   Read_world
 * Purpose:    Get generation 0 from the user
 * In args:    prompt
 *             m:  number of rows in visible world
 *             n:  number of cols in visible world
 * Out arg:    w1:  stores generation 0
 *
 */
 void Read_world(char prompt[], int w1[], int m, int n) {
   int i, j;
   char c;

   printf("%s\n", prompt);
   for (i = 0; i < m; i++) {
      for (j = 0; j < n; j++) {
         scanf("%c", &c);
         if (c == LIVE_IO)
            w1[i*n + j] = LIVE;
         else
            w1[i*n + j] = DEAD;
      }
      /* Read end of line character */
      scanf("%c", &c);
   }
}  /* Read_world */

/*---------------------------------------------------------------------
 * Function:   Gen_world
 * Purpose:    Use a random number generator to create generation 0 
 * In args:    prompt
 *             m:  number of rows in visible world
 *             n:  number of cols in visible world
 * Out arg:    w1:  stores generation 0
 *
 */
void Gen_world(char prompt[], int w1[], int m, int n) {
   int i, j;
   double prob;
#  ifdef DEBUG
   int live_count = 0;
#  endif
   
   printf("%s\n", prompt);
   scanf("%lf", &prob);

   srandom(1);
   for (i = 0; i < m; i++)
      for (j = 0; j < n; j++)
         if (random()/((double) RAND_MAX) <= prob) {
            w1[i*n + j] = LIVE;
#           ifdef DEBUG
            live_count++;
#           endif
         } else {
            w1[i*n + j] = DEAD;
         }

#  ifdef DEBUG
         printf("Live count = %d, request prob = %f, actual prob = %f\n",
            live_count, prob, ((double) live_count)/(m*n));
#  endif
}  /* Gen_world */

/*---------------------------------------------------------------------
 * Function:     Play_life
 * Purpose:      Play Conway's game of life.  (See header doc)
 * In args:      rank   
 *
 */
 void *Play_life(void* rank) {
   long my_rank = (long) rank;
   int i, j, count;
   int local_m = m/r;
   int local_n = n/s;
   int start_row = (my_rank/s)*(local_m);
   int start_col = (my_rank%s)*(local_n);

   while (curr_gen < max_gens) {
      for (i = start_row; i < start_row+local_m; i++) {
         for (j = start_col; j < start_col+local_n; j++) {
            count = Count_nbhrs(w1, m, n, i, j);
   #        ifdef DEBUG
            if(my_rank == 0)
            printf("curr_gen = %d, i = %d, j = %d, count = %d\n",
               curr_gen, i, j, count);
   #        endif
            if (count < 2 || count > 3)
               w2[i*n + j] = DEAD;
            else if (count == 2)
               w2[i*n + j] = w1[i*n + j];
            else /* count == 3 */
               w2[i*n + j] = LIVE;
            if (w2[i*n + j] == LIVE) live_count++;
         }
      }
      Barrier(rank);
      if(BREAK == 1) break;     
   }

   return NULL;
}  /* Play_life */
  

/*---------------------------------------------------------------------
 * Function:   Print_world
 * Purpose:    Print the current world
 * In args:    title
 *             w1:  current gen
 */
void Print_world(char title[], int w1[]) {
   int i, j;

   printf("%s\n\n", title);

   for (i = 0; i < m; i++) {
      for (j = 0; j < n; j++)
         if (w1[i*n + j] == LIVE)
            printf("%c", LIVE_IO);
         else
            printf("%c", DEAD_IO);
     printf("\n");
   }

   printf("-------------\n");
}  /* Print_world */

/*---------------------------------------------------------------------
 * Function:   Count_nbhrs
 * Purpose:    Count the number of living nbhrs of the cell (i,j)
 * In args:    w1:  current world
 *             m:   number of rows in world
 *             n:   number of cols in world
 *             i:   row number of current cell
 *             j:   col number of current cell
 * Ret val:    The number of neighboring cells with living neighbors
 *
 * Note:       Since the top row of cells is adjacent to the bottom
 *             row, and since the left col of cells is adjacent to the
 *             right col, in a very small world, it's possible to
 *             count a cell as a neighbor twice.  So we assume that
 *             m and n are at least 3.
 */
int Count_nbhrs(int* w1, int m, int n, int i, int j) {
   int i1, j1, i2, j2;
   int count = 0;

   for (i1 = i-1; i1 <= i+1; i1++)
      for (j1 = j-1; j1 <= j+1; j1++) {
         i2 = (i1 + m) % m;
         j2 = (j1 + n) % n;
         count += w1[i2*n + j2];
      }
   count -= w1[i*n + j];

   return count;
}  /* Count_nbhrs */

/*-------------------------------------------------------------------
 * Function:    Barrier
 * Purpose:     Run BARRIER_COUNT barriers
 * In arg:      rank
 * Global var:  thread_count, barrier_thread_count, barrier_mutex,
 *              ok_to_proceed
 */
void *Barrier(void* rank) {
   int *tmp;
   char title[MAX_TITLE];

   pthread_mutex_lock(&barrier_mutex);
   barrier_thread_count++;
   if (barrier_thread_count == thread_count) {
      tmp = w1;
      w1 = w2;
      w2 = tmp;
      curr_gen++;

      if(live_count > 0){
         sprintf(title, "Generation %d:", curr_gen);
         Print_world(title, w1);
      } else {
         BREAK = 1;
      }

      live_count = 0;      
      barrier_thread_count = 0;
      pthread_cond_broadcast(&ok_to_proceed);
   } else {
      while (pthread_cond_wait(&ok_to_proceed,
                &barrier_mutex) != 0);
      // Mutex is relocked at this point.
   }
   pthread_mutex_unlock(&barrier_mutex);


   return NULL;
}  /* Barrier */