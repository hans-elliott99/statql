#include <stdio.h>
#include <stdlib.h> // calloc
#include <string.h> // strcmp
#include <stdarg.h> // va_list, va_start, va_end
#include <math.h> // sqrt
#include "sqlite/sqlite3.h"

#include "global.h"
#include "array.h"
#include "list.h"
#include "memory.h"

/////////////////////////////////////////
int main2() {
    if (atexit(free_memstack)) {
        fprintf(stderr, "Failed to register 'free_memstack'\n");
        return 1;
    }

    ARRP v1 = alloc_array(1, INTS_ARR);
    set_ints_elt(v1, 0, 41);
    printf("v1: %d\n", ints_elt(v1, 0));

    ARRP v2 = alloc_array(10, REALS_ARR);
    set_reals_elt(v2, 0, 33.33);
    real(v2)[1] = 44.44;
    printf("v2: %f\n", reals_elt(v2, 0));
    for (size_t i = 0; i < LENGTH(v2); i++) {
        printf("v2[%zu]: %f\n", i, real(v2)[i]);
    }

    ARRP v3 = alloc_array(1, STRINGS_ARR);
    set_strings_elt(v3, 0, "hello");
    printf("v3: %s\n", as_string(v3, 0));
    printf("v3: %s\n", strings_elt(v3, 0));

    printf("initial len: %zu\n", memstack.len);
    for (struct DLNode *n = memstack.tail; n != NULL; n = n->prev) {
        switch (n->vec->type) {
        case INTS_ARR:
            printf(" * data[0]: %d\n", n->vec->ints[0]);
            break;
        case REALS_ARR:
            printf(" * data[0]: %f\n", n->vec->reals[0]);
            break;
        case STRINGS_ARR:
            printf(" * data[0]: %s\n", n->vec->strings[0]);
            break;
        default:
            break;
        }
    }

    dllist_remove(&memstack, v2.node);
    printf("len: %zu\n", memstack.len);

    free_array(&v3);
    printf("len: %zu\n", memstack.len);
    return 0;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



size_t which_str(ARRP v, const char *str) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        if (strcmp(strings_elt(v, i), str) == 0) {
            return i;
        }
    }
    return LENGTH(v);
}



// VECTOR EXTENSIONS

/*allocated with same dimensions as example, so maintains matrix attributes*/
ARRP alloc_same(const ARRP v, arrtype_t type) {
    ARRP v2 = alloc_array(LENGTH(v), type);
    v2.ncols = v.ncols;
    v2.nrows = v.nrows;
    return v2;
}

ARRP copyarr(const ARRP v) {
    ARRP v2 = alloc_same(v, arrtype(v));
    switch (arrtype(v)) {
    case INTS_ARR:
        for (size_t i = 0; i < LENGTH(v); ++i) {
            integer(v2)[i] = integer(v)[i];
        }
        break;
    case REALS_ARR:
        for (size_t i = 0; i < LENGTH(v); ++i) {
            real(v2)[i] = real(v)[i];
        }
        break;
    case STRINGS_ARR:
        for (size_t i = 0; i < LENGTH(v); ++i) {
            set_strings_elt(v2, i, strings_elt(v, i));
        }
        break;
    default:
        break;
    }
    return v2;
}



ARRP add_num(ARRP v, double scalar) {
    ARRP v2 = alloc_same(v, REALS_ARR);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v2, i, reals_elt(v, i) + scalar);
    }
    return v2;
}

ARRP set_add_num(ARRP v, double scalar) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v, i, reals_elt(v, i) + scalar);
    }
    return v;
}

ARRP mul_num(ARRP v, double scalar) {
    ARRP v2 = alloc_same(v, REALS_ARR);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v2, i, reals_elt(v, i) * scalar);
    }
    return v2;
}

ARRP set_mul_num(ARRP v, double scalar) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v, i, reals_elt(v, i) * scalar);
    }
    return v;
}

ARRP div_num(ARRP v, double scalar) {
    ARRP v2 = alloc_same(v, REALS_ARR);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v2, i, reals_elt(v, i) / scalar);
    }
    return v2;
}

ARRP set_div_num(ARRP v, double scalar) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v, i, reals_elt(v, i) / scalar);
    }
    return v;
}

