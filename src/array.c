#include "array.h"
#include "memory.h"
#include "list.h"

#include <stdio.h> 
#include <string.h>
#include <stdarg.h>
#include <math.h> // sqrt

#include "global.h"



/*
    arrtype_t - Array Type
*/
const char *arrtype_str(arrtype_t t) {
    switch (t) {
        case INTS_ARR:
            return "INTS_ARR";
        case REALS_ARR:
            return "REALS_ARR";
        case STRINGS_ARR:
            return "STRINGS_ARR";
        case NULL_ARR:
            return "NULL_ARR";
        default:
            return "UNKOWN";
    }
}



/*
    Misc Helpers
*/
size_t _as_ix(size_t dims[2], size_t ixs[2]) {
    // row major order:
    // row-ix * ncol + col-ix
    return ixs[0] * dims[1] + ixs[1];
}

void check_valid_dims(size_t *dims, size_t ndim) {
    for (size_t i=0; i < ndim; ++i) {
        if (dims[i] <= 0) {
            fprintf(stderr, "check_valid_dims: dims[%ld] <= 0\n", i);
            exit(1);
        }
    }
}

void check_valid_ix(size_t dims[2], size_t ix[2]) {
    if (ix[0] < 0 || ix[0] >= dims[0]) {
        fprintf(stderr, "check_valid_ix: ix[0] out of bounds\n");
        exit(1);
    }
    if (ix[1] < 0 || ix[1] >= dims[1]) {
        fprintf(stderr, "check_valid_ix: ix[1] out of bounds\n");
        exit(1);
    }
}



/*
    ArrayStruct
    - currently only 1d and 2d arrays are supported
*/
void alloc_array_struct(ArrayStruct *ar, arrtype_t type, size_t dim0, size_t dim1) {
    size_t dims[2] = {dim0, dim1};
    check_valid_dims(dims, 2);
    size_t nelem = dims[0] * dims[1];
    ar->type = type;
    ar->capacity = nelem;
    ar->nalloc = (type == STRINGS_ARR) ? 0 : nelem; // inidiv strings need allocation
    ar->dims[0] = dims[0];
    ar->dims[1] = dims[1];
    ar->data = NULL;
    ar->ints = NULL;
    ar->reals = NULL;
    ar->strings = NULL;
    switch (type) {
        case INTS_ARR:
            ar->data = chk_calloc(nelem, sizeof(int));
            ar->ints = (int*)ar->data;
            break;
        case REALS_ARR:
            ar->data = chk_calloc(nelem, sizeof(double));
            ar->reals = (double*)ar->data;
            break;
        case STRINGS_ARR:
            ar->data = chk_malloc(nelem * sizeof(char*));
            ar->strings = (char**)ar->data;
            break;
        case NULL_ARR:
            break;
        default:
            fprintf(stderr, "alloc_array_struct: unknown type: %d\n", type);
            exit(1);
    }
}


void free_arraystruct_data(ArrayStruct *ar) {
    if (ar->type == STRINGS_ARR) {
        for (size_t i = 0; i < ar->nalloc; ++i) {
            chk_free(ar->strings[i]);
        }
    }
    chk_free(ar->data);
    ar->data = NULL;
    ar->ints = NULL;
    ar->reals = NULL;
    ar->strings = NULL;
    ar->capacity = 0;
    ar->nalloc = 0;
    ar->dims[0] = 0;
    ar->dims[1] = 0;
}



/*
    ARRP - array pointer
*/
ARRP alloc_array(arrtype_t type, size_t dim0, size_t dim1) {
    ARRP v;
    v.node = chk_malloc(sizeof(struct DLNode));
    v.node->arr = chk_malloc(sizeof(ArrayStruct));
    dllist_append(&memstack, v.node);
    alloc_array_struct(v.node->arr, type, dim0, dim1);
    return v;
}

ARRP empty() {
    ARRP v;
    v.node = NULL;
    return v;
}

void free_array(ARRP *v) {
    if (v->node != NULL)
        dllist_remove(&memstack, v->node); // calls free_arraystruct_data
    v->node = NULL;
}


