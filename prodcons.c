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
    

    matrix = GenMatrixRandom(matrix);
    
    pthread_mutex_lock(&mutex);
    while (matrices >= BOUNDED_BUFFER_SIZE) {
        pthread_cond_wait(&prod_condition, &mutex);
    }
    if (matrices < BOUNDED_BUFFER_SIZE) {
        matrices++;
        put(matrix);
        stats->sumtotal += SumMatrix(matrix);
        stats->matrixtotal++;
        increment_cnt(counters.prod);
        pthread_cond_signal(&cons_condition);
    } else {
        FreeMatrix(matrix);
        pthread_cond_signal(&cons_condition);
    }
    pthread_mutex_unlock(&mutex);
}
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

  while (get_cnt(counters.cons) < NUMBER_OF_MATRICES)
  {
    Matrix* matrix_three = NULL;
    pthread_mutex_lock(&mutex);
    while (matrices < 1)
      pthread_cond_wait(&cons_condition, &mutex);
    if (matrices > 0) {
      matrix_one = get();
      stats->sumtotal += SumMatrix(matrix_one);
      stats->matrixtotal++;
      increment_cnt(counters.cons);
      matrices--;
    }
    pthread_cond_signal(&prod_condition);
    pthread_mutex_unlock(&mutex);

    pthread_mutex_lock(&mutex);
    while (matrix_three == NULL) {
      if (matrices < 1 && get_cnt(counters.prod) >= NUMBER_OF_MATRICES) break;
      while (matrices < 1) {
        pthread_cond_wait(&cons_condition, &mutex);
      }
      if (matrices > 0) {
        matrix_two = get();
        stats->sumtotal += SumMatrix(matrix_two);
        stats->matrixtotal++;
        matrices--;
        increment_cnt(counters.cons);
        if (matrix_two != NULL) {
          matrix_three = MatrixMultiply(matrix_one, matrix_two);
          FreeMatrix(matrix_two);
        }
      }
    }
    pthread_cond_signal(&prod_condition);
    pthread_mutex_unlock(&mutex);
    if (matrix_three != NULL) {
      stats->multtotal++;
      free(matrix_three);
    }
    FreeMatrix(matrix_one);
  }
  return (void*) stats;
}
