/*
 *  prodcons module
 *  Producer Consumer module
 *
 *  Implements routines for the producer consumer module based on
 *  chapter 30, section 2 of Operating Systems: Three Easy Pieces
 *
 *  University of Washington, Tacoma
 *  TCSS 422 - Operating Systems
 */

// Authors: Conner Webber & Abraham Engebretson

// Include only libraries for this module
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "counter.h"
#include "matrix.h"
#include "pcmatrix.h"
#include "prodcons.h"


// Define Locks, Condition variables, and so on here
int fill = 0; // next empty index in bigmatrix
int use = 0; // index to oldest element in bigmatrix
int count = 0; // total count of items in bigmatrix
int matricesProduced = 0; // total matrix consumed count
int matricesConsumed = 0; // total matrix produced count
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; // not using fine-grained locking :(
pthread_cond_t empty = PTHREAD_COND_INITIALIZER; // condition variable for empty buffer
pthread_cond_t full = PTHREAD_COND_INITIALIZER; // condition variable for full buffer



// put and get taken directly from slides
int put(Matrix * value) {
    bigmatrix[fill] = value;
    fill = (fill + 1) % BOUNDED_BUFFER_SIZE;
    count++;

    return count;
}

Matrix * get() {
    Matrix * tmp = bigmatrix[use];
    use = (use + 1) % BOUNDED_BUFFER_SIZE;
    count--;

    return tmp;
}

// Matrix PRODUCER worker thread
void *prod_worker(void *arg) {
    // allocate & initialize stats
    ProdConsStats* stats = (ProdConsStats*) malloc(sizeof(ProdConsStats));
    stats->sumtotal = 0;
    stats->multtotal = 0;
    stats->matrixtotal = 0;


    while (1) {
        Matrix* m = GenMatrixRandom(); // generate matrix
        pthread_mutex_lock(&mutex);

        // wait if the buffer is currently full
        while (count == BOUNDED_BUFFER_SIZE)
            pthread_cond_wait(&empty, &mutex);

        // if the number of matrices produced equals the target, clean up and kill the worker.
        // check is done after while loop to accommodate signaling other producers
        if (matricesProduced >= NUMBER_OF_MATRICES) {
            pthread_cond_signal(&empty); // signal other producers to wake up and return stats
            pthread_mutex_unlock(&mutex);
            // note that it may not have been necessary to generate matrix if
            // we are freeing it anyway...
            FreeMatrix(m);
            break;
        }
        put(m); // add generated matrix to buffer

        // unlock & do stuff with stats
        matricesProduced++;
        // sum matrix is done inside lock to avoid consumer freeing it
        stats->sumtotal += SumMatrix(m); 
        pthread_cond_signal(&full);
        pthread_mutex_unlock(&mutex);

        stats->matrixtotal++;
    }
    
    return stats;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg) {
    // allocate & initialize stats
    ProdConsStats* stats = (ProdConsStats*) malloc(sizeof(ProdConsStats));
    stats->sumtotal = 0;
    stats->multtotal = 0;
    stats->matrixtotal = 0;
    // define matrices to be used for matrix multiplication
    Matrix *m1, *m2, *m3;

    while (1) {
        // lock for m1
        pthread_mutex_lock(&mutex);
        // wait until the buffer has an element OR we have finished consuming matrices
        while (count == 0 && matricesConsumed < NUMBER_OF_MATRICES)
            pthread_cond_wait(&full, &mutex);

        // check afterward if we are done generating matrices & return
        if (matricesConsumed >= NUMBER_OF_MATRICES) {
            pthread_cond_signal(&full); // wake up other consumers to return
            pthread_mutex_unlock(&mutex);
            return stats;
        }

        m1 = get();
        // increment stats
        matricesConsumed++;
        stats->sumtotal += SumMatrix(m1);
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
        stats->matrixtotal++;

        // continue this loop while matrix multiplication fails
        while (m3 == NULL) {
            // lock for m2
            pthread_mutex_lock(&mutex);
            // wait until the buffer has an element OR we have finished consuming matrices
            while (count == 0 && matricesConsumed < NUMBER_OF_MATRICES)
                pthread_cond_wait(&full, &mutex);

            // check afterward if we are done generating matrices & return
            if (matricesConsumed >= NUMBER_OF_MATRICES) {
                pthread_cond_signal(&full); // wake up other consumers to return
                pthread_mutex_unlock(&mutex);
                FreeMatrix(m1); // no longer using m1
                return stats;
            }

            m2 = get();
            // increment stats
            matricesConsumed++;
            stats->sumtotal += SumMatrix(m2);
            pthread_cond_signal(&empty);
            pthread_mutex_unlock(&mutex);
            stats->matrixtotal++;
            m3 = MatrixMultiply(m1, m2);

            // if matrix multiplication fails, look for new m2
            if (m3 == NULL)
                FreeMatrix(m2);
        }

        // display the two matrices used for multiplication, increment stat, and free all matrices.
        DisplayMatrix(m1,stdout);
        printf("    X\n");
        DisplayMatrix(m2,stdout);
        printf("    =\n");
        DisplayMatrix(m3, stdout);
        printf("\n");
        stats->multtotal++;
        FreeMatrix(m1);
        FreeMatrix(m2);
        FreeMatrix(m3);
        m1 = m2 = m3 = NULL;
    }

    return stats;
}