// doesn't handle modification of dims!!! newsize would be dim0*dim1
ARRP resize_array(ARRP v, size_t newsize) {
    if (newsize == 0) {
        free_arraystruct_data(v.node->arr); // but leave node on memstack, for reuse
        return v;
    }
    if (newsize == v.node->arr->capacity) {
        return v;
    }
    switch (arrtype(v)) {
        case INTS_ARR:
            chk_realloc((void**)&v.node->arr->ints, newsize * sizeof(int));
            v.node->arr->data = v.node->arr->ints;
            v.node->arr->nalloc = newsize;
            v.node->arr->capacity = newsize;
            break;
        case REALS_ARR:
            chk_realloc((void**)&v.node->arr->reals, newsize * sizeof(double));
            v.node->arr->data = v.node->arr->reals;
            v.node->arr->nalloc = newsize;
            v.node->arr->capacity = newsize;
            break;
        case STRINGS_ARR: ;
            /*
                If new size is less than current number of allocated strings,
                don't leave allocated strings hanging at the end of array
            */
            size_t nalloc = v.node->arr->nalloc;
            for (size_t i = nalloc; i > newsize; --i) {
                chk_free(v.node->arr->strings[i]);
                v.node->arr->nalloc--;
            }
            chk_realloc((void**)&v.node->arr->strings, newsize * sizeof(char*));
            v.node->arr->data = v.node->arr->strings;
            v.node->arr->capacity = newsize;
            // nalloc doesn't increase
            break;
        default:
            fprintf(stderr, "resize_array: unknown type: %d\n", arrtype(v));
            exit(1);
    }
    return v;
}


size_t length(ARRP v) {
    return v.node->arr->nalloc;
}

size_t capacity(ARRP v) {
    return v.node->arr->capacity;
}

size_t *dims(ARRP v) {
    return v.node->arr->dims;
}

arrtype_t arrtype(ARRP v) {
    return v.node->arr->type;
}



/*INTEGER ARRAY*/

/*direct access to flat array*/
int *integer(ARRP v) {
    if (arrtype(v) != INTS_ARR) {
        fprintf(stderr, "integer: vector is of type %s, expected INTS_ARR\n",
                arrtype_str(arrtype(v)));
        exit(1);
    }
    return v.node->arr->ints;
}

int ints_elt(ARRP v, size_t dim0, size_t dim1) {
    size_t ixs[2] = {dim0, dim1};
    check_valid_ix(dims(v), ixs);
    if (arrtype(v) != INTS_ARR) {
        fprintf(stderr, "ints_elt: array is of type %s, expected INTS_ARR\n",
                arrtype_str(arrtype(v)));
        exit(1);
    }
    return v.node->arr->ints[_as_ix(dims(v), ixs)];
}

int as_int(ARRP v, size_t dim0, size_t dim1) {
    size_t ixs[2] = {dim0, dim1};
    check_valid_ix(dims(v), ixs);
    if (arrtype(v) == STRINGS_ARR) {
        fprintf(stderr, "as_int: not implemented for STRINGS_ARR\n");
        exit(1);
    }
    if (arrtype(v) == REALS_ARR) {
        return (int)v.node->arr->reals[_as_ix(dims(v), ixs)];
    } else {
        return v.node->arr->ints[_as_ix(dims(v), ixs)];
    }
}

void set_ints_elt(ARRP v, size_t dim0, size_t dim1, int val) {
    size_t ixs[2] = {dim0, dim1};
    check_valid_ix(dims(v), ixs);
    if (arrtype(v) != INTS_ARR) {
        fprintf(stderr, "set_ints_elt: array is of type %s, expected INTS_ARR\n",
                arrtype_str(arrtype(v)));
        exit(1);
    }
    v.node->arr->ints[_as_ix(dims(v), ixs)] = val;
}

/*convert a real array to an integer array*/
void cast_ints(ARRP v) {
    if (arrtype(v) == INTS_ARR) {
        return;
    } else if (arrtype(v) == STRINGS_ARR) {
        fprintf(stderr, "cast_ints: not implemented for STRINGS_ARR");
        exit(1);
    }
    v.node->arr->type = INTS_ARR;
    v.node->arr->ints = chk_malloc(v.node->arr->capacity * sizeof(int));
    for (size_t i = 0; i < v.node->arr->nalloc; i++) {
        v.node->arr->ints[i] = (int)v.node->arr->reals[i];
    }
    chk_free(v.node->arr->reals);
    v.node->arr->reals = NULL;
    v.node->arr->data = v.node->arr->ints;
}