ARRP pow2(ARRP v) {
    ARRP v2 = alloc_same(v, REALS_ARR);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v2, i, reals_elt(v, i) * reals_elt(v, i));
    }
    return v2;
}

ARRP set_pow2(ARRP v) {
    cast_real(v); // in case input is an int vector
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v, i, reals_elt(v, i) * reals_elt(v, i));
    }
    return v;
}

ARRP vsqrt(ARRP v) {
    ARRP v2 = alloc_same(v, REALS_ARR);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v2, i, sqrt(reals_elt(v, i)));
    }
    return v2;
}

ARRP set_vsqrt(ARRP v) {
    cast_real(v); // in case input is an int vector
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v, i, sqrt(reals_elt(v, i)));
    }
    return v;
}


ARRP set_fill_num(ARRP v, double val) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v, i, val);
    }
    return v;
}

ARRP set_fill_num_rng(ARRP v, double start, double step) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v, i, start + i * step);
    }
    return v;
}

ARRP add_int(ARRP v, int scalar) {
    ARRP v2 = alloc_same(v, INTS_ARR);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v2, i, ints_elt(v, i) + scalar);
    }
    return v2;
}

ARRP set_add_int(ARRP v, int scalar) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v, i, ints_elt(v, i) + scalar);
    }
    return v;
}

ARRP mul_int(ARRP v, int scalar) {
    ARRP v2 = alloc_same(v, INTS_ARR);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v2, i, ints_elt(v, i) * scalar);
    }
    return v2;
}

ARRP set_mul_int(ARRP v, int scalar) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v, i, ints_elt(v, i) * scalar);
    }
    return v;
}

ARRP set_fill_int(ARRP v, int val) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v, i, val);
    }
    return v;
}

ARRP set_fill_str(ARRP v, const char *val) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_strings_elt(v, i, val);
    }
    return v;
}

ARRP arrp_recip(ARRP v) {
    ARRP v2 = alloc_same(v, REALS_ARR);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v2, i, 1.0 / reals_elt(v, i));
    }
    return v2;
}

ARRP set_arrp_recip(ARRP v) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_reals_elt(v, i, 1.0 / reals_elt(v, i));
    }
    return v;
}


ARRP __add(ARRP v1, ARRP v2, int inplace) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (arrtype(v1) == STRINGS_ARR || arrtype(v2) == STRINGS_ARR) {
        fprintf(stderr, "__add: not implemented for STRINGS_ARR\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "__add: lengths are not compatible\n");
        exit(1);
    }
    ARRP v3;
    if (inplace) {
        cast_real(v1);
        v3 = v1;
    } else {
        v3 = alloc_same(n1 > n2 ? v1 : v2, REALS_ARR);
    }
    if (n1 > n2) {
        // n2 is the scalar
        for (size_t i = 0; i < n1; ++i) {
            // reals_elt casts integer vector data to double
            real(v3)[i] = reals_elt(v1, i) + reals_elt(v2, 0);
        }
    } else if (n2 > n1) {
        // n1 is the scalar
        for (size_t i = 0; i < n2; ++i) {
            real(v3)[i] = reals_elt(v1, 0) + reals_elt(v2, i);
        }
    } else {
        // both vectors, equal length
        for (size_t i = 0; i < n1; ++i) {
            real(v3)[i] = reals_elt(v1, i) + reals_elt(v2, i);
        }
    }
    return v3;
}

ARRP add(const ARRP v1, const ARRP v2) {
    return __add(v1, v2, 0);
}

ARRP set_add(ARRP v1, const ARRP v2) {
    return __add(v1, v2, 1);
}


