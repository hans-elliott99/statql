#ifndef __ARRAY_H
#define __ARRAY_H


#include <stdlib.h> // size_t
#include <stdint.h> // uint32_t


/*
    ARRAY DATA STRUCTURES AND MAIN METHODS
*/

typedef enum {
    INTS_ARR = 0,
    REALS_ARR,
    STRINGS_ARR,
    NULL_ARR
} arrtype_t;

const char *arrtype_str(arrtype_t t);


typedef struct ArrayStruct {
    arrtype_t type;         // type of data contained by vector
    void *data;             // pointer to the memory allocated for the vector
    int *ints;              // pointer to data, if type is INTS_ARR
    double *reals;        // pointer to data, if type is REALS_ARR
    char **strings;         // pointer to data, if type is STRINGS_ARR
    size_t capacity;        // vector capacity / length
    size_t nalloc;          // number of allocated elements (differs from capacity only for STRINGS_ARR)
    size_t dims[2];
} ArrayStruct;

void alloc_array_struct(ArrayStruct *ar, arrtype_t type, size_t dim0, size_t dim1);
void free_arraystruct_data(ArrayStruct *ar);


typedef struct ARRP {
    struct DLNode *node;
} ARRP;

ARRP alloc_array(arrtype_t type, size_t dim0, size_t dim1);
ARRP alloc_row_array(arrtype_t type, size_t length);
ARRP resize_array(ARRP v, size_t newsize);
ARRP empty();
void free_array(ARRP *v);

int *integer(ARRP v);
void set_ints_elt(ARRP v, size_t dim0, size_t dim1, int val);
int ints_elt(ARRP v, size_t dim0, size_t dim1);
int as_int(ARRP v, size_t dim0, size_t dim1);
void cast_ints(ARRP v);
//
double *real(ARRP v);
double reals_elt(ARRP v, size_t dim0, size_t dim1);
double as_real(ARRP v, size_t dim0, size_t dim1);
void set_reals_elt(ARRP v, size_t dim0, size_t dim1, double val);
void cast_reals(ARRP v);
//
const char *strings_elt(ARRP v, size_t dim0, size_t dim1);
void set_strings_elt(ARRP v, size_t dim0, size_t dim1, const char *val);

size_t length(ARRP v);
size_t capacity(ARRP v);
size_t *dims(ARRP v);
arrtype_t arrtype(ARRP v);

void* arrp_data(ARRP v); // generic version of real/integer
ARRP alloc_same(const ARRP v, arrtype_t type);
ARRP copyarr(const ARRP v);
int dims_eq(ARRP v1, ARRP v2);
int ints_eq(ARRP v1, ARRP v2);
int reals_eq_tol(ARRP v1, ARRP v2, double tol);

/*
    ARRAY OPERATIONS
*/
/*
        SCALAR OPS
*/
ARRP add_num(ARRP v, double scalar);
ARRP set_add_num(ARRP v, double scalar);
ARRP mul_num(ARRP v, double scalar);
ARRP set_mul_num(ARRP v, double scalar);
ARRP div_num(ARRP v, double scalar);
ARRP set_div_num(ARRP v, double scalar);
/*
        FILL VALUES
*/
ARRP set_fill_num(ARRP v, double start, double step);
ARRP set_rand_unif(ARRP v, uint32_t seed);
ARRP set_fill_str(ARRP v, const char *val);
/*
    ELEMENT-WISE FUNCTIONS
*/
ARRP arrp_pow2(ARRP v);
ARRP set_arrp_pow2(ARRP v);
ARRP arrp_sqrt(ARRP v);
ARRP set_arrp_sqrt(ARRP v);
/*
    ARRAY OPERATIONS
*/
ARRP add(ARRP v1, ARRP v2);
ARRP set_add(ARRP v1, ARRP v2);
ARRP subtract(ARRP v1, ARRP v2);
ARRP set_subtract(ARRP v1, ARRP v2);
ARRP mul(ARRP v1, ARRP v2);
ARRP set_mul(ARRP v1, ARRP v2);
ARRP divide(ARRP v1, ARRP v2);
ARRP set_divide(ARRP v1, ARRP v2);
/*
    MATIRX OPERATIONS
*/
ARRP matmul(const ARRP m1, const ARRP m2);
ARRP set_matmul(ARRP m1, const ARRP m2);
ARRP transpose(const ARRP v);
ARRP set_transpose(ARRP v);
ARRP crossprod(ARRP x, ARRP y);
ARRP tcrossprod(ARRP x, ARRP y);




#endif // __ARRAY_H