/*REAL ARRAY*/
double *real(ARRP v) {
    if (arrtype(v) != REALS_ARR) {
        fprintf(stderr, "real: array is of type %s, expected REALS_ARR\n",
                arrtype_str(arrtype(v)));
        exit(1);
    }
    return v.node->arr->reals;
}

double reals_elt(ARRP v, size_t dim0, size_t dim1) {
    size_t ixs[2] = {dim0, dim1};
    check_valid_ix(dims(v), ixs);
    if (arrtype(v) != REALS_ARR) {
        fprintf(stderr, "reals_elt: array is of type %s, expected REALS_ARR\n",
                arrtype_str(arrtype(v)));
        exit(1);
    }
    return v.node->arr->reals[_as_ix(dims(v), ixs)];
}


double as_real(ARRP v, size_t dim0, size_t dim1) {
    size_t ixs[2] = {dim0, dim1};
    check_valid_ix(dims(v), ixs);
    if (arrtype(v) == STRINGS_ARR) {
        fprintf(stderr, "as_int: not implemented for STRINGS_ARR\n");
        exit(1);
    }
    if (arrtype(v) == INTS_ARR) {
        return (double)v.node->arr->ints[_as_ix(dims(v), ixs)];
    } else {
        return v.node->arr->reals[_as_ix(dims(v), ixs)];
    }
}

void set_reals_elt(ARRP v, size_t dim0, size_t dim1, double val) {
    size_t ixs[2] = {dim0, dim1};
    check_valid_ix(dims(v), ixs);
    if (arrtype(v) != REALS_ARR) {
        fprintf(stderr, "set_reals_elt: array is of type %s, expected REALS_ARR\n",
                arrtype_str(arrtype(v)));
        exit(1);
    }
    v.node->arr->reals[_as_ix(dims(v), ixs)] = val;
}

/*convert an integer array to a real array*/
void cast_reals(ARRP v) {
    if (arrtype(v) == REALS_ARR) {
        return;
    } else if (arrtype(v) == STRINGS_ARR) {
        fprintf(stderr, "cast_reals: not implemented for STRINGS_ARR");
        exit(1);
    }
    v.node->arr->type = REALS_ARR;
    v.node->arr->reals = chk_malloc(v.node->arr->capacity * sizeof(double));
    for (size_t i = 0; i < v.node->arr->nalloc; i++) {
        v.node->arr->reals[i] = (double)v.node->arr->ints[i];
    }
    chk_free(v.node->arr->ints);
    v.node->arr->ints = NULL;
    v.node->arr->data = v.node->arr->reals;
}


/*STRING ARRAY*/
const char *strings_elt(ARRP v, size_t dim0, size_t dim1) {
    size_t ixs[2] = {dim0, dim1};
    check_valid_ix(dims(v), ixs);
    if (arrtype(v) != STRINGS_ARR) {
        fprintf(stderr, "strings_elt: array is of type %s, expected STRINGS_ARR\n",
                arrtype_str(arrtype(v)));
        exit(1);
    }
    return v.node->arr->strings[_as_ix(dims(v), ixs)];
}

void set_strings_elt(ARRP v, size_t dim0, size_t dim1, const char *val) {
    size_t ixs[2] = {dim0, dim1};
    check_valid_ix(dims(v), ixs);
    if (arrtype(v) != STRINGS_ARR) {
        fprintf(stderr, "set_strings_elt: array is of type %s, expected STRINGS_ARR\n",
                arrtype_str(arrtype(v)));
        exit(1);
    }
    chk_strcpy(&v.node->arr->strings[_as_ix(dims(v), ixs)], val);
    v.node->arr->nalloc++;
}



/*MISC*/
void* arrp_data(ARRP v) {
    return v.node->arr->data;
}



/*allocate new array with same dimensions as the input,
  so maintains matrix attributes*/