ARRP __subtract(ARRP v1, ARRP v2, int inplace) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (arrtype(v1) == STRINGS_ARR || arrtype(v2) == STRINGS_ARR) {
        fprintf(stderr, "__subtract: not implemented for STRINGS_ARR\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "__subtract: lengths are not compatible\n");
        exit(1);
    }
    ARRP v3;
    if (inplace) {
        cast_real(v1);
        v3 = v1;
    } else {
        v3 = alloc_same(n1 > n2 ? v1 : v2, REALS_ARR);
    }
    if (n1 > n2) {
        // n2 is the scalar
        for (size_t i = 0; i < n1; ++i) {
            // reals_elt casts integer vector data to double
            real(v3)[i] = reals_elt(v1, i) - reals_elt(v2, 0);
        }
    } else if (n2 > n1) {
        // n1 is the scalar
        for (size_t i = 0; i < n2; ++i) {
            real(v3)[i] = reals_elt(v1, 0) - reals_elt(v2, i);
        }
    } else {
        // both vectors, equal length
        for (size_t i = 0; i < n1; ++i) {
            real(v3)[i] = reals_elt(v1, i) - reals_elt(v2, i);
        }
    }
    return v3;
}

ARRP subtract(const ARRP v1, const ARRP v2) {
    return __subtract(v1, v2, 0);
}

ARRP set_subtract(ARRP v1, const ARRP v2) {
    return __subtract(v1, v2, 1);
}


ARRP __mul(ARRP v1, ARRP v2, int inplace) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (arrtype(v1) == STRINGS_ARR || arrtype(v2) == STRINGS_ARR) {
        fprintf(stderr, "__mul: not implemented for STRINGS_ARR\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "__mul: lengths are not compatible\n");
        exit(1);
    }
    ARRP v3;
    if (inplace) {
        cast_real(v1);
        v3 = v1;
    } else {
        v3 = alloc_same(n1 > n2 ? v1 : v2, REALS_ARR);
    }
    if (n1 > n2) {
        // n2 is the scalar
        for (size_t i = 0; i < n1; ++i) {
            // reals_elt casts integer vector data to double
            real(v3)[i] = reals_elt(v1, i) * reals_elt(v2, 0);
        }
    } else if (n2 > n1) {
        // n1 is the scalar
        for (size_t i = 0; i < n2; ++i) {
            real(v3)[i] = reals_elt(v1, 0) * reals_elt(v2, i);
        }
    } else {
        // both vectors, equal length
        for (size_t i = 0; i < n1; ++i) {
            real(v3)[i] = reals_elt(v1, i) * reals_elt(v2, i);
        }
    }
    return v3;
}

ARRP mul(const ARRP v1, const ARRP v2) {
    return __mul(v1, v2, 0);
}

ARRP set_mul(ARRP v1, const ARRP v2) {
    return __mul(v1, v2, 1);
}



ARRP __div(ARRP v1, ARRP v2, int inplace) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (arrtype(v1) == STRINGS_ARR || arrtype(v2) == STRINGS_ARR) {
        fprintf(stderr, "__div: not implemented for STRINGS_ARR\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "__div: lengths are not compatible\n");
        exit(1);
    }
    ARRP v3;
    if (inplace) {
        cast_real(v1);
        v3 = v1;
    } else {
        v3 = alloc_same(n1 > n2 ? v1 : v2, REALS_ARR);
    }
    if (n1 > n2) {
        // n2 is the scalar
        for (size_t i = 0; i < n1; ++i) {
            // reals_elt casts integer vector data to double
            real(v3)[i] = reals_elt(v1, i) / reals_elt(v2, 0);
        }
    } else if (n2 > n1) {
        // n1 is the scalar
        for (size_t i = 0; i < n2; ++i) {
            real(v3)[i] = reals_elt(v1, 0) / reals_elt(v2, i);
        }
    } else {
        // both vectors, equal length
        for (size_t i = 0; i < n1; ++i) {
            real(v3)[i] = reals_elt(v1, i) / reals_elt(v2, i);
        }
    }
    return v3;
}

ARRP divide(const ARRP v1, const ARRP v2) {
    return __div(v1, v2, 0);
}

ARRP set_divide(ARRP v1, const ARRP v2) {
    return __div(v1, v2, 1);
}


ARRP alloc_matrix(size_t nrows, size_t ncols) {
    ARRP m = alloc_array(nrows * ncols, REALS_ARR);
    m.nrows = nrows;
    m.ncols = ncols;
    return m;
}

size_t nrow(const ARRP v) {
    return v.nrows;
}

size_t ncol(const ARRP v) {
    return v.ncols;
}

