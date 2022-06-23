#include "taco_helpers.h"
#define TACO_MIN(_a,_b) ((_a) < (_b) ? (_a) : (_b))
#define TACO_MAX(_a,_b) ((_a) > (_b) ? (_a) : (_b))
#define TACO_DEREF(_a) (((___context___*)(*__ctx__))->_a)
int omp_get_thread_num() { return 0; }
int omp_get_max_threads() { return 1; }
int cmp(const void *a, const void *b) {
  return *(static_cast<const int *>(a)) - *(static_cast<const int *>(b));
}
int taco_binarySearchAfter(int *array, int arrayStart, int arrayEnd, int target) {
  if (array[arrayStart] >= target) {
    return arrayStart;
  }
  int lowerBound = arrayStart; // always < target
  int upperBound = arrayEnd; // always >= target
  while (upperBound - lowerBound > 1) {
    int mid = (upperBound + lowerBound) / 2;
    int midValue = array[mid];
    if (midValue < target) {
      lowerBound = mid;
    }
    else if (midValue > target) {
      upperBound = mid;
    }
    else {
      return mid;
    }
  }
  return upperBound;
}
int taco_binarySearchBefore(int *array, int arrayStart, int arrayEnd, int target) {
  if (array[arrayEnd] <= target) {
    return arrayEnd;
  }
  int lowerBound = arrayStart; // always <= target
  int upperBound = arrayEnd; // always > target
  while (upperBound - lowerBound > 1) {
    int mid = (upperBound + lowerBound) / 2;
    int midValue = array[mid];
    if (midValue < target) {
      lowerBound = mid;
    }
    else if (midValue > target) {
      upperBound = mid;
    }
    else {
      return mid;
    }
  }
  return lowerBound;
}
taco_tensor_t* init_taco_tensor_t(int32_t order, int32_t csize,
                                  int32_t* dimensions, int32_t* mode_ordering,
                                  int* mode_types) {
  taco_tensor_t* t = static_cast<taco_tensor_t *>(malloc(sizeof(taco_tensor_t)));
  t->order         = order;
  t->dimensions    = static_cast<int32_t *>( malloc(static_cast<size_t>(order) * sizeof(int32_t)));
  t->mode_ordering = static_cast<int32_t *>( malloc(static_cast<size_t>(order) * sizeof(int32_t)));
  t->mode_types    = static_cast<int *>( malloc(static_cast<size_t>(order) * sizeof(int)));
  t->indices       = static_cast<uint8_t ***>( malloc(static_cast<size_t>(order) * sizeof(uint8_t***)));
  t->csize         = csize;
  for (int32_t i = 0; i < order; i++) {
    t->dimensions[i]    = dimensions[i];
    t->mode_ordering[i] = mode_ordering[i];
    t->mode_types[i]    = mode_types[i];
    switch (t->mode_types[i]) {
      case taco_mode_dense:
        t->indices[i] = static_cast<uint8_t **>( malloc(1 * sizeof(uint8_t **)));
        break;
      case taco_mode_sparse:
        t->indices[i] = static_cast<uint8_t **>( malloc(2 * sizeof(uint8_t **)));
        break;
    }
  }
  return t;
}
void deinit_taco_tensor_t(void* s) {
  auto t = static_cast<taco_tensor_t *>(s);
  for (int i = 0; i < t->order; i++) {
    free(t->indices[i]);
  }
  free(t->indices);
  free(t->dimensions);
  free(t->mode_ordering);
  free(t->mode_types);
  free(t);
}

double sqrt_double(double a) {
  return sqrt(a);
}

void *pochi_malloc(size_t size) {
  return malloc(size);
}

void *pochi_calloc(size_t nitems, size_t size) {
  return calloc(nitems, size);
}

void *pochi_realloc(void *ptr, size_t size) {
  return realloc(ptr, size);
}

void pochi_free(void *ptr) {
  free(ptr);
}

void pochi_qsort(void *base, size_t nitems) {
  qsort(base, nitems, sizeof(const int), cmp);
}

long pochi_bitand(long a, long b) {
  return a & b;
}

long pochi_bitor(long a, long b) {
  return a | b;
}

int min(int a, int b) {
  return a < b ? a : b;
}

int max(int a, int b) {
  return a > b ? a : b;
}

#include "taco_wrapper_impls.h"