ARRP alloc_same(const ARRP v, arrtype_t type) {
    ARRP v2 = alloc_array(type, dims(v)[0], dims(v)[1]);
    return v2;
}


/*create a copy of the given array*/
ARRP copyarr(const ARRP v) {
    ARRP v2 = alloc_same(v, arrtype(v));
    switch (arrtype(v)) {
    case INTS_ARR:
        for (size_t i = 0; i < length(v); ++i) {
            integer(v2)[i] = integer(v)[i];
        }
        break;
    case REALS_ARR:
        for (size_t i = 0; i < length(v); ++i) {
            real(v2)[i] = real(v)[i];
        }
        break;
    case STRINGS_ARR: ;
        size_t nrow = dims(v)[0];
        size_t ncol = dims(v)[1];
        for (size_t i = 0; i < nrow; ++i) {
            for (size_t j = 0; j < ncol; ++j)
                set_strings_elt(v2, i, j, strings_elt(v, i, j));
        }
        break;
    default:
        fprintf(stderr, "copyarr: array of unkown type\n");
        exit(1);
        break;
    }
    return v2;
}



/*
    ARRAY OPERATIONS
*/


/*
                SCALAR OPS
*/

void __add_num(ARRP *vin, ARRP *vout, double scalar) {
    switch (arrtype(*vin))
    {
    case INTS_ARR:
        for (size_t i=0; i < length(*vin); ++i)
            integer(*vout)[i] = integer(*vin)[i] + scalar;
        break;
    case REALS_ARR:
        for (size_t i=0; i < length(*vin); ++i)
            real(*vout)[i] = real(*vin)[i] + scalar;
        break;
    default:
        fprintf(stderr, "__add_num: unsupported type: %s",
                arrtype_str(arrtype(*vin)));
        exit(1);
        break;
    }
}

ARRP add_num(ARRP v, double scalar) {
    ARRP v2 = alloc_same(v, arrtype(v));
    __add_num(&v, &v2, scalar);
    return v2;
}

ARRP set_add_num(ARRP v, double scalar) {
    __add_num(&v, &v, scalar);
    return v;
}


void __mul_num(ARRP *vin, ARRP *vout, double scalar) {
    switch (arrtype(*vin))
    {
    case INTS_ARR:
        for (size_t i=0; i < length(*vin); ++i)
            integer(*vout)[i] = integer(*vin)[i] * scalar;
        break;
    case REALS_ARR:
        for (size_t i=0; i < length(*vin); ++i)
            real(*vout)[i] = real(*vin)[i] * scalar;
        break;
    default:
        fprintf(stderr, "__mul_num: unsupported type: %s",
                arrtype_str(arrtype(*vin)));
        exit(1);
        break;
    }
}


ARRP mul_num(ARRP v, double scalar) {
    ARRP v2 = alloc_same(v, arrtype(v));
    __mul_num(&v, &v2, scalar);
    return v2;
}

ARRP set_mul_num(ARRP v, double scalar) {
    __mul_num(&v, &v, scalar);
    return v;
}


ARRP div_num(ARRP v, double scalar) {
    ARRP v2 = alloc_same(v, arrtype(v));
    __mul_num(&v, &v2, 1/scalar);
    return v2;
}

ARRP set_div_num(ARRP v, double scalar) {
    __mul_num(&v, &v, 1/scalar);
    return v;
}



/*
                FILL VALUES
*/

/*
    array type unaffected
*/
ARRP set_fill_num(ARRP v, double start, double step) {
    switch (arrtype(v))
    {
    case INTS_ARR:
        for (size_t i=0; i < length(v); ++i) {
            integer(v)[i] = (int)(start + step * i);
        }
        break;
    case REALS_ARR:
        for (size_t i=0; i < length(v); ++i) {
            real(v)[i] = start + step * i;
        }
        break;
    default:
        fprintf(stderr, "set_fill_num: unsupported type: %s",
                arrtype_str(arrtype(v)));
        exit(1);
        break;
    }
    return v;
}