ARRP set_asmatrix(ARRP v, size_t nrows, size_t ncols) {
    if (LENGTH(v) != nrows * ncols) {
        fprintf(stderr, "as_matrix: lengths are not compatible\n");
        exit(1);
    }
    cast_real(v); // in case input is an int vector
    v.ncols = ncols;
    v.nrows = nrows;
    return v;
}

ARRP set_asvector(ARRP v) {
    if (v.ncols == 0)
        return v; // already a vector
    v.nrows = 0;
    v.ncols = 0;
    return v;
}

void free_matrix(ARRP *m) {
    free_array(m);
}

int is_matrix(const ARRP m) {
    if (m.ncols > 0 && m.nrows > 0)
        return 1;
    return 0;
}

void assert_matrix(const ARRP m) {
    if (m.ncols == 0) {
        fprintf(stderr, "assert_matrix: ncols == 0, not a matrix\n");
        exit(1);
    }
}


/*repurpose a matrix by resizing the underlying vector as needed*/
ARRP resize_matrix(ARRP *m, size_t nrows, size_t ncols) {
    assert_matrix(*m);
    if (nrows == 0 || ncols == 0) {
        fprintf(stderr, "resize_matrix: nrows or ncols == 0\n");
        exit(1);
    }
    if (nrows == m->nrows && ncols == m->ncols) {
        return *m;
    }
    resize_array(m, nrows * ncols); // sets length to nrows * ncols
    m->ncols = ncols;
    m->nrows = nrows;
    return *m;
}

double MATRIX_ELT(const ARRP m, size_t i, size_t j) {
    assert_matrix(m);
    return real(m)[i * m.ncols + j];
}

void SET_MATRIX_ELT(ARRP m, size_t i, size_t j, double val) {
    assert_matrix(m);
    real(m)[i * m.ncols + j] = val;
}

ARRP MATRIX_ROW(const ARRP m, size_t i) {
    assert_matrix(m);
    ARRP row = alloc_array(m.ncols, REALS_ARR);
    size_t rowstart = i * m.ncols;
    for (size_t j = 0; j < m.ncols; ++j) {
        set_reals_elt(row, j, real(m)[rowstart + j]);
    }
    return row;
}

/*use ncols as upper index boundary*/
double *MATRIX_ROW_PTR(const ARRP m, size_t i) {
    assert_matrix(m);
    return real(m) + i * m.ncols;
}

void SET_MATRIX_ROW(ARRP m, size_t i, const double *row) {
    assert_matrix(m);
    size_t rowstart = i * m.ncols;
    for (size_t j = 0; j < m.ncols; ++j) {
        real(m)[rowstart + j] = row[j];
    }
}

ARRP MATRIX_COL(const ARRP m, size_t j) {
    assert_matrix(m);
    ARRP col = alloc_array(m.nrows, REALS_ARR);
    for (size_t i = 0; i < m.nrows; ++i) {
        set_reals_elt(col, i, MATRIX_ELT(m, i, j));
    }
    return col;
}

void SET_MATRIX_COL(ARRP m, size_t j, const double *col) {
    assert_matrix(m);
    for (size_t i = 0; i < m.nrows; ++i) {
        SET_MATRIX_ELT(m, i, j, col[i]);
    }
}


ARRP matmul(const ARRP m1, const ARRP m2) {
    if (m1.ncols != m2.nrows) {
        fprintf(stderr, "matmul: m1.ncols != m2.nrows\n");
        exit(1);
    }
    ARRP out = alloc_matrix(m1.nrows, m2.ncols);
    double sum;
    for (size_t i = 0; i < m1.nrows; ++i) {
        for (size_t j = 0; j < m2.ncols; ++j) {
            sum = 0;
            for (size_t k = 0; k < m1.ncols; ++k) {
                sum += MATRIX_ELT(m1, i, k) * MATRIX_ELT(m2, k, j);
            }
            SET_MATRIX_ELT(out, i, j, sum);
        }
    }
    return out;
}

