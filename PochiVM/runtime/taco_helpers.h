#ifndef TACO_C_HEADERS
#define TACO_C_HEADERS
#include <cmath>
#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "taco_wrappers.h"
// Necessary because the name 'I' is used for template parameters in Pochi
// source Instead use _Complex_I
#undef I

#include <string.h>
#ifndef TACO_TENSOR_T_DEFINED
#define TACO_TENSOR_T_DEFINED
// Pochivm doesn't work with enums so do it this way for now
const int taco_mode_dense = 0;
const int taco_mode_sparse = 1;
class taco_tensor_t {
public:
  int32_t order;          // tensor order (number of modes)
  int32_t *dimensions;    // tensor dimensions
  int32_t csize;          // component size
  int32_t *mode_ordering; // mode storage ordering
  int *mode_types;        // mode storage types
  uint8_t ***indices;     // tensor index data (per mode)
  uint8_t *vals;          // tensor values
  int32_t vals_size;      // values array size
};
#endif
int omp_get_thread_num();
int omp_get_max_threads();
int cmp(const void *a, const void *b);
int taco_binarySearchAfter(int *array, int arrayStart, int arrayEnd,
                           int target);
int taco_binarySearchBefore(int *array, int arrayStart, int arrayEnd,
                            int target);
taco_tensor_t *init_taco_tensor_t(int32_t order, int32_t csize,
                                  int32_t *dimensions, int32_t *mode_ordering,
                                  int *mode_types);
// Use void* stead of taco_tensor_t* because
void deinit_taco_tensor_t(void *t);
double sqrt_double(double a);
void *pochi_malloc(size_t size);
void *pochi_realloc(void *ptr, size_t size);
void *pochi_calloc(size_t nitems, size_t size);
void pochi_free(void *ptr);
void pochi_qsort(void *base, size_t nitems);
long pochi_bitand(long a, long b);
long pochi_bitor(long a, long b);
int min(int a, int b);
int max(int a, int b);
#endif
