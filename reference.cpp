#ifndef TACO_C_HEADERS
#define TACO_C_HEADERS
#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if _OPENMP
#include <omp.h>
#endif
#define TACO_MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define TACO_MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#define TACO_DEREF(_a) (((___context___ *)(*__ctx__))->_a)
#ifndef TACO_TENSOR_T_DEFINED
#define TACO_TENSOR_T_DEFINED
typedef enum { taco_mode_dense, taco_mode_sparse } taco_mode_t;
typedef struct {
  int32_t order;           // tensor order (number of modes)
  int32_t *dimensions;     // tensor dimensions
  int32_t csize;           // component size
  int32_t *mode_ordering;  // mode storage ordering
  taco_mode_t *mode_types; // mode storage types
  uint8_t ***indices;      // tensor index data (per mode)
  uint8_t *vals;           // tensor values
  int32_t vals_size;       // values array size
} taco_tensor_t;
#endif
#if !_OPENMP
int omp_get_thread_num() { return 0; }
int omp_get_max_threads() { return 1; }
#endif
int cmp(const void *a, const void *b) {
  return *((const int *)a) - *((const int *)b);
}
int taco_binarySearchAfter(int *array, int arrayStart, int arrayEnd,
                           int target) {
  if (array[arrayStart] >= target) {
    return arrayStart;
  }
  int lowerBound = arrayStart; // always < target
  int upperBound = arrayEnd;   // always >= target
  while (upperBound - lowerBound > 1) {
    int mid = (upperBound + lowerBound) / 2;
    int midValue = array[mid];
    if (midValue < target) {
      lowerBound = mid;
    } else if (midValue > target) {
      upperBound = mid;
    } else {
      return mid;
    }
  }
  return upperBound;
}
int taco_binarySearchBefore(int *array, int arrayStart, int arrayEnd,
                            int target) {
  if (array[arrayEnd] <= target) {
    return arrayEnd;
  }
  int lowerBound = arrayStart; // always <= target
  int upperBound = arrayEnd;   // always > target
  while (upperBound - lowerBound > 1) {
    int mid = (upperBound + lowerBound) / 2;
    int midValue = array[mid];
    if (midValue < target) {
      lowerBound = mid;
    } else if (midValue > target) {
      upperBound = mid;
    } else {
      return mid;
    }
  }
  return lowerBound;
}
taco_tensor_t *init_taco_tensor_t(int32_t order, int32_t csize,
                                  int32_t *dimensions, int32_t *mode_ordering,
                                  taco_mode_t *mode_types) {
  taco_tensor_t *t = (taco_tensor_t *)malloc(sizeof(taco_tensor_t));
  t->order = order;
  t->dimensions = (int32_t *)malloc(order * sizeof(int32_t));
  t->mode_ordering = (int32_t *)malloc(order * sizeof(int32_t));
  t->mode_types = (taco_mode_t *)malloc(order * sizeof(taco_mode_t));
  t->indices = (uint8_t ***)malloc(order * sizeof(uint8_t ***));
  t->csize = csize;
  for (int32_t i = 0; i < order; i++) {
    t->dimensions[i] = dimensions[i];
    t->mode_ordering[i] = mode_ordering[i];
    t->mode_types[i] = mode_types[i];
    switch (t->mode_types[i]) {
    case taco_mode_dense:
      t->indices[i] = (uint8_t **)malloc(1 * sizeof(uint8_t **));
      break;
    case taco_mode_sparse:
      t->indices[i] = (uint8_t **)malloc(2 * sizeof(uint8_t **));
      break;
    }
  }
  return t;
}
void deinit_taco_tensor_t(taco_tensor_t *t) {
  for (int i = 0; i < t->order; i++) {
    free(t->indices[i]);
  }
  free(t->indices);
  free(t->dimensions);
  free(t->mode_ordering);
  free(t->mode_types);
  free(t);
}
#endif