/*
    recycles m1 to store m3
    HANS
    TODO: undesirable behavior in the set...matrix functions because setting
     new values for ncols and nrows requires input to be a pointer (ie, *ARRP)
     for the functions to work without assignment. So these produce diff behavior:
        set_matmul(X, Y)     // ncols and nrows not updated
        X = set_matmul(X, Y) // ncols and nrows updates as expected
    Could add a dimension feature to the underlying vector struct to clean this
    all up... this could be a good idea anyway.
*/
ARRP set_matmul(ARRP m1, const ARRP m2) {
    ARRP m3 = matmul(m1, m2);
    m1 = resize_matrix(&m1, m3.nrows, m3.ncols);
    for (size_t i = 0; i < LENGTH(m1); ++i) {
        real(m1)[i] = real(m3)[i];
    }
    free_matrix(&m3);
    return m1;
}


ARRP transp(const ARRP m) {
    ARRP mt = alloc_matrix(m.ncols, m.nrows);
    for (size_t i = 0; i < m.nrows; ++i) {
        for (size_t j = 0; j < m.ncols; ++j) {
            SET_MATRIX_ELT(mt, j, i, MATRIX_ELT(m, i, j));
        }
    }
    return mt;
}

ARRP set_transp(ARRP m) {
    size_t n = nrow(m);
    size_t p = ncol(m);
    // special cases, avoid allocating temporary matrix
    if (n == 0 || n == 1) {
        // vector (not yet a matrix) or row vector
        cast_real(m); // in case input is an int vector
        m.ncols = 1;
        m.nrows = p;
        return m;
    } else if (p == 1) {
        // column vector
        m.ncols = n;
        m.nrows = 1;
        return m;
    }
    // else matrix
    ARRP mt = transp(m);
    m.nrows = mt.nrows;
    m.ncols = mt.ncols;
    for (size_t i = 0; i < LENGTH(m); ++i) {
        real(m)[i] = real(mt)[i];
    }
    free_matrix(&mt);
    return m;
}

/*Q^T Q*/
ARRP crossprod(const ARRP m1, const ARRP m2) {
    ARRP mt = transp(m1);
    set_matmul(mt, m2);
    return mt;
}

/*Q Q^T*/
ARRP tcrossprod(const ARRP m1, const ARRP m2) {
    ARRP mt = transp(m2);
    set_matmul(m1, mt);
    return mt;
}


void print_matrix(const ARRP m) {
    // if not matrix, print vector horizontally
    size_t nrow = (m.nrows) ? m.nrows : 1;
    size_t ncol = (m.ncols) ? m.ncols : LENGTH(m);
    for (size_t i = 0; i < nrow; ++i) {
        for (size_t j = 0; j < ncol; ++j) {
            printf("%f\t", real(m)[i * m.ncols + j]);
        }
        putchar('\n');
    }
}


/*
    Solve R u = y for u where R is an upper right triangular matrix, using back
    substitution. Doesn't check for upper triangularity.
*/
ARRP backsub_uppertri(const ARRP R, const ARRP y) {
    size_t p = ncol(R);
    ARRP beta = alloc_array(p, REALS_ARR);
    double summ;
    for (size_t j = p; j > 0; --j) {
        summ = reals_elt(y, j - 1);
        for (size_t i = j; i < p; ++i) {
            summ -= MATRIX_ELT(R, j - 1, i) * reals_elt(beta, i);
        }
        real(beta)[j - 1] = summ / MATRIX_ELT(R, j - 1, j - 1);
    }
    return beta;
}

