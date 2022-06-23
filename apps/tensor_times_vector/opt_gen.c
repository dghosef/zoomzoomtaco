#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <complex.h>
#include <string.h>
#if _OPENMP
#include <omp.h>
#endif
#define TACO_MIN(_a,_b) ((_a) < (_b) ? (_a) : (_b))
#define TACO_MAX(_a,_b) ((_a) > (_b) ? (_a) : (_b))
#define TACO_DEREF(_a) (((___context___*)(*__ctx__))->_a)
#ifndef TACO_TENSOR_T_DEFINED
#define TACO_TENSOR_T_DEFINED
typedef enum { taco_mode_dense, taco_mode_sparse } taco_mode_t;
typedef struct {
  int32_t      order;         // tensor order (number of modes)
  int32_t*     dimensions;    // tensor dimensions
  int32_t      csize;         // component size
  int32_t*     mode_ordering; // mode storage ordering
  taco_mode_t* mode_types;    // mode storage types
  uint8_t***   indices;       // tensor index data (per mode)
  uint8_t*     vals;          // tensor values
  uint8_t*     fill_value;    // tensor fill value
  int32_t      vals_size;     // values array size
} taco_tensor_t;
#endif
int compute(taco_tensor_t *A121, taco_tensor_t *pwtk, taco_tensor_t *A65) {
  int A1211_dimension = (int)(A121->dimensions[0]);
  double* restrict A121_vals = (double*)(A121->vals);
  int pwtk1_dimension = (int)(pwtk->dimensions[0]);
  int* restrict pwtk2_pos = (int*)(pwtk->indices[1][0]);
  int* restrict pwtk2_crd = (int*)(pwtk->indices[1][1]);
  double* restrict pwtk_vals = (double*)(pwtk->vals);
  int A651_dimension = (int)(A65->dimensions[0]);
  double* restrict A65_vals = (double*)(A65->vals);

  #pragma omp parallel for schedule(runtime)
  for (int32_t i124 = 0; i124 < pwtk1_dimension; i124++) {
    double ti125A121_val = 0.0;
    for (int32_t i125pwtk = pwtk2_pos[i124]; i125pwtk < pwtk2_pos[(i124 + 1)]; i125pwtk++) {
      int32_t i125 = pwtk2_crd[i125pwtk];
      ti125A121_val += pwtk_vals[i125pwtk] * A65_vals[i125];
    }
    A121_vals[i124] = ti125A121_val;
  }
  return 0;
}