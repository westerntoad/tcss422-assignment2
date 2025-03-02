/*
 *  pcmatrix module
 *  Primary module providing control flow for the pcMatrix program
 *
 *  Producer consumer bounded buffer program to produce random matrices in parallel
 *  and consume them while searching for valid pairs for matrix multiplication.
 *  Matrix multiplication requires the first matrix column count equal the
 *  second matrix row count.
 *
 *  A matrix is consumed from the bounded buffer.  Then matrices are consumed
 *  from the bounded buffer, ONE AT A TIME, until an eligible matrix for multiplication
 *  is found.
 *
 *  Totals are tracked using the ProdConsStats Struct for each thread separately:
 *  - the total number of matrices multiplied (multtotal from each consumer thread)
 *  - the total number of matrices produced (matrixtotal from each producer thread)
 *  - the total number of matrices consumed (matrixtotal from each consumer thread)
 *  - the sum of all elements of all matrices produced and consumed (sumtotal from each producer and consumer thread)
 *  
 *  Then, these values from each thread are aggregated in main thread for output
 *
 *  Correct programs will produce and consume the same number of matrices, and
 *  report the same sum for all matrix elements produced and consumed.
 *
 *  Each thread produces a total sum of the value of
 *  randomly generated elements.  Producer sum and consumer sum must match.
 *
 *  University of Washington, Tacoma
 *  TCSS 422 - Operating Systems
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <time.h>
#include "matrix.h"
#include "counter.h"
#include "prodcons.h"
#include "pcmatrix.h"

int main (int argc, char * argv[])
{
  // Process command line arguments
  int numw = NUMWORK;
  if (argc==1)
  {
    BOUNDED_BUFFER_SIZE=MAX;
    NUMBER_OF_MATRICES=LOOPS;
    MATRIX_MODE=DEFAULT_MATRIX_MODE;
    printf("USING DEFAULTS: worker_threads=%d bounded_buffer_size=%d matricies=%d matrix_mode=%d\n",numw,BOUNDED_BUFFER_SIZE,NUMBER_OF_MATRICES,MATRIX_MODE);
  }
  else
  {
    if (argc>1)
    {
      numw=atoi(argv[1]);
      BOUNDED_BUFFER_SIZE=MAX;
      NUMBER_OF_MATRICES=LOOPS;
      MATRIX_MODE=DEFAULT_MATRIX_MODE;
    }
    if (argc>2)
    {
      BOUNDED_BUFFER_SIZE=atoi(argv[2]);
    }
    if (argc>3)
    {
      NUMBER_OF_MATRICES=atoi(argv[3]);
    }
    if (argc>4)
    {
      MATRIX_MODE=atoi(argv[4]);
    }
    printf("USING: worker_threads=%d bounded_buffer_size=%d matricies=%d matrix_mode=%d\n",numw,BOUNDED_BUFFER_SIZE,NUMBER_OF_MATRICES,MATRIX_MODE);
  }

  time_t t;
  // Seed the random number generator with the system time
  srand((unsigned) time(&t));

  //
  // Demonstration code to show the use of matrix routines
  //
  // DELETE THIS CODE FOR YOUR SUBMISSION
  // ----------------------------------------------------------
  // bigmatrix = (Matrix **) malloc(sizeof(Matrix *) * BOUNDED_BUFFER_SIZE);
  // printf("MATRIX MULTIPLICATION DEMO:\n\n");
  // Matrix *m1, *m2, *m3;
  // for (int i=0;i<NUMBER_OF_MATRICES;i++)
  // {
  //   m1 = GenMatrixRandom();
  //   m2 = GenMatrixRandom();
  //   m3 = MatrixMultiply(m1, m2);
  //   if (m3 != NULL)
  //   {
  //     DisplayMatrix(m1,stdout);
  //     printf("    X\n");
  //     DisplayMatrix(m2,stdout);
  //     printf("    =\n");
  //     DisplayMatrix(m3,stdout);
  //     printf("\n");
  //     FreeMatrix(m3);
  //     FreeMatrix(m2);
  //     FreeMatrix(m1);
  //     m1=NULL;
  //     m2=NULL;
  //     m3=NULL;
  //   }
  // }
  // return 0;
  // ----------------------------------------------------------



  printf("Producing %d matrices in mode %d.\n",NUMBER_OF_MATRICES,MATRIX_MODE);
  printf("Using a shared buffer of size=%d\n", BOUNDED_BUFFER_SIZE);
  printf("With %d producer and consumer thread(s).\n",numw);
  printf("\n");

  bigmatrix = (Matrix **) malloc(BOUNDED_BUFFER_SIZE * sizeof(Matrix *));
  pthread_t threads[NUMWORK * 2];
  ProdConsStats* return_values[NUMWORK * 2];

  // https://stackoverflow.com/questions/35403892/creating-threads-in-a-loop
  for (int i = 0; i < NUMWORK * 2; i++) {
    if (i % 2 == 0) {
      if (pthread_create(&threads[i], NULL, prod_worker, NULL) != 0) {
        fprintf(stderr, "error: Cannot create thread # %d\n", i);
        break;
      }
    } else {
      if (pthread_create(&threads[i], NULL, cons_worker, NULL) != 0) {
        fprintf(stderr, "error: Cannot create thread # %d\n", i);
        break;
    }
    }
  }

  // join threads, recieving stats from worker functions
  for (int i = 0; i < NUMWORK * 2; i++) {
    if (i % 2 == 0) {
      if (pthread_join(threads[i], (void*) &return_values[i]) != 0) {
        fprintf(stderr, "error: Cannot join thread # %d\n", i);
      }
    } else {
      if (pthread_join(threads[i], (void*) &return_values[i]) != 0) {
        fprintf(stderr, "error: Cannot join thread # %d\n", i);
      }
    }
  }

  // initialize total stats
  int matrices_produced = 0; // total #matrices produced
  int matrices_consumed = 0; // total #matrices consumed
  int produced_total_sum = 0; // total sum of elements for matrices produced
  int consumed_total_sum = 0; // total sum of elements for matrices consumed
  int total_multiplications = 0; // total # multiplications

  // loop over return_values, summing stats from each producer/consumer worker
  for (int i = 0; i < NUMWORK * 2; i++) {
    if (i % 2 == 0) {
      matrices_produced += return_values[i]->matrixtotal;
      produced_total_sum += return_values[i]->sumtotal;
    } else {
      matrices_consumed += return_values[i]->matrixtotal;
      consumed_total_sum += return_values[i]->sumtotal;
      total_multiplications += return_values[i]->multtotal;
    }
    free(return_values[i]);
  }
  free(bigmatrix);
  // consume ProdConsStats from producer and consumer threads [HINT: return from join]
  // add up total matrix stats in prs, cos, prodtot, constot, consmul

  printf("Sum of Matrix elements --> Produced=%d = Consumed=%d\n",produced_total_sum,consumed_total_sum);
  printf("Matrices produced=%d consumed=%d multiplied=%d\n",
    matrices_produced,
    matrices_consumed,
    total_multiplications);
  return 0; 
}