int assemble(taco_tensor_t *A, taco_tensor_t *B, taco_tensor_t *c) {
  int *restrict A2_pos = (int *)(A->indices[1][0]);
  int *restrict A2_crd = (int *)(A->indices[1][1]);
  double *restrict A_vals = (double *)(A->vals);
  int *restrict B1_pos = (int *)(B->indices[0][0]);
  int *restrict B1_crd = (int *)(B->indices[0][1]);
  int *restrict B2_pos = (int *)(B->indices[1][0]);
  int *restrict B2_crd = (int *)(B->indices[1][1]);

  A2_pos = (int32_t *)malloc(sizeof(int32_t) * 3);
  A2_pos[0] = 0;
  for (int32_t pA2 = 1; pA2 < 3; pA2++) {
    A2_pos[pA2] = 0;
  }
  int32_t A2_crd_size = 1048576;
  A2_crd = (int32_t *)malloc(sizeof(int32_t) * A2_crd_size);
  int32_t i10A = 0;

  for (int32_t i9B = B1_pos[0]; i9B < B1_pos[1]; i9B++) {
    int32_t i9 = B1_crd[i9B];
    int32_t pA2_begin = i10A;

    for (int32_t i10B = B2_pos[i9B]; i10B < B2_pos[(i9B + 1)]; i10B++) {
      int32_t i10 = B2_crd[i10B];
      if (A2_crd_size <= i10A) {
        A2_crd =
            (int32_t *)realloc(A2_crd, sizeof(int32_t) * (A2_crd_size * 2));
        A2_crd_size *= 2;
      }
      A2_crd[i10A] = i10;
      i10A++;
    }

    A2_pos[i9 + 1] = i10A - pA2_begin;
  }

  int32_t csA2 = 0;
  for (int32_t pA20 = 1; pA20 < 3; pA20++) {
    csA2 += A2_pos[pA20];
    A2_pos[pA20] = csA2;
  }

  A_vals = (double *)malloc(sizeof(double) * i10A);

  A->indices[1][0] = (uint8_t *)(A2_pos);
  A->indices[1][1] = (uint8_t *)(A2_crd);
  A->vals = (uint8_t *)A_vals;
  return 0;
}

int compute(taco_tensor_t *A, taco_tensor_t *B, taco_tensor_t *c) {
  double *restrict A_vals = (double *)(A->vals);
  int *restrict B1_pos = (int *)(B->indices[0][0]);
  int *restrict B1_crd = (int *)(B->indices[0][1]);
  int *restrict B2_pos = (int *)(B->indices[1][0]);
  int *restrict B2_crd = (int *)(B->indices[1][1]);
  int *restrict B3_pos = (int *)(B->indices[2][0]);
  int *restrict B3_crd = (int *)(B->indices[2][1]);
  double *restrict B_vals = (double *)(B->vals);
  int *restrict c1_pos = (int *)(c->indices[0][0]);
  int *restrict c1_crd = (int *)(c->indices[0][1]);
  double *restrict c_vals = (double *)(c->vals);

  int32_t i10A = 0;

  for (int32_t i9B = B1_pos[0]; i9B < B1_pos[1]; i9B++) {
    for (int32_t i10B = B2_pos[i9B]; i10B < B2_pos[(i9B + 1)]; i10B++) {
      double ti11A_val = 0.0;
      int32_t i11B = B3_pos[i10B];
      int32_t pB3_end = B3_pos[(i10B + 1)];
      int32_t i11c = c1_pos[0];
      int32_t pc1_end = c1_pos[1];

      while (i11B < pB3_end && i11c < pc1_end) {
        int32_t i11B0 = B3_crd[i11B];
        int32_t i11c0 = c1_crd[i11c];
        int32_t i11 = TACO_MIN(i11B0, i11c0);
        if (i11B0 == i11 && i11c0 == i11) {
          ti11A_val += B_vals[i11B] * c_vals[i11c];
        }
        i11B += (int32_t)(i11B0 == i11);
        i11c += (int32_t)(i11c0 == i11);
      }
      A_vals[i10A] = ti11A_val;
      i10A++;
    }
  }
  return 0;
}