ARRP set_fill_str(ARRP v, const char *val) {
    if (arrtype(v) != STRINGS_ARR) {
        fprintf(stderr, "set_fill_str: unsupported type: %s",
                arrtype_str(arrtype(v)));
        exit(1);
    }
    for (size_t i=0; i < dims(v)[0]; ++i) {
        for (size_t j=0; j < dims(v)[1]; ++j)
            set_strings_elt(v, i, j, val);
    }
    return v;
}


/*
                ELEMENT-WISE FUNCTIONS
*/


/*
    array type unaffected 
*/
void __arrp_pow2(ARRP *vin, ARRP *vout) {
    switch (arrtype(*vin))
    {
    case INTS_ARR:
        for (size_t i=0; i < length(*vin); ++i) {
            int num = integer(*vin)[i];
            integer(*vout)[i] = num*num;
        }
        break;
    case REALS_ARR:
        for (size_t i=0; i < length(*vin); ++i) {
            double num = real(*vin)[i];
            real(*vout)[i] = num*num;
        }
        break;
    default:
        fprintf(stderr, "__arrp_pow2: unsupported type: %s",
                arrtype_str(arrtype(*vin)));
        exit(1);
        break;
    }
}
ARRP arrp_pow2(ARRP v) {
    ARRP v2 = alloc_same(v, arrtype(v));
    __arrp_pow2(&v, &v2);
    return v2;
}
ARRP set_arrp_pow2(ARRP v) {
    __arrp_pow2(&v, &v);
    return v;
}

/*
    array type always converted to real
*/
void __arrp_sqrt(ARRP *vin, ARRP *vout) {
    switch (arrtype(*vin))
    {
    case INTS_ARR:
        for (size_t i=0; i < length(*vin); ++i) {
            real(*vout)[i] = sqrt((double)integer(*vin)[i]);
        }
    case REALS_ARR:
        for (size_t i=0; i < length(*vin); ++i) {
            real(*vout)[i] = sqrt(real(*vin)[i]);
        }
        break;
    default:
        fprintf(stderr, "__arrp_sqrt: unsupported type: %s",
                arrtype_str(arrtype(*vin)));
        exit(1);
        break;
    }
}
ARRP arrp_sqrt(ARRP v) {
    ARRP v2 = alloc_same(v, REALS_ARR);
    __arrp_sqrt(&v, &v2);
    return v2;
}
ARRP set_arrp_sqrt(ARRP v) {
    cast_reals(v);
    __arrp_sqrt(&v, &v);
    return v;
}


/*
    Reciprocal
    array type always converted to real
*/
void __arrp_recip(ARRP *vin, ARRP *vout) {
    switch (arrtype(*vin))
    {
    case INTS_ARR:
        for (size_t i=0; i < length(*vin); ++i) {
            real(*vout)[i] = 1. / (double)integer(*vin)[i];
        }
    case REALS_ARR:
        for (size_t i=0; i < length(*vin); ++i) {
            real(*vout)[i] = 1. / real(*vin)[i];
        }
        break;
    default:
        fprintf(stderr, "__arrp_sqrt: unsupported type: %s",
                arrtype_str(arrtype(*vin)));
        exit(1);
        break;
    }
}
ARRP arrp_recip(ARRP v) {
    ARRP v2 = alloc_same(v, REALS_ARR);
    __arrp_recip(&v, &v2);
    return v2;
}
ARRP set_arrp_recip(ARRP v) {
    cast_reals(v);
    __arrp_recip(&v, &v);
    return v;
}


/*
                ARRAY OPERATIONS
*/

