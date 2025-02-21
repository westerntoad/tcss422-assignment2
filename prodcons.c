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
    counter_t* ctr = (counter_t*) arg;
    ProdConsStats* stats = (ProdConsStats*) malloc(sizeof(ProdConsStats));
    stats->sumtotal = 0;
    stats->multtotal = 0;
    stats->matrixtotal = 0;

    while (1) {
        Matrix* m = GenMatrixRandom();
        pthread_mutex_lock(&mutex);
        while (count == BOUNDED_BUFFER_SIZE)
            pthread_cond_wait(&empty, &mutex);

        if (get_cnt(ctr) >= NUMBER_OF_MATRICES) {
            pthread_cond_signal(&full);
            pthread_mutex_unlock(&mutex);
            return stats;
        }
        put(m);
        increment_cnt(ctr);
        pthread_cond_signal(&full);
        pthread_mutex_unlock(&mutex);

        stats->sumtotal += SumMatrix(m);
        stats->matrixtotal++;
    }
    
    return stats;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg) {
    counter_t* ctr = (counter_t*) arg;
    ProdConsStats* stats = (ProdConsStats*) malloc(sizeof(ProdConsStats));
    Matrix* m1 = NULL;
    Matrix* m2 = NULL;
    Matrix* m3 = NULL;
    stats->sumtotal = 0;
    stats->multtotal = 0;
    stats->matrixtotal = 0;

    while (get_cnt(ctr) < NUMBER_OF_MATRICES) {
        // get m1
        pthread_mutex_lock(&mutex);
        while (count == 0)
            pthread_cond_wait(&full, &mutex);
        m1 = get();
        pthread_cond_signal(&empty);
        pthread_mutex_unlock(&mutex);
        stats->matrixtotal++;
        stats->sumtotal += SumMatrix(m1);
        increment_cnt(ctr);

        // get m2
        while (m3 == NULL) {
            if (get_cnt(ctr) >= NUMBER_OF_MATRICES) {
                FreeMatrix(m1);
                return stats;
            }

            pthread_mutex_lock(&mutex);
            while (count == 0)
                pthread_cond_wait(&full, &mutex);
            m2 = get();
            pthread_cond_signal(&empty);
            pthread_mutex_unlock(&mutex);
            stats->matrixtotal++;
            stats->sumtotal += SumMatrix(m2);
            increment_cnt(ctr);
            m3 = MatrixMultiply(m1, m2);
            stats->multtotal++;

            if (m3 == NULL)
                FreeMatrix(m2);
        }

        DisplayMatrix(m3, stdout);
        FreeMatrix(m1);
        FreeMatrix(m2);
        FreeMatrix(m3);
        m1 = NULL;
        m2 = NULL;
        m3 = NULL;
    }

    return stats;
}