#ifndef TACO_C_HEADERS
#define TACO_C_HEADERS
#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if _OPENMP
#include <omp.h>
#endif
#define TACO_MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define TACO_MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#define TACO_DEREF(_a) (((___context___ *)(*__ctx__))->_a)
#ifndef TACO_TENSOR_T_DEFINED
#define TACO_TENSOR_T_DEFINED
typedef enum { taco_mode_dense, taco_mode_sparse } taco_mode_t;
typedef struct {
  int32_t order;           // tensor order (number of modes)
  int32_t *dimensions;     // tensor dimensions
  int32_t csize;           // component size
  int32_t *mode_ordering;  // mode storage ordering
  taco_mode_t *mode_types; // mode storage types
  uint8_t ***indices;      // tensor index data (per mode)
  uint8_t *vals;           // tensor values
  int32_t vals_size;       // values array size
} taco_tensor_t;
#endif
#if !_OPENMP
int omp_get_thread_num() { return 0; }
int omp_get_max_threads() { return 1; }
#endif
int cmp(const void *a, const void *b) {
  return *((const int *)a) - *((const int *)b);
}
int taco_binarySearchAfter(int *array, int arrayStart, int arrayEnd,
                           int target) {
  if (array[arrayStart] >= target) {
    return arrayStart;
  }
  int lowerBound = arrayStart; // always < target
  int upperBound = arrayEnd;   // always >= target
  while (upperBound - lowerBound > 1) {
    int mid = (upperBound + lowerBound) / 2;
    int midValue = array[mid];
    if (midValue < target) {
      lowerBound = mid;
    } else if (midValue > target) {
      upperBound = mid;
    } else {
      return mid;
    }
  }
  return upperBound;
}
int taco_binarySearchBefore(int *array, int arrayStart, int arrayEnd,
                            int target) {
  if (array[arrayEnd] <= target) {
    return arrayEnd;
  }
  int lowerBound = arrayStart; // always <= target
  int upperBound = arrayEnd;   // always > target
  while (upperBound - lowerBound > 1) {
    int mid = (upperBound + lowerBound) / 2;
    int midValue = array[mid];
    if (midValue < target) {
      lowerBound = mid;
    } else if (midValue > target) {
      upperBound = mid;
    } else {
      return mid;
    }
  }
  return lowerBound;
}
taco_tensor_t *init_taco_tensor_t(int32_t order, int32_t csize,
                                  int32_t *dimensions, int32_t *mode_ordering,
                                  taco_mode_t *mode_types) {
  taco_tensor_t *t = (taco_tensor_t *)malloc(sizeof(taco_tensor_t));
  t->order = order;
  t->dimensions = (int32_t *)malloc(order * sizeof(int32_t));
  t->mode_ordering = (int32_t *)malloc(order * sizeof(int32_t));
  t->mode_types = (taco_mode_t *)malloc(order * sizeof(taco_mode_t));
  t->indices = (uint8_t ***)malloc(order * sizeof(uint8_t ***));
  t->csize = csize;
  for (int32_t i = 0; i < order; i++) {
    t->dimensions[i] = dimensions[i];
    t->mode_ordering[i] = mode_ordering[i];
    t->mode_types[i] = mode_types[i];
    switch (t->mode_types[i]) {
    case taco_mode_dense:
      t->indices[i] = (uint8_t **)malloc(1 * sizeof(uint8_t **));
      break;
    case taco_mode_sparse:
      t->indices[i] = (uint8_t **)malloc(2 * sizeof(uint8_t **));
      break;
    }
  }
  return t;
}
void deinit_taco_tensor_t(taco_tensor_t *t) {
  for (int i = 0; i < t->order; i++) {
    free(t->indices[i]);
  }
  free(t->indices);
  free(t->dimensions);
  free(t->mode_ordering);
  free(t->mode_types);
  free(t);
}
#endif