/*
    Add 2 arrays together.
    Arrays must be of the same length.
    If inplace is on, first array is modified in place.
*/
ARRP __add(ARRP v1, ARRP v2, int inplace, int sign) {
    size_t n1 = length(v1);
    size_t n2 = length(v2);
    if (arrtype(v1) == STRINGS_ARR || arrtype(v2) == STRINGS_ARR) {
        fprintf(stderr, "__add: not implemented for STRINGS_ARR\n");
        exit(1);
    }
    if (n1 != n2) {
        fprintf(stderr, "__add: lengths are not compatible\n");
        exit(1);
    }
    arrtype_t newtype = arrtype(v1);
    ARRP vnew;
    if (inplace) {
        vnew = v1;
    } else {
        vnew = alloc_same(v1, newtype);
    }
    switch (newtype)
    {
    case INTS_ARR:
        // both v1 and v2 have to be int
        for (size_t i = 0; i < n1; ++i) {
            integer(vnew)[i] = integer(v1)[i] + sign * integer(v2)[i];
        }
        break;
    case REALS_ARR:
        if (arrtype(v1) == INTS_ARR) {
            // v2 must be real
            for (size_t i = 0; i < n1; ++i) {
                real(vnew)[i] = integer(v1)[i] + sign * real(v2)[i];
            }
        } else if (arrtype(v2) == INTS_ARR) {
            // v1 must be real
            for (size_t i = 0; i < n1; ++i) {
                real(vnew)[i] = real(v1)[i] + sign * integer(v2)[i];
            }
        } else {
            // both v1 and v2 are real
            for (size_t i = 0; i < n1; ++i) {
                real(vnew)[i] = real(v1)[i] + sign * real(v2)[i];
            }
        }
        break;
    case STRINGS_ARR:
    case NULL_ARR:
        break;
    }
    return vnew;
}


ARRP add(ARRP v1, ARRP v2) {
    return __add(v1, v2, 0, 1);
}

ARRP set_add(ARRP v1, ARRP v2) {
    return __add(v1, v2, 1, 1);
}

ARRP subtract(ARRP v1, ARRP v2) {
    return __add(v1, v2, 0, -1);
}

ARRP set_subtract(ARRP v1, ARRP v2) {
    return __add(v1, v2, 1, -1);
}


ARRP __mul(ARRP v1, ARRP v2, int inplace) {
    size_t n1 = length(v1);
    size_t n2 = length(v2);
    if (arrtype(v1) == STRINGS_ARR || arrtype(v2) == STRINGS_ARR) {
        fprintf(stderr, "__mul: not implemented for STRINGS_ARR\n");
        exit(1);
    }
    if (n1 != n2) {
        fprintf(stderr, "__mul: lengths are not compatible\n");
        exit(1);
    }
    arrtype_t newtype = arrtype(v1);
    ARRP vnew;
    if (inplace) {
        vnew = v1;
    } else {
        vnew = alloc_same(v1, newtype);
    }
    switch (newtype)
    {
    case INTS_ARR:
        // both v1 and v2 have to be int
        for (size_t i = 0; i < n1; ++i) {
            integer(vnew)[i] = integer(v1)[i] * integer(v2)[i];
        }
        break;
    case REALS_ARR:
        if (arrtype(v1) == INTS_ARR) {
            // v2 must be real
            for (size_t i = 0; i < n1; ++i) {
                real(vnew)[i] = integer(v1)[i] * real(v2)[i];
            }
        } else if (arrtype(v2) == INTS_ARR) {
            // v1 must be real
            for (size_t i = 0; i < n1; ++i) {
                real(vnew)[i] = real(v1)[i] * integer(v2)[i];
            }
        } else {
            // both v1 and v2 are real
            for (size_t i = 0; i < n1; ++i) {
                real(vnew)[i] = real(v1)[i] * real(v2)[i];
            }
        }
        break;
    case STRINGS_ARR:
    case NULL_ARR:
        break;
    }
    return vnew;
}


ARRP mul(ARRP v1, ARRP v2) {
    return __mul(v1, v2, 0);
}

ARRP set_mul(ARRP v1, ARRP v2) {
    return __mul(v1, v2, 1);
}


