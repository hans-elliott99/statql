#include "stdio.h"
#include "list.h"
#include "array.h"


/*
    Add arrays with broadcasting?

    Add 2 arrays together.
    If one array is of length 1, it is broadcasted to the length of the other.
    Otherwise, arrays must be of the same length.
    If inplace is on, first array is modified in place.
*/
ARRP __add_broadcast(ARRP v1, ARRP v2, int inplace) {
    size_t n1 = length(v1);
    size_t n2 = length(v2);
    if (arrtype(v1) == STRINGS_ARR || arrtype(v2) == STRINGS_ARR) {
        fprintf(stderr, "__add: not implemented for STRINGS_ARR\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "__add: lengths are not compatible\n");
        exit(1);
    }
    arrtype_t newtype = (arrtype(v1) == REALS_ARR && arrtype(v2) == REALS_ARR) ?
                         REALS_ARR : INTS_ARR;
    ARRP vnew;
    if (inplace) {
        if (n1 >= n2) {
            vnew = v1;
        } else {
            // v1 gets modfied inplace, thus need to resize v1 to be like v2
            resize_array(v1, n2);
            v1.node->arr->dims[0] = dims(v2)[0];
            v1.node->arr->dims[1] = dims(v2)[1];
            vnew = v1;
        }
        // if v1 is real but v2 is int, cast v1 to int
        if (newtype == INTS_ARR) cast_ints(vnew); 
    } else {
        vnew = alloc_same(n1 > n2 ? v1 : v2, newtype);
    }
    double scalar;
    if (n1 > n2) {
        // v2 is the scalar
        scalar = as_real(v2, 0, 0);
        switch (newtype)
        {
        case INTS_ARR: 
            if (arrtype(v1) == INTS_ARR) {
                for (size_t i=0; i < n1; ++i)
                    integer(vnew)[i] = integer(v1)[i] + scalar;
            } else {
                for (size_t i=0; i < n1; ++i)
                    integer(vnew)[i] = (int)(real(v1)[i] + scalar);
            }
            break;
        case REALS_ARR:
            // both arrays are real
            for (size_t i=0; i < n1; ++i)
                real(vnew)[i] = real(v1)[i] + scalar;
        case STRINGS_ARR:
        case NULL_ARR:
            break;
        }
    } else if (n2 > n1) {
        // v1 is the scalar; need to resize vnew
        scalar = as_real(v1, 0, 0);
        switch (newtype)
        {
        case INTS_ARR: 
            if (arrtype(v2) == INTS_ARR) {
                for (size_t i=0; i < n2; ++i)
                    integer(vnew)[i] = integer(v2)[i] + scalar;
            } else {
                for (size_t i=0; i < n2; ++i)
                    integer(vnew)[i] = (int)(real(v2)[i] + scalar);
            }
            break;
        case REALS_ARR:
            // both arrays are real
            for (size_t i=0; i < n2; ++i)
                real(vnew)[i] = real(v2)[i] + scalar;
        case STRINGS_ARR:
        case NULL_ARR:
            break;
        }
    } else {
        // both arrays equal length
        switch (newtype)
        {
        case INTS_ARR:
            if (arrtype(v1) == INTS_ARR && arrtype(v2) == INTS_ARR) {
                for (size_t i = 0; i < n1; ++i) {
                    integer(vnew)[i] = integer(v1)[i] + integer(v2)[i];
                }
            } else if (arrtype(v1) == INTS_ARR) {
                // v2 must be real, else above is false
                // (one must be int for newtype to be ints_arr)
                for (size_t i = 0; i < n1; ++i) {
                    integer(vnew)[i] = integer(v1)[i] + real(v2)[i];
                }
            } else if (arrtype(v2) == INTS_ARR) {
                // v1 must be real
                for (size_t i = 0; i < n1; ++i) {
                    integer(vnew)[i] = real(v1)[i] + integer(v2)[i];
                }
            }
            break;
        case REALS_ARR:
            // both v1 and v2 have to be real
            for (size_t i = 0; i < n1; ++i) {
                real(vnew)[i] = real(v1)[i] + real(v2)[i];
            }
            break;
        case STRINGS_ARR:
        case NULL_ARR:
            break;
        }
    }
    return vnew;
}