int pack(taco_tensor_t *A201, taco_tensor_t *A200) {
  int *restrict A2011_pos = (int *)(A201->indices[0][0]);
  int *restrict A2011_crd = (int *)(A201->indices[0][1]);
  int *restrict A2012_pos = (int *)(A201->indices[1][0]);
  int *restrict A2012_crd = (int *)(A201->indices[1][1]);
  int *restrict A2013_pos = (int *)(A201->indices[2][0]);
  int *restrict A2013_crd = (int *)(A201->indices[2][1]);
  double *restrict A201_vals = (double *)(A201->vals);
  int *restrict A2001_pos = (int *)(A200->indices[0][0]);
  int *restrict A2001_crd = (int *)(A200->indices[0][1]);
  int *restrict A2002_crd = (int *)(A200->indices[1][1]);
  int *restrict A2003_crd = (int *)(A200->indices[2][1]);
  double *restrict A200_vals = (double *)(A200->vals);

  A2011_pos = (int32_t *)malloc(sizeof(int32_t) * 2);
  A2011_pos[0] = 0;
  int32_t A2011_crd_size = 1048576;
  A2011_crd = (int32_t *)malloc(sizeof(int32_t) * A2011_crd_size);
  int32_t i202A201 = 0;
  int32_t A2012_pos_size = 1048576;
  A2012_pos = (int32_t *)malloc(sizeof(int32_t) * A2012_pos_size);
  A2012_pos[0] = 0;
  int32_t A2012_crd_size = 1048576;
  A2012_crd = (int32_t *)malloc(sizeof(int32_t) * A2012_crd_size);
  int32_t i203A201 = 0;
  int32_t A2013_pos_size = 1048576;
  A2013_pos = (int32_t *)malloc(sizeof(int32_t) * A2013_pos_size);
  A2013_pos[0] = 0;
  int32_t A2013_crd_size = 1048576;
  A2013_crd = (int32_t *)malloc(sizeof(int32_t) * A2013_crd_size);
  int32_t i204A201 = 0;
  int32_t A201_capacity = 1048576;
  A201_vals = (double *)malloc(sizeof(double) * A201_capacity);

  int32_t i202A200 = A2001_pos[0];
  int32_t pA2001_end = A2001_pos[1];

  while (i202A200 < pA2001_end) {
    int32_t i202 = A2001_crd[i202A200];
    int32_t A2001_segend = i202A200 + 1;
    while (A2001_segend < pA2001_end && A2001_crd[A2001_segend] == i202) {
      A2001_segend++;
    }
    int32_t pA2012_begin = i203A201;
    if (A2012_pos_size <= i202A201 + 1) {
      A2012_pos =
          (int32_t *)realloc(A2012_pos, sizeof(int32_t) * (A2012_pos_size * 2));
      A2012_pos_size *= 2;
    }

    int32_t i203A200 = i202A200;

    while (i203A200 < A2001_segend) {
      int32_t i203 = A2002_crd[i203A200];
      int32_t A2002_segend = i203A200 + 1;
      while (A2002_segend < A2001_segend && A2002_crd[A2002_segend] == i203) {
        A2002_segend++;
      }
      int32_t pA2013_begin = i204A201;
      if (A2013_pos_size <= i203A201 + 1) {
        A2013_pos = (int32_t *)realloc(A2013_pos,
                                       sizeof(int32_t) * (A2013_pos_size * 2));
        A2013_pos_size *= 2;
      }
`
      int32_t i204A200 = i203A200;
`
      while (i204A200 < A2002_segend) {
        int32_t i204 = A2003_crd[i204A200];
        double A200_val = A200_vals[i204A200];
        i204A200++;
        while (i204A200 < A2002_segend && A2003_crd[i204A200] == i204) {
          A200_val += A200_vals[i204A200];
          i204A200++;
        }
        if (A201_capacity <= i204A201) {
          A201_vals = (double *)realloc(A201_vals,
                                        sizeof(double) * (A201_capacity * 2));
          A201_capacity *= 2;
        }
        A201_vals[i204A201] = A200_val;
        if (A2013_crd_size <= i204A201) {
          A2013_crd = (int32_t *)realloc(A2013_crd, sizeof(int32_t) *
                                                        (A2013_crd_size * 2));
          A2013_crd_size *= 2;
        }
        A2013_crd[i204A201] = i204;
        i204A201++;
      }
`
      A2013_pos[i203A201 + 1] = i204A201;
      if (pA2013_begin < i204A201) {
        if (A2012_crd_size <= i203A201) {
          A2012_crd = (int32_t *)realloc(A2012_crd, sizeof(int32_t) *
                                                        (A2012_crd_size * 2));
          A2012_crd_size *= 2;
        }
        A2012_crd[i203A201] = i203;
        i203A201++;
      }
      i203A200 = A2002_segend;
    }

-
    A2012_pos[i202A201 + 1] = i203A201;
    if (pA2012_begin < i203A201) {
      if (A2011_crd_size <= i202A201) {
        A2011_crd = (int32_t *)realloc(A2011_crd,
                                       sizeof(int32_t) * (A2011_crd_size * 2));
        A2011_crd_size *= 2;
      }
      A2011_crd[i202A201] = i202;
      i202A201++;
    }
    i202A200 = A2001_segend;
  }

  A2011_pos[1] = i202A201;

  A201->indices[0][0] = (uint8_t *)(A2011_pos);
  A201->indices[0][1] = (uint8_t *)(A2011_crd);
  A201->indices[1][0] = (uint8_t *)(A2012_pos);
  A201->indices[1][1] = (uint8_t *)(A2012_crd);
  A201->indices[2][0] = (uint8_t *)(A2013_pos);
  A201->indices[2][1] = (uint8_t *)(A2013_crd);
  A201->vals = (uint8_t *)A201_vals;
  return 0;
}

int iterate(void **__ctx__, char *__coords__, double *__val__,
            int32_t *__bufcap__, taco_tensor_t *A201) {
  int *restrict A2011_pos = (int *)(A201->indices[0][0]);
  int *restrict A2011_crd = (int *)(A201->indices[0][1]);
  int *restrict A2012_pos = (int *)(A201->indices[1][0]);
  int *restrict A2012_crd = (int *)(A201->indices[1][1]);
  int *restrict A2013_pos = (int *)(A201->indices[2][0]);
  int *restrict A2013_crd = (int *)(A201->indices[2][1]);
  double *restrict A201_vals = (double *)(A201->vals);

  typedef struct ___context___ {
    int32_t size;
    int32_t state;
    int32_t i202A201;
    int32_t i202;
    int32_t i203A201;
    int32_t i203;
    int32_t i204A201;
    int32_t i204;
  } ___context___;
  int32_t i202A201;
  int32_t i202;
  int32_t i203A201;
  int32_t i203;
  int32_t i204A201;
  int32_t i204;
  int32_t __bufsize__ = 0;
  int32_t __bufcapcopy__ = *__bufcap__;
  if (*__ctx__) {
    i202A201 = TACO_DEREF(i202A201);
    i202 = TACO_DEREF(i202);
    i203A201 = TACO_DEREF(i203A201);
    i203 = TACO_DEREF(i203);
    i204A201 = TACO_DEREF(i204A201);
    i204 = TACO_DEREF(i204);
    switch (TACO_DEREF(state)) {
    case 0:
      goto resume_iterate0;
    case 1:
      goto resume_iterate1;
    }
  } else {
    *__ctx__ = malloc(sizeof(___context___));
    TACO_DEREF(size) = sizeof(___context___);
  }

  for (i202A201 = A2011_pos[0]; i202A201 < A2011_pos[1]; i202A201++) {
    i202 = A2011_crd[i202A201];
    for (i203A201 = A2012_pos[i202A201]; i203A201 < A2012_pos[(i202A201 + 1)];
         i203A201++) {
      i203 = A2012_crd[i203A201];
      for (i204A201 = A2013_pos[i203A201]; i204A201 < A2013_pos[(i203A201 + 1)];
           i204A201++) {
        i204 = A2013_crd[i204A201];
        *(int32_t *)(__coords__ + 12 * __bufsize__) = i202;
        *(int32_t *)(__coords__ + 12 * __bufsize__ + 4) = i203;
        *(int32_t *)(__coords__ + 12 * __bufsize__ + 8) = i204;
        __val__[__bufsize__] = A201_vals[i204A201];
        if (++__bufsize__ == __bufcapcopy__) {
          TACO_DEREF(i202A201) = i202A201;
          TACO_DEREF(i202) = i202;
          TACO_DEREF(i203A201) = i203A201;
          TACO_DEREF(i203) = i203;
          TACO_DEREF(i204A201) = i204A201;
          TACO_DEREF(i204) = i204;
          TACO_DEREF(state) = 0;
          return __bufsize__;
        }
      resume_iterate0:;
      }
    }
  }
  TACO_DEREF(state) = 1;
  return __bufsize__;
  if (__bufsize__ > 0) {
  }
resume_iterate1:
  free(*__ctx__);
  *__ctx__ = NULL;
  return 0;
}

#ifndef TACO_C_HEADERS
#define TACO_C_HEADERS
#include <complex.h>
#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if _OPENMP
#include <omp.h>
#endif
#define TACO_MIN(_a, _b) ((_a) < (_b) ? (_a) : (_b))
#define TACO_MAX(_a, _b) ((_a) > (_b) ? (_a) : (_b))
#define TACO_DEREF(_a) (((___context___ *)(*__ctx__))->_a)
#ifndef TACO_TENSOR_T_DEFINED
#define TACO_TENSOR_T_DEFINED
typedef enum { taco_mode_dense, taco_mode_sparse } taco_mode_t;
typedef struct {
  int32_t order;           // tensor order (number of modes)
  int32_t *dimensions;     // tensor dimensions
  int32_t csize;           // component size
  int32_t *mode_ordering;  // mode storage ordering
  taco_mode_t *mode_types; // mode storage types
  uint8_t ***indices;      // tensor index data (per mode)
  uint8_t *vals;           // tensor values
  int32_t vals_size;       // values array size
} taco_tensor_t;
#endif
#if !_OPENMP
int omp_get_thread_num() { return 0; }
int omp_get_max_threads() { return 1; }
#endif
int cmp(const void *a, const void *b) {
  return *((const int *)a) - *((const int *)b);
}
int taco_binarySearchAfter(int *array, int arrayStart, int arrayEnd,
                           int target) {
  if (array[arrayStart] >= target) {
    return arrayStart;
  }
  int lowerBound = arrayStart; // always < target
  int upperBound = arrayEnd;   // always >= target
  while (upperBound - lowerBound > 1) {
    int mid = (upperBound + lowerBound) / 2;
    int midValue = array[mid];
    if (midValue < target) {
      lowerBound = mid;
    } else if (midValue > target) {
      upperBound = mid;
    } else {
      return mid;
    }
  }
  return upperBound;
}
int taco_binarySearchBefore(int *array, int arrayStart, int arrayEnd,
                            int target) {
  if (array[arrayEnd] <= target) {
    return arrayEnd;
  }
  int lowerBound = arrayStart; // always <= target
  int upperBound = arrayEnd;   // always > target
  while (upperBound - lowerBound > 1) {
    int mid = (upperBound + lowerBound) / 2;
    int midValue = array[mid];
    if (midValue < target) {
      lowerBound = mid;
    } else if (midValue > target) {
      upperBound = mid;
    } else {
      return mid;
    }
  }
  return lowerBound;
}
taco_tensor_t *init_taco_tensor_t(int32_t order, int32_t csize,
                                  int32_t *dimensions, int32_t *mode_ordering,
                                  taco_mode_t *mode_types) {
  taco_tensor_t *t = (taco_tensor_t *)malloc(sizeof(taco_tensor_t));
  t->order = order;
  t->dimensions = (int32_t *)malloc(order * sizeof(int32_t));
  t->mode_ordering = (int32_t *)malloc(order * sizeof(int32_t));
  t->mode_types = (taco_mode_t *)malloc(order * sizeof(taco_mode_t));
  t->indices = (uint8_t ***)malloc(order * sizeof(uint8_t ***));
  t->csize = csize;
  for (int32_t i = 0; i < order; i++) {
    t->dimensions[i] = dimensions[i];
    t->mode_ordering[i] = mode_ordering[i];
    t->mode_types[i] = mode_types[i];
    switch (t->mode_types[i]) {
    case taco_mode_dense:
      t->indices[i] = (uint8_t **)malloc(1 * sizeof(uint8_t **));
      break;
    case taco_mode_sparse:
      t->indices[i] = (uint8_t **)malloc(2 * sizeof(uint8_t **));
      break;
    }
  }
  return t;
}
void deinit_taco_tensor_t(taco_tensor_t *t) {
  for (int i = 0; i < t->order; i++) {
    free(t->indices[i]);
  }
  free(t->indices);
  free(t->dimensions);
  free(t->mode_ordering);
  free(t->mode_types);
  free(t);
}
#endif

int pack(taco_tensor_t *A277, taco_tensor_t *A276) {
  int *restrict A2771_pos = (int *)(A277->indices[0][0]);
  int *restrict A2771_crd = (int *)(A277->indices[0][1]);
  double *restrict A277_vals = (double *)(A277->vals);
  int *restrict A2761_pos = (int *)(A276->indices[0][0]);
  int *restrict A2761_crd = (int *)(A276->indices[0][1]);
  double *restrict A276_vals = (double *)(A276->vals);

  A2771_pos = (int32_t *)malloc(sizeof(int32_t) * 2);
  A2771_pos[0] = 0;
  int32_t A2771_crd_size = 1048576;
  A2771_crd = (int32_t *)malloc(sizeof(int32_t) * A2771_crd_size);
  int32_t i278A277 = 0;
  int32_t A277_capacity = 1048576;
  A277_vals = (double *)malloc(sizeof(double) * A277_capacity);

  int32_t i278A276 = A2761_pos[0];
  int32_t pA2761_end = A2761_pos[1];

  while (i278A276 < pA2761_end) {
    int32_t i278 = A2761_crd[i278A276];
    double A276_val = A276_vals[i278A276];
    i278A276++;
    while (i278A276 < pA2761_end && A2761_crd[i278A276] == i278) {
      A276_val += A276_vals[i278A276];
      i278A276++;
    }
    if (A277_capacity <= i278A277) {
      A277_vals =
          (double *)realloc(A277_vals, sizeof(double) * (A277_capacity * 2));
      A277_capacity *= 2;
    }
    A277_vals[i278A277] = A276_val;
    if (A2771_crd_size <= i278A277) {
      A2771_crd =
          (int32_t *)realloc(A2771_crd, sizeof(int32_t) * (A2771_crd_size * 2));
      A2771_crd_size *= 2;
    }
    A2771_crd[i278A277] = i278;
    i278A277++;
  }

  A2771_pos[1] = i278A277;

  A277->indices[0][0] = (uint8_t *)(A2771_pos);
  A277->indices[0][1] = (uint8_t *)(A2771_crd);
  A277->vals = (uint8_t *)A277_vals;
  return 0;
}

int iterate(void **__ctx__, char *__coords__, double *__val__,
            int32_t *__bufcap__, taco_tensor_t *A277) {
  int *restrict A2771_pos = (int *)(A277->indices[0][0]);
  int *restrict A2771_crd = (int *)(A277->indices[0][1]);
  double *restrict A277_vals = (double *)(A277->vals);

  typedef struct ___context___ {
    int32_t size;
    int32_t state;
    int32_t i278A277;
    int32_t i278;
  } ___context___;
  int32_t i278A277;
  int32_t i278;
  int32_t __bufsize__ = 0;
  int32_t __bufcapcopy__ = *__bufcap__;
  if (*__ctx__) {
    i278A277 = TACO_DEREF(i278A277);
    i278 = TACO_DEREF(i278);
    switch (TACO_DEREF(state)) {
    case 0:
      goto resume_iterate0;
    case 1:
      goto resume_iterate1;
    }
  } else {
    *__ctx__ = malloc(sizeof(___context___));
    TACO_DEREF(size) = sizeof(___context___);
  }

  for (i278A277 = A2771_pos[0]; i278A277 < A2771_pos[1]; i278A277++) {
    i278 = A2771_crd[i278A277];
    *(int32_t *)(__coords__ + 4 * __bufsize__) = i278;
    __val__[__bufsize__] = A277_vals[i278A277];
    if (++__bufsize__ == __bufcapcopy__) {
      TACO_DEREF(i278A277) = i278A277;
      TACO_DEREF(i278) = i278;
      TACO_DEREF(state) = 0;
      return __bufsize__;
    }
  resume_iterate0:;
  }
  TACO_DEREF(state) = 1;
  return __bufsize__;
  if (__bufsize__ > 0) {
  }
resume_iterate1:
  free(*__ctx__);
  *__ctx__ = NULL;
  return 0;
}