ARRP __div(ARRP v1, ARRP v2, int inplace) {
    size_t n1 = length(v1);
    size_t n2 = length(v2);
    if (arrtype(v1) == STRINGS_ARR || arrtype(v2) == STRINGS_ARR) {
        fprintf(stderr, "__div: not implemented for STRINGS_ARR\n");
        exit(1);
    }
    if (n1 != n2) {
        fprintf(stderr, "__div: lengths are not compatible\n");
        exit(1);
    }
    arrtype_t newtype = arrtype(v1);
    ARRP vnew;
    if (inplace) {
        vnew = v1;
    } else {
        vnew = alloc_same(v1, newtype);
    }
    switch (newtype)
    {
    case INTS_ARR:
        // both v1 and v2 have to be int
        for (size_t i = 0; i < n1; ++i) {
            integer(vnew)[i] = integer(v1)[i] / integer(v2)[i];
        }
        break;
    case REALS_ARR:
        if (arrtype(v1) == INTS_ARR) {
            // v2 must be real
            for (size_t i = 0; i < n1; ++i) {
                real(vnew)[i] = integer(v1)[i] / real(v2)[i];
            }
        } else if (arrtype(v2) == INTS_ARR) {
            // v1 must be real
            for (size_t i = 0; i < n1; ++i) {
                real(vnew)[i] = real(v1)[i] / integer(v2)[i];
            }
        } else {
            // both v1 and v2 are real
            for (size_t i = 0; i < n1; ++i) {
                real(vnew)[i] = real(v1)[i] / real(v2)[i];
            }
        }
        break;
    case STRINGS_ARR:
    case NULL_ARR:
        break;
    }
    return vnew;
}

ARRP divide(ARRP v1, ARRP v2) {
    return __div(v1, v2, 0);
}

ARRP set_divide(ARRP v1, ARRP v2) {
    return __div(v1, v2, 1);
}


int dims_eq(ARRP v1, ARRP v2) {
    size_t *dims1 = dims(v1);
    size_t *dims2 = dims(v2);
    return dims1[0] == dims2[0] && dims1[1] == dims2[1];
}

int ints_eq(ARRP v1, ARRP v2) {
    if (arrtype(v1) != INTS_ARR || arrtype(v2) != INTS_ARR) {
        fprintf(stderr, "ints_eq: received non-INTS_ARR array\n");
        exit(1);
    }
    size_t n1 = length(v1);
    size_t n2 = length(v2);
    if (n1 != n2) {
        return 0;
    }
    for (size_t i = 0; i < n1; ++i) {
        if (integer(v1)[i] != integer(v2)[i]) {
            return 0;
        }
    }
    return 1;
}

int reals_eq_tol(ARRP v1, ARRP v2, double tol) {
    if (arrtype(v1) != REALS_ARR || arrtype(v2) != REALS_ARR) {
        fprintf(stderr, "reals_eq_tol: received non-REALS_ARR array\n");
        exit(1);
    }
    size_t n1 = length(v1);
    size_t n2 = length(v2);
    if (n1 != n2) {
        return 0;
    }
    for (size_t i = 0; i < n1; ++i) {
        if (fabs(real(v1)[i] - real(v2)[i]) > tol) {
            return 0;
        }
    }
    return 1;
}


/*
        MATRIX OPERATIONS
*/
ARRP row(const ARRP v, size_t dim0) {
    size_t nrow = dims(v)[0];
    size_t ncol = dims(v)[1];
    if (dim0 >= nrow || dim0 < 0) {
        fprintf(stderr, "row: dim0 out of bounds\n");
        exit(1);
    }
    ARRP v2 = alloc_array(arrtype(v), 1, ncol);
    for (size_t j = 0; j < ncol; ++j) {
        switch (arrtype(v))
        {
        case INTS_ARR:
            set_ints_elt(v2, 0, j, ints_elt(v, dim0, j));
            break;
        case REALS_ARR:
            set_reals_elt(v2, 0, j, reals_elt(v, dim0, j));
            break;
        case STRINGS_ARR:
            set_strings_elt(v2, 0, j, strings_elt(v, dim0, j));
            break;
        default:
            fprintf(stderr, "row: unsupported type: %s\n", arrtype_str(arrtype(v)));
            exit(1);
            break;
        }
    }
    return v2;
}


double *real_row_ptr(const ARRP v, size_t dim0) {
    return real(v) + dim0 * dims(v)[1];
}


