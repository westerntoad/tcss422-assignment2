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
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_cond_t prod_condition = PTHREAD_COND_INITIALIZER;
pthread_cond_t cons_condition = PTHREAD_COND_INITIALIZER;
int temp = 0;
int matrices = 0;
int fill = 0;
int use = 0;
counters_t counters = {&(counter_t){0, PTHREAD_MUTEX_INITIALIZER}, &(counter_t){0, PTHREAD_MUTEX_INITIALIZER}};

// Bounded buffer put() get()
int put(Matrix * value)
{
  bigmatrix[fill] = value;
  fill = (fill + 1) % BOUNDED_BUFFER_SIZE;
  return 1;
}

Matrix * get()
{
  Matrix* matrix = bigmatrix[use];
  bigmatrix[use] = NULL;
  use = (use + 1) % BOUNDED_BUFFER_SIZE;
  return matrix;
}

// Matrix PRODUCER worker thread
void *prod_worker(void *arg)
{
  ProdConsStats* stats = (ProdConsStats*) malloc(sizeof(ProdConsStats));
  stats->matrixtotal = 0;
  stats->multtotal = 0;
  stats->sumtotal = 0;  
  Matrix* matrix;

  while (get_cnt(counters.prod) < NUMBER_OF_MATRICES) {

    matrix = GenMatrixRandom();
     
    pthread_mutex_lock(&mutex);

    // wait while the buffer is full
    while (matrices == BOUNDED_BUFFER_SIZE && get_cnt(counters.prod) < NUMBER_OF_MATRICES)
      pthread_cond_wait(&prod_condition, &mutex);

    // if NUMBER_OF_MATRCICES has been reached, then break
    if (get_cnt(counters.prod) == NUMBER_OF_MATRICES) {
      FreeMatrix(matrix);
      pthread_cond_broadcast(&prod_condition);
      pthread_cond_broadcast(&cons_condition);
      pthread_mutex_unlock(&mutex);
      break;
    }

    // add a matrix to the buffer, and signal consumers
    put(matrix);
    matrices++;
    stats->sumtotal += SumMatrix(matrix);
    stats->matrixtotal++;
    increment_cnt(counters.prod);
    pthread_cond_signal(&cons_condition);
    pthread_mutex_unlock(&mutex);
  }

  // shutdown producers

  return (void*) stats;
}

// Matrix CONSUMER worker thread
void *cons_worker(void *arg)
{
  ProdConsStats* stats = (ProdConsStats*) malloc(sizeof(ProdConsStats));
  stats->matrixtotal = 0;
  stats->multtotal = 0;
  stats->sumtotal = 0;
  Matrix* matrix_one = NULL;
  Matrix* matrix_two = NULL;
  Matrix* matrix_three = NULL;

  while (get_cnt(counters.cons) < NUMBER_OF_MATRICES) {

    if (matrix_three != NULL) {
      FreeMatrix(matrix_three);
      matrix_three = NULL;
    }
    pthread_mutex_lock(&mutex);

    // wait while buffer is empty
    while (matrices == 0 && get_cnt(counters.cons) < NUMBER_OF_MATRICES)
      pthread_cond_wait(&cons_condition, &mutex);

    if (get_cnt(counters.cons) == NUMBER_OF_MATRICES) {
      if (matrix_one != NULL) {
        FreeMatrix(matrix_one);
      }
      if (matrix_three != NULL) {
        FreeMatrix(matrix_three);
      }
      pthread_cond_broadcast(&prod_condition);
      pthread_cond_broadcast(&cons_condition);
      pthread_mutex_unlock(&mutex);
      break;
    }

    matrix_one = get();

    matrices--;
    pthread_cond_signal(&prod_condition);
    pthread_mutex_unlock(&mutex);

    stats->sumtotal += SumMatrix(matrix_one);
    stats->matrixtotal++;
    increment_cnt(counters.cons);

    while (matrix_three == NULL) {

      pthread_mutex_lock(&mutex);

      while (matrices == 0 && get_cnt(counters.cons) < NUMBER_OF_MATRICES) {
        pthread_cond_wait(&cons_condition, &mutex);
      }

      if (get_cnt(counters.cons) == NUMBER_OF_MATRICES) {
        pthread_mutex_unlock(&mutex);
        break;
      }

      matrix_two = get();
      matrices--;
      stats->matrixtotal++;
      pthread_cond_signal(&prod_condition);
      pthread_mutex_unlock(&mutex);
      increment_cnt(counters.cons);
      stats->sumtotal += SumMatrix(matrix_two);
      matrix_three = MatrixMultiply(matrix_one, matrix_two);
      FreeMatrix(matrix_two);
      if (matrix_three != NULL) {
        FreeMatrix(matrix_one);
        matrix_one = NULL;
        stats->multtotal++;
      }
    }
  }
  
  return (void*) stats;
}