/*
    Invert an upper right triangular matrix using back substitution.
    Doesn't check for upper triangularity.
    Example:
     Compute $(X^\top X)^{-1}$ after QR decomposition because $R$ is equivalent
     to $L^\top L$ where $L$ is the lower triangular from Choleski decomposition
     of X, for which
     $(X^\top X)^{-1} = (L^{-1})^\top L^{-1} = R^{-1} (R^{-1})^\top$
*/
ARRP invert_upper_right(const ARRP R) {
    assert_matrix(R);
    size_t p = ncol(R);
    /*
        Create p x p diag matrix of 1s to solve
          R u = I
        Minimize allocations by setting Rinv = I to start. 
    */
    ARRP Rinv = alloc_matrix(p, p);
    for (size_t i = 0; i < p; ++i) SET_MATRIX_ELT(Rinv, i, i, 1.);
    /*
        invert upper right triangular matrix via back substitution
    */
    double elt;
    double *row, *rowprev;
    size_t i, j, k;
    for (j = p; j > 0; --j) {
        /*Rinv[j, ] <- y[j, ]*/
        for (i = j; i < p; ++i) {
            /*
            Rinv[j, ] <- Rinv[j, ] - R[j, i] * Rinv[i, ]
                avoid extra allocations needed for:
                  rowprev = MATRIX_ROW(Rinv, i); // row to subtract
                  row = set_add(row, set_mul_num(rowprev, -elt));
            */
            row = MATRIX_ROW_PTR(Rinv, j - 1);
            rowprev = MATRIX_ROW_PTR(Rinv, i);
            elt = MATRIX_ELT(R, j - 1, i);
            for (k = 0; k < p; ++k) {
                row[k] -= elt * rowprev[k];
            }
        }
        /*
        Rinv[j, ] <- Rinv[j, ] / R[j, j]
            avoid extra allocations needed for:
              row = set_div_num(row, MATRIX_ELT(R, j - 1, j - 1));
              SET_MATRIX_ROW(Rinv, j - 1, row);
              free_array(&row);
        */
        elt = MATRIX_ELT(R, j - 1, j - 1);
        for (k = 0; k < p; ++k) {
            MATRIX_ROW_PTR(Rinv, j - 1)[k] /= elt;
        }
    }
    return Rinv;
}


// developing set_matmul...
int main_00() {
    init_memstack();
    ARRP X = alloc_matrix(5, 3);
    set_fill_num(X, 1, 0);

    ARRP Y = alloc_matrix(3, 2);
    set_fill_num(Y, 1, 1);

    ARRP Z = matmul(X, Y);
    printf("Z:\n"); print_matrix(Z); putchar('\n');

    X = set_matmul(X, Y);
    printf("X:\n"); print_matrix(X); putchar('\n');

    // printf("nrow: %zu, ncol: %zu\n", nrow(X), ncol(X));
    return 0;
}


