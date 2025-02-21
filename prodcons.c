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
int matricesProduced = 0;
int matricesConsumed = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;



// Bounded buffer put() get()
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
    ProdConsStats* stats = (ProdConsStats*) malloc(sizeof(ProdConsStats));
    stats->sumtotal = 0;
    stats->multtotal = 0;
    stats->matrixtotal = 0;


    while (1) {
        Matrix* m = GenMatrixRandom();
        pthread_mutex_lock(&mutex);
        while (count == BOUNDED_BUFFER_SIZE)
            pthread_cond_wait(&empty, &mutex);

        if (matricesProduced >= NUMBER_OF_MATRICES) {
            pthread_cond_signal(&empty);
            pthread_mutex_unlock(&mutex);
            break;
        }
        put(m);
        matricesProduced++;
        stats->sumtotal += SumMatrix(m);
        pthread_cond_signal(&full);
        pthread_mutex_unlock(&mutex);

        stats->matrixtotal++;
    }
    
    return stats;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg) {
    ProdConsStats* stats = (ProdConsStats*) malloc(sizeof(ProdConsStats));
    Matrix *m1, *m2, *m3;
    stats->sumtotal = 0;
    stats->multtotal = 0;
    stats->matrixtotal = 0;

    while (1) {
        // get m1
        pthread_mutex_lock(&mutex);
        while (count == 0 && matricesConsumed < NUMBER_OF_MATRICES)
            pthread_cond_wait(&full, &mutex);

        if (matricesConsumed >= NUMBER_OF_MATRICES) {
            pthread_cond_signal(&full);
            pthread_mutex_unlock(&mutex);
            return stats;
        }

        m1 = get();
        matricesConsumed++;
        stats->sumtotal += SumMatrix(m1);
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
        stats->matrixtotal++;

        // get m2
        while (m3 == NULL) {
            pthread_mutex_lock(&mutex);
            while (count == 0 && matricesConsumed < NUMBER_OF_MATRICES)
                pthread_cond_wait(&full, &mutex);

            if (matricesConsumed >= NUMBER_OF_MATRICES) {
                pthread_cond_signal(&full);
                pthread_mutex_unlock(&mutex);
                FreeMatrix(m1);
                return stats;
            }

            m2 = get();
            matricesConsumed++;
            stats->sumtotal += SumMatrix(m2);
            pthread_cond_signal(&empty);
            pthread_mutex_unlock(&mutex);
            stats->matrixtotal++;
            m3 = MatrixMultiply(m1, m2);

            if (m3 == NULL)
                FreeMatrix(m2);
        }

        //DisplayMatrix(m3, stdout);
        stats->multtotal++;
        FreeMatrix(m1);
        FreeMatrix(m2);
        FreeMatrix(m3);
        m1 = m2 = m3 = NULL;
    }

    return stats;
}
