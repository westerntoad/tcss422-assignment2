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

int main (int argc, char * argv[]) {
    //int numw = NUMWORK;
    int numw = 2;
    BOUNDED_BUFFER_SIZE=MAX;
    NUMBER_OF_MATRICES=LOOPS;
    //NUMBER_OF_MATRICES=8;
    //NUMBER_OF_MATRICES=8;
    MATRIX_MODE=DEFAULT_MATRIX_MODE;
    if (argc >= 2)
        numw = atoi(argv[1]);
    if (argc >= 3)
        BOUNDED_BUFFER_SIZE = atoi(argv[2]);
    if (argc >= 4)
        NUMBER_OF_MATRICES = atoi(argv[3]);
    if (argc >= 5)
        MATRIX_MODE = atoi(argv[4]);

    if (argc == 1) {
        printf("USING DEFAULTS: worker_threads=%d bounded_buffer_size=%d matricies=%d matrix_mode=%d\n",numw,BOUNDED_BUFFER_SIZE,NUMBER_OF_MATRICES,MATRIX_MODE);
    } else {
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
    /*bigmatrix = (Matrix **) malloc(sizeof(Matrix *) * BOUNDED_BUFFER_SIZE);
    printf("MATRIX MULTIPLICATION DEMO:\n\n");
    Matrix *m1, *m2, *m3;
    for (int i=0;i<NUMBER_OF_MATRICES;i++)
    {
    m1 = GenMatrixRandom();
    m2 = GenMatrixRandom();
    m3 = MatrixMultiply(m1, m2);
    if (m3 != NULL)
    {
      DisplayMatrix(m1,stdout);
      printf("    X\n");
      DisplayMatrix(m2,stdout);
      printf("    =\n");
      DisplayMatrix(m3,stdout);
      printf("\n");
      FreeMatrix(m3);
      FreeMatrix(m2);
      FreeMatrix(m1);
      m1=NULL;
      m2=NULL;
      m3=NULL;
    }
    }
    return 0;*/
    // ----------------------------------------------------------



    printf("Producing %d matrices in mode %d.\n",NUMBER_OF_MATRICES,MATRIX_MODE);
    printf("Using a shared buffer of size=%d\n", BOUNDED_BUFFER_SIZE);
    printf("With %d producer and consumer thread(s).\n",numw);
    printf("\n");

    bigmatrix = (Matrix **) malloc(sizeof(Matrix *) * BOUNDED_BUFFER_SIZE);
    counter_t* prodCtr = (counter_t*) malloc(sizeof(counter_t));
    counter_t* consCtr = (counter_t*) malloc(sizeof(counter_t));
    init_cnt(prodCtr);
    init_cnt(consCtr);

    pthread_t* producers = malloc(sizeof(pthread_t) * numw);
    pthread_t* consumers = malloc(sizeof(pthread_t) * numw);
    for (int i = 0; i < numw; i++) {
        pthread_create(producers + i, NULL, prod_worker, prodCtr);
        pthread_create(consumers + i, NULL, cons_worker, consCtr);
    }

    ProdConsStats** prStats = malloc(sizeof(ProdConsStats*) * numw);
    ProdConsStats** coStats = malloc(sizeof(ProdConsStats*) * numw);
    for (int i = 0; i < numw; i++) {
        pthread_join(*(producers + i), (void**) (prStats + i));
        pthread_join(*(consumers + i), (void**) (coStats + i));
    }

    // These are used to aggregate total numbers for main thread output
    int prs = 0; // total #matrices produced
    int cos = 0; // total #matrices consumed
    int prodtot = 0; // total sum of elements for matrices produced
    int constot = 0; // total sum of elements for matrices consumed
    int consmul = 0; // total # multiplications

    // consume ProdConsStats from producer and consumer threads [HINT: return from join]
    // add up total matrix stats in prs, cos, prodtot, constot, consmul
    for (int i = 0; i < numw; i++) {
        prs += (**(prStats + i)).matrixtotal;
        cos += (**(coStats + i)).matrixtotal;
        prodtot += (**(prStats + i)).sumtotal;
        constot += (**(coStats + i)).sumtotal;
        consmul += (**(coStats + i)).multtotal;
    }

    //printf("Sum of Matrix elements --> Produced=%d = Consumed=%d\n",prs,cos);
    //printf("Matrices produced=%d consumed=%d multiplied=%d\n",prodtot,constot,consmul);
    printf("Sum of Matrix elements --> Produced=%d , Consumed=%d\n",prodtot,constot);
    printf("Matrices produced=%d consumed=%d multiplied=%d\n",prs,cos,consmul);

    return 0;
}