/*
    qr decomposition by gram schmidt to obtain least squares estimates
    TODO: check for singularity?

    TODO: allow for weight matrix for WLS
      - simple, X becomes W^(1/2) X (McCullagh and Nelder, pg 88)
      because beta = (X^t W X)^-1 X^t W y, and W is symmetric and positive
      definite which allows us to write W = W^(1/2) W^(1/2)
      - so just pre-matmul X by W^(1/2) and y by W^(1/2)
*/
int main() {
    init_memstack();
    const double data[12] = {1.,2.,3.,
                             1.,3.,9.,
                             1.,9.,10.,
                             1.,4.,5.};
    const ARRP X = alloc_matrix(4, 3);
        for (size_t i = 0; i < LENGTH(X); ++i) real(X)[i] = data[i]; 
        printf("X:\n"); print_matrix(X); putchar('\n');
    const ARRP y = alloc_array(4, REALS_ARR); set_fill_num(y, 1, 0);
        for (size_t i = 0; i < LENGTH(y); ++i) real(y)[i] = i + 1;
        printf("y:\n"); print_matrix(y); putchar('\n');
// void qr_decomp(const ARRP X, const ARRP y, ARRP *Qin, ARRP *Rin) {
    assert_matrix(X);
    ARRP ymat;
    int free_y = 0;
    if (is_matrix(y)) {
        ymat = y;
    } else {
        ymat = copyarr(y);
        ymat = set_asmatrix(ymat, nrow(X), 1);
        free_y = 1;
    }
    size_t n = nrow(X);
    size_t p = ncol(X);
    ARRP Q; // = *Qin;
    Q = alloc_matrix(n, p);
    ARRP R; // = *Rin;
    R = alloc_matrix(p, p);
    /*
        Gram-Schmidt orthonormalization
    */
    printf("1: memstack.len: %zu\n", memstack.len);
    ARRP v, u, Rij, tmp;
    double val;
    for (size_t j = 0; j < p; ++j) {
        v = set_asmatrix(MATRIX_COL(X, j), n, 1); // n x 1
        u = copyarr(v);
        if (j > 0) {
            for (size_t i = 0; i < j; ++i) {
                // calculate R[i, j] = t(Q[, i]) %*% v
                Rij = set_matmul(set_asmatrix(MATRIX_COL(Q, i), 1, n), // get transpose of col (1 x n)
                                 v); // 1 x n  X  n x 1
                val = real(Rij)[0];
                SET_MATRIX_ELT(R, i, j, val);
                free_array(&Rij);
                // update u = u + (- R[i, j] * Q[, i])
                tmp = MATRIX_COL(Q, i);
                set_add(u, set_mul_num(tmp, -val));
                free_array(&tmp);
            }
        }
        // calculate R[j, j]
        tmp = set_vsqrt(crossprod(u, u)); // 1 x n  X  n x 1
        val = real(tmp)[0];
        SET_MATRIX_ELT(R, j, j, val);
        // calculate Q[, j]
        SET_MATRIX_COL(Q, j, real(set_div_num(u, val)));
        free_array(&tmp);
        free_array(&v);
        free_array(&u);
    }
    // calculate coefficients
    u = crossprod(Q, ymat);
    ARRP beta = backsub_uppertri(R, u);
    printf("beta:\n"); print_matrix(beta); putchar('\n');

    // calculate fitted values and residuals
    /*yhat = Q Q^\top y*/
    ARRP yhat = transp(Q); // Qt
    yhat = set_matmul(set_matmul(Q, yhat), ymat); // consumes Q
    // set_asvector(yhat); // convert back to vector
    printf("yhat:\n"); print_matrix(yhat); putchar('\n');
    /*resid = ymat - yhat*/
    // set_asvector(ymat); // convert back to vector
    ARRP resid = subtract(ymat, yhat);
    printf("resid:\n"); print_matrix(resid); putchar('\n');
    if (free_y) free_array(&ymat);

    // calculate sigma^2
    set_div_num(set_pow2(resid), n - p);
    double sigma2 = 0;
    for (size_t i = 0; i < LENGTH(resid); ++i) sigma2 += real(resid)[i];
    printf("sigma: %f\n", sqrt(sigma2));
    // calculate (X^t X)^-1 and SEs
    /*(X^tX)^{-1} = R^{-1} (R^{-1})^t*/
    ARRP Rinv = invert_upper_right(R);
    printf("Rinv:\n"); print_matrix(Rinv); putchar('\n');
    ARRP XtXinv = set_matmul(Rinv, transp(Rinv)); // consumes Rinv
    printf("XtXinv:\n"); print_matrix(XtXinv); putchar('\n');
    // calculate SEs
    ARRP SEs = alloc_array(p, REALS_ARR);
    for (size_t i = 0; i < p; ++i) {
        real(SEs)[i] = sqrt(MATRIX_ELT(XtXinv, i, i) * sigma2);
        printf("beta[%zu]: %f\t", i, real(beta)[i]);
        printf("SEs[%zu]: %f\n", i, real(SEs)[i]);
    }
    printf("\n2: memstack.len: %zu\n", memstack.len);
    return 0;
}


int main4() {
    init_memstack();
    // test matrix
    ARRP m1 = alloc_matrix(3, 3);
    set_fill_num(m1, 2, 0);
    print_matrix(m1);
    ARRP m0 = arrp_recip(m1);
    print_matrix(m0);
    putchar('\n');

    printf("memstack.len: %zu\n", memstack.len);
    for (size_t i = 0; i < m1.nrows; ++i) {
        ARRP row = alloc_array(m1.ncols, REALS_ARR);
        ARRP row2 = alloc_array(1, REALS_ARR); set_fill_num(row2, i * 3, 0);
        set_fill_num(row, i * 3, 0);
        set_mul(set_add(row, row), row2);
        SET_MATRIX_ROW(m1, i, real(row));
        free_array(&row);
        free_array(&row2);
        free_array(&row2); // no effect
        printf("memstack.len: %zu\n", memstack.len);
        // for (size_t j = 0; j < m1.ncols; ++j) {
        //     SET_MATRIX_ELT(m1, i, j, i * m1.ncols + j);
        // }
    }
    print_matrix(m1);
    putchar('\n');

    ARRP m2 = transp(m1);
    print_matrix(m2);
    putchar('\n');

    ARRP m3 = matmul(m2, m1);
    print_matrix(m3);
    return 0;
}