void set_row(ARRP v, size_t dim0, ARRP vrow) {
    size_t nrow = dims(v)[0];
    size_t ncol = dims(v)[1];
    if (dim0 >= nrow || dim0 < 0) {
        fprintf(stderr, "set_row: dim0 out of bounds\n");
        exit(1);
    }
    if (dims(vrow)[0] != 1 || dims(vrow)[1] != ncol) {
        fprintf(stderr, "set_row: dimensions of row array are not compatible\n");
        exit(1);
    }
    for (size_t j = 0; j < ncol; ++j) {
        switch (arrtype(v))
        {
        case INTS_ARR:
            set_ints_elt(v, dim0, j, ints_elt(vrow, 0, j));
            break;
        case REALS_ARR:
            set_reals_elt(v, dim0, j, reals_elt(vrow, 0, j));
            break;
        case STRINGS_ARR:
            set_strings_elt(v, dim0, j, strings_elt(vrow, 0, j));
            break;
        default:
            fprintf(stderr, "set_row: unsupported type: %s\n", arrtype_str(arrtype(v)));
            exit(1);
            break;
        }
    }
}



ARRP col(ARRP v, size_t dim1) {
    size_t nrow = dims(v)[0];
    size_t ncol = dims(v)[1];
    if (dim1 >= ncol || dim1 < 0) {
        fprintf(stderr, "col: dim1 out of bounds\n");
        exit(1);
    }
    ARRP v2 = alloc_array(arrtype(v), nrow, 1);
    for (size_t i = 0; i < nrow; ++i) {
        switch (arrtype(v))
        {
        case INTS_ARR:
            set_ints_elt(v2, i, 0, ints_elt(v, i, dim1));
            break;
        case REALS_ARR:
            set_reals_elt(v2, i, 0, reals_elt(v, i, dim1));
            break;
        case STRINGS_ARR:
            set_strings_elt(v2, i, 0, strings_elt(v, i, dim1));
            break;
        default:
            fprintf(stderr, "col: unsupported type: %s\n", arrtype_str(arrtype(v)));
            exit(1);
            break;
        }
    }
    return v2;
}


ARRP set_col(ARRP v, size_t dim1, ARRP vcol) {
    size_t nrow = dims(v)[0];
    size_t ncol = dims(v)[1];
    if (dim1 >= ncol || dim1 < 0) {
        fprintf(stderr, "set_col: dim1 out of bounds\n");
        exit(1);
    }
    if (dims(vcol)[0] != nrow || dims(vcol)[1] != 1) {
        fprintf(stderr, "set_col: dimensions of col array are not compatible\n");
        exit(1);
    }
    for (size_t i = 0; i < nrow; ++i) {
        switch (arrtype(v))
        {
        case INTS_ARR:
            set_ints_elt(v, i, dim1, ints_elt(vcol, i, 0));
            break;
        case REALS_ARR:
            set_reals_elt(v, i, dim1, reals_elt(vcol, i, 0));
            break;
        case STRINGS_ARR:
            set_strings_elt(v, i, dim1, strings_elt(vcol, i, 0));
            break;
        default:
            fprintf(stderr, "set_col: unsupported type: %s\n", arrtype_str(arrtype(v)));
            exit(1);
            break;
        }
    }
    return v;
}



ARRP matmul(const ARRP m1, const ARRP m2) {
    if (dims(m1)[1] != dims(m2)[0]) { // m1 cols must eq m2 rows
        fprintf(stderr, "__matmul: dimensions are not compatible\n");
        exit(1);
    }
    ARRP out = alloc_array(REALS_ARR, dims(m1)[0], dims(m2)[1]); // m1.rows x m2.cols
    double sum;
    for (size_t i = 0; i < dims(m1)[0]; ++i) {
        for (size_t j = 0; j < dims(m2)[1]; ++j) {
            sum = 0;
            for (size_t k = 0; k < dims(m1)[1]; ++k) {
                sum += reals_elt(m1, i, k) * reals_elt(m2, k, j);
            }
            set_reals_elt(out, i, j, sum);
        }
    }
}

ARRP set_matmul(ARRP m1, ARRP m2) {
    // compute product
    ARRP prod = matmul(m1, m2);
    // reshape m1 and copy product into it
    cast_reals(m1);
    m1 = resize_array(m1, length(prod));
    m1.node->arr->dims[0] = dims(prod)[0];
    m1.node->arr->dims[1] = dims(prod)[1];
    for (size_t i = 0; i < length(prod); ++i) {
        real(m1)[i] = real(prod)[i];
    }
    // discard temp array
    free_array(&prod);
    return m1;
}

