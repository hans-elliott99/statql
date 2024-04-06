#include <stdio.h>
#include <stdlib.h> // calloc
#include <string.h> // strcmp
#include <stdarg.h> // va_list, va_start, va_end
#include <math.h> // sqrt
#include "sqlite/sqlite3.h"

#include "global.h"
#include "vector.h"
#include "list.h"
#include "memory.h"

/////////////////////////////////////////
int main2() {
    if (atexit(free_memstack)) {
        fprintf(stderr, "Failed to register 'free_memstack'\n");
        return 1;
    }

    VECP v1 = alloc_vector(1, INTS_VEC);
    set_ints_elt(v1, 0, 41);
    printf("v1: %d\n", as_int(v1, 0));

    VECP v2 = alloc_vector(10, DOUBLES_VEC);
    set_doubles_elt(v2, 0, 33.33);
    DOUBLE(v2)[1] = 44.44;
    printf("v2: %f\n", as_double(v2, 0));
    for (size_t i = 0; i < LENGTH(v2); i++) {
        printf("v2[%zu]: %f\n", i, DOUBLE(v2)[i]);
    }

    VECP v3 = alloc_vector(1, STRINGS_VEC);
    set_strings_elt(v3, 0, "hello");
    printf("v3: %s\n", as_string(v3, 0));
    printf("v3: %s\n", STRING_ELT(v3, 0));

    printf("initial len: %zu\n", memstack.len);
    for (struct DLNode *n = memstack.tail; n != NULL; n = n->prev) {
        switch (n->vec->type) {
        case INTS_VEC:
            printf(" * data[0]: %d\n", n->vec->ints[0]);
            break;
        case DOUBLES_VEC:
            printf(" * data[0]: %f\n", n->vec->doubles[0]);
            break;
        case STRINGS_VEC:
            printf(" * data[0]: %s\n", n->vec->strings[0]);
            break;
        default:
            break;
        }
    }

    dllist_remove(&memstack, v2.node);
    printf("len: %zu\n", memstack.len);

    free_vector(&v3);
    printf("len: %zu\n", memstack.len);
    return 0;
}




////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*query_buff must be freed*/
void build_sql_query(char **query_buff, const char *format_string, ...) {
    va_list argptr;
    va_start(argptr, format_string);
    // determine length needed for query
    int req_len = snprintf(NULL, 0, format_string, argptr); 
    // allocate memory for query and write it
    *query_buff = chk_malloc((req_len + 1) * sizeof(char));
    vsnprintf(*query_buff, req_len + 1, format_string, argptr);
    va_end(argptr);
}


void exec_sqlite_query(sqlite3 *db,
                       const char *query,
                       int (*callback)(void*, int, char**, char**),
                       void *callback_arg) {
    char *errptr = NULL;
    int rc;
    rc = sqlite3_exec(db, query, callback, callback_arg, &errptr);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "sqlite3 error: %s\n", errptr);
        sqlite3_free(errptr);
        exit(1);
    }
}


int print_callback(void *print_header, int argc, char **data, char **columns) {
    int *ph = (int*)print_header;
    if (*ph) {
        for (int i = 0; i < argc; i++) {
            printf("%s\t", columns[i]);
        }
        putchar('\n');
        (*ph) = 0;
    }
    for (int i = 0; i < argc; i++) {
        printf("%s\t", data[i] ? data[i] : "NULL");
    }
    putchar('\n');
    return 0;
}


void print_head(sqlite3 *db, size_t nrows) {
    if (nrows == 0) {
        nrows = 5;
    }
    // build query
    char *query;
    build_sql_query(&query, "SELECT * FROM birthwt LIMIT %zu", nrows);
    int print_header = 1;
    exec_sqlite_query(db,
                      query,
                      print_callback,
                      &print_header);
    chk_free(query);
}


/*
    simple query to get the number of rows/cols in the table
*/
int nrow_callback(void *n, int argc, char **data, char **columns) {
    *(size_t*)n = strtoul(data[0], NULL, 10);
    return 0;
}

size_t db_nrows(sqlite3 *db) {
    const char *query = "SELECT COUNT(1) FROM birthwt";
    size_t n = 0;
    exec_sqlite_query(db, query, nrow_callback, &n);
    return n;
}


int ncol_callback(void *n, int argc, char **data, char **columns) {
    *(size_t*)n = argc;
    return 0;
}

size_t db_ncols(sqlite3 *db) {
    const char *query = "SELECT * FROM birthwt LIMIT 1";
    size_t n = 0;
    exec_sqlite_query(db, query, ncol_callback, &n);
    return n;
}

int col_type_callback(void *coltypes, int argc, char **data, char **columns) {
    int *ctypes = (int*)coltypes;
    size_t col_ix = strtoul(data[0], NULL, 10);
    if (strcmp(data[2], "INTEGER") == 0) {
        ctypes[col_ix] = INTS_VEC;
    } else if (strcmp(data[2], "REAL") == 0) {
        ctypes[col_ix] = DOUBLES_VEC;
    } else if (strcmp(data[2], "TEXT") == 0) {
        ctypes[col_ix] = STRINGS_VEC;
    } else if (strcmp(data[2], "BLOB") == 0) {
        ctypes[col_ix] = STRINGS_VEC;
    } else {
        // TODO
        ctypes[col_ix] = NULL_VEC;
        printf("UNKNOWN - not implemented\n");
    }
    return 0;
}


VECP db_coltypes(sqlite3 *db, size_t ncols) {
    VECP v = alloc_vector(ncols, INTS_VEC);
    const char *query = "PRAGMA table_info(birthwt)";
    exec_sqlite_query(db, query, col_type_callback, INTEGER(v));
    return v;
}


int colnames_callback(void *vec, int argc, char **data, char **columns) {
    VECP *v = (VECP*)vec;
    for (int i = 0; i < argc; i++) {
        set_strings_elt(*v, i, columns[i]);
    }
    return 0;
}

VECP db_colnames(sqlite3 *db, size_t ncols) {
    VECP v = alloc_vector(ncols, STRINGS_VEC);
    const char *query = "SELECT * FROM birthwt LIMIT 1";
    exec_sqlite_query(db, query, colnames_callback, &v);
    return v;
}


typedef struct sqlite_table {
    char *name;
    char *dbpath;
    size_t nrows;
    size_t ncols;
    VECP colnames;
    VECP coltypes;
    sqlite3 *db;
    int ini;
} sqlite_table;


sqlite_table tab = {
    NULL,
    NULL,
    0,
    0,
    {0},
    {0},
    NULL,
    0
};


void free_sqlite_table(void);

void init_sqlite_table(const char *dbpath,
                       const char *table) {
    
    if (tab.ini) {
        fprintf(stderr, "Warning: init_sqlite_table: already initialized, freeing...\n");
        free_sqlite_table();
    }
    printf("Connecting to %s for table '%s'...\n\n", dbpath, table);
    int rc = sqlite3_open(dbpath, &tab.db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(tab.db));
        exit(1);
    }
    tab.nrows = db_nrows(tab.db);
    tab.ncols = db_ncols(tab.db);
    tab.colnames = db_colnames(tab.db, tab.ncols);
    tab.coltypes = db_coltypes(tab.db, tab.ncols);
    chk_strcpy(&tab.name, table);
    chk_strcpy(&tab.dbpath, dbpath);
    tab.ini = 1;
}


void free_sqlite_table(void) {
    printf("\nFreeing sqlite_table...\n");
    free_vector(&tab.colnames);
    free_vector(&tab.coltypes);
    chk_free(tab.name);
    chk_free(tab.dbpath);
    printf("\nClosing database connection...\n");
    if (tab.db) {
        int rc = sqlite3_close(tab.db);
        if (rc != SQLITE_OK) {
            // TODO ?
            fprintf(stderr, ":( Failed to close database: %s\n", sqlite3_errmsg(tab.db));
        }
        tab.db = NULL;
    } else{
        printf("Nothing to do :O\n");
    }
    tab.ini = 0;
}


size_t which_str(VECP v, const char *str) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        if (strcmp(STRING_ELT(v, i), str) == 0) {
            return i;
        }
    }
    return LENGTH(v);
}



// VECTOR EXTENSIONS

/*allocated with same dimensions as example, so maintains matrix attributes*/
VECP alloc_same(const VECP v, vectype_t type) {
    VECP v2 = alloc_vector(LENGTH(v), type);
    v2.ncols = v.ncols;
    v2.nrows = v.nrows;
    return v2;
}

VECP copyvec(const VECP v) {
    VECP v2 = alloc_same(v, TYPEOF(v));
    switch (TYPEOF(v)) {
    case INTS_VEC:
        for (size_t i = 0; i < LENGTH(v); ++i) {
            INTEGER(v2)[i] = INTEGER(v)[i];
        }
        break;
    case DOUBLES_VEC:
        for (size_t i = 0; i < LENGTH(v); ++i) {
            DOUBLE(v2)[i] = DOUBLE(v)[i];
        }
        break;
    case STRINGS_VEC:
        for (size_t i = 0; i < LENGTH(v); ++i) {
            set_strings_elt(v2, i, STRING_ELT(v, i));
        }
        break;
    default:
        break;
    }
    return v2;
}



VECP add_dbl(VECP v, double scalar) {
    VECP v2 = alloc_same(v, DOUBLES_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v2, i, as_double(v, i) + scalar);
    }
    return v2;
}

VECP set_add_dbl(VECP v, double scalar) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v, i, as_double(v, i) + scalar);
    }
    return v;
}

VECP mul_dbl(VECP v, double scalar) {
    VECP v2 = alloc_same(v, DOUBLES_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v2, i, as_double(v, i) * scalar);
    }
    return v2;
}

VECP set_mul_dbl(VECP v, double scalar) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v, i, as_double(v, i) * scalar);
    }
    return v;
}

VECP div_dbl(VECP v, double scalar) {
    VECP v2 = alloc_same(v, DOUBLES_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v2, i, as_double(v, i) / scalar);
    }
    return v2;
}

VECP set_div_dbl(VECP v, double scalar) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v, i, as_double(v, i) / scalar);
    }
    return v;
}

VECP pow2(VECP v) {
    VECP v2 = alloc_same(v, DOUBLES_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v2, i, as_double(v, i) * as_double(v, i));
    }
    return v2;
}

VECP set_pow2(VECP v) {
    CAST_DOUBLE(v); // in case input is an int vector
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v, i, as_double(v, i) * as_double(v, i));
    }
    return v;
}

VECP vsqrt(VECP v) {
    VECP v2 = alloc_same(v, DOUBLES_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v2, i, sqrt(as_double(v, i)));
    }
    return v2;
}

VECP set_vsqrt(VECP v) {
    CAST_DOUBLE(v); // in case input is an int vector
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v, i, sqrt(as_double(v, i)));
    }
    return v;
}


VECP set_fill_dbl(VECP v, double val) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v, i, val);
    }
    return v;
}

VECP set_fill_dbl_rng(VECP v, double start, double step) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v, i, start + i * step);
    }
    return v;
}

VECP add_int(VECP v, int scalar) {
    VECP v2 = alloc_same(v, INTS_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v2, i, as_int(v, i) + scalar);
    }
    return v2;
}

VECP set_add_int(VECP v, int scalar) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v, i, as_int(v, i) + scalar);
    }
    return v;
}

VECP mul_int(VECP v, int scalar) {
    VECP v2 = alloc_same(v, INTS_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v2, i, as_int(v, i) * scalar);
    }
    return v2;
}

VECP set_mul_int(VECP v, int scalar) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v, i, as_int(v, i) * scalar);
    }
    return v;
}

VECP set_fill_int(VECP v, int val) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v, i, val);
    }
    return v;
}

VECP set_fill_str(VECP v, const char *val) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_strings_elt(v, i, val);
    }
    return v;
}

VECP reciprocal(VECP v) {
    VECP v2 = alloc_same(v, DOUBLES_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v2, i, 1.0 / as_double(v, i));
    }
    return v2;
}

VECP set_reciprocal(VECP v) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v, i, 1.0 / as_double(v, i));
    }
    return v;
}


VECP __add(VECP v1, VECP v2, int inplace) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (TYPEOF(v1) == STRINGS_VEC || TYPEOF(v2) == STRINGS_VEC) {
        fprintf(stderr, "__add: not implemented for STRINGS_VEC\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "__add: lengths are not compatible\n");
        exit(1);
    }
    VECP v3;
    if (inplace) {
        CAST_DOUBLE(v1);
        v3 = v1;
    } else {
        v3 = alloc_same(n1 > n2 ? v1 : v2, DOUBLES_VEC);
    }
    if (n1 > n2) {
        // n2 is the scalar
        for (size_t i = 0; i < n1; ++i) {
            // as_double casts integer vector data to double
            DOUBLE(v3)[i] = as_double(v1, i) + as_double(v2, 0);
        }
    } else if (n2 > n1) {
        // n1 is the scalar
        for (size_t i = 0; i < n2; ++i) {
            DOUBLE(v3)[i] = as_double(v1, 0) + as_double(v2, i);
        }
    } else {
        // both vectors, equal length
        for (size_t i = 0; i < n1; ++i) {
            DOUBLE(v3)[i] = as_double(v1, i) + as_double(v2, i);
        }
    }
    return v3;
}

VECP add(const VECP v1, const VECP v2) {
    return __add(v1, v2, 0);
}

VECP set_add(VECP v1, const VECP v2) {
    return __add(v1, v2, 1);
}


VECP __subtract(VECP v1, VECP v2, int inplace) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (TYPEOF(v1) == STRINGS_VEC || TYPEOF(v2) == STRINGS_VEC) {
        fprintf(stderr, "__subtract: not implemented for STRINGS_VEC\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "__subtract: lengths are not compatible\n");
        exit(1);
    }
    VECP v3;
    if (inplace) {
        CAST_DOUBLE(v1);
        v3 = v1;
    } else {
        v3 = alloc_same(n1 > n2 ? v1 : v2, DOUBLES_VEC);
    }
    if (n1 > n2) {
        // n2 is the scalar
        for (size_t i = 0; i < n1; ++i) {
            // as_double casts integer vector data to double
            DOUBLE(v3)[i] = as_double(v1, i) - as_double(v2, 0);
        }
    } else if (n2 > n1) {
        // n1 is the scalar
        for (size_t i = 0; i < n2; ++i) {
            DOUBLE(v3)[i] = as_double(v1, 0) - as_double(v2, i);
        }
    } else {
        // both vectors, equal length
        for (size_t i = 0; i < n1; ++i) {
            DOUBLE(v3)[i] = as_double(v1, i) - as_double(v2, i);
        }
    }
    return v3;
}

VECP subtract(const VECP v1, const VECP v2) {
    return __subtract(v1, v2, 0);
}

VECP set_subtract(VECP v1, const VECP v2) {
    return __subtract(v1, v2, 1);
}


VECP __mul(VECP v1, VECP v2, int inplace) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (TYPEOF(v1) == STRINGS_VEC || TYPEOF(v2) == STRINGS_VEC) {
        fprintf(stderr, "__mul: not implemented for STRINGS_VEC\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "__mul: lengths are not compatible\n");
        exit(1);
    }
    VECP v3;
    if (inplace) {
        CAST_DOUBLE(v1);
        v3 = v1;
    } else {
        v3 = alloc_same(n1 > n2 ? v1 : v2, DOUBLES_VEC);
    }
    if (n1 > n2) {
        // n2 is the scalar
        for (size_t i = 0; i < n1; ++i) {
            // as_double casts integer vector data to double
            DOUBLE(v3)[i] = as_double(v1, i) * as_double(v2, 0);
        }
    } else if (n2 > n1) {
        // n1 is the scalar
        for (size_t i = 0; i < n2; ++i) {
            DOUBLE(v3)[i] = as_double(v1, 0) * as_double(v2, i);
        }
    } else {
        // both vectors, equal length
        for (size_t i = 0; i < n1; ++i) {
            DOUBLE(v3)[i] = as_double(v1, i) * as_double(v2, i);
        }
    }
    return v3;
}

VECP mul(const VECP v1, const VECP v2) {
    return __mul(v1, v2, 0);
}

VECP set_mul(VECP v1, const VECP v2) {
    return __mul(v1, v2, 1);
}



VECP __div(VECP v1, VECP v2, int inplace) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (TYPEOF(v1) == STRINGS_VEC || TYPEOF(v2) == STRINGS_VEC) {
        fprintf(stderr, "__div: not implemented for STRINGS_VEC\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "__div: lengths are not compatible\n");
        exit(1);
    }
    VECP v3;
    if (inplace) {
        CAST_DOUBLE(v1);
        v3 = v1;
    } else {
        v3 = alloc_same(n1 > n2 ? v1 : v2, DOUBLES_VEC);
    }
    if (n1 > n2) {
        // n2 is the scalar
        for (size_t i = 0; i < n1; ++i) {
            // as_double casts integer vector data to double
            DOUBLE(v3)[i] = as_double(v1, i) / as_double(v2, 0);
        }
    } else if (n2 > n1) {
        // n1 is the scalar
        for (size_t i = 0; i < n2; ++i) {
            DOUBLE(v3)[i] = as_double(v1, 0) / as_double(v2, i);
        }
    } else {
        // both vectors, equal length
        for (size_t i = 0; i < n1; ++i) {
            DOUBLE(v3)[i] = as_double(v1, i) / as_double(v2, i);
        }
    }
    return v3;
}

VECP divide(const VECP v1, const VECP v2) {
    return __div(v1, v2, 0);
}

VECP set_divide(VECP v1, const VECP v2) {
    return __div(v1, v2, 1);
}


VECP alloc_matrix(size_t nrows, size_t ncols) {
    VECP m = alloc_vector(nrows * ncols, DOUBLES_VEC);
    m.nrows = nrows;
    m.ncols = ncols;
    return m;
}

size_t nrow(const VECP v) {
    return v.nrows;
}

size_t ncol(const VECP v) {
    return v.ncols;
}

VECP set_asmatrix(VECP v, size_t nrows, size_t ncols) {
    if (LENGTH(v) != nrows * ncols) {
        fprintf(stderr, "as_matrix: lengths are not compatible\n");
        exit(1);
    }
    CAST_DOUBLE(v); // in case input is an int vector
    v.ncols = ncols;
    v.nrows = nrows;
    return v;
}

VECP set_asvector(VECP v) {
    if (v.ncols == 0)
        return v; // already a vector
    v.nrows = 0;
    v.ncols = 0;
    return v;
}

void free_matrix(VECP *m) {
    free_vector(m);
}

int is_matrix(const VECP m) {
    if (m.ncols > 0 && m.nrows > 0)
        return 1;
    return 0;
}

void assert_matrix(const VECP m) {
    if (m.ncols == 0) {
        fprintf(stderr, "assert_matrix: ncols == 0, not a matrix\n");
        exit(1);
    }
}


/*repurpose a matrix by resizing the underlying vector as needed*/
VECP resize_matrix(VECP *m, size_t nrows, size_t ncols) {
    assert_matrix(*m);
    if (nrows == 0 || ncols == 0) {
        fprintf(stderr, "resize_matrix: nrows or ncols == 0\n");
        exit(1);
    }
    if (nrows == m->nrows && ncols == m->ncols) {
        return *m;
    }
    resize_vector(m, nrows * ncols); // sets length to nrows * ncols
    m->ncols = ncols;
    m->nrows = nrows;
    return *m;
}

double MATRIX_ELT(const VECP m, size_t i, size_t j) {
    assert_matrix(m);
    return DOUBLE(m)[i * m.ncols + j];
}

void SET_MATRIX_ELT(VECP m, size_t i, size_t j, double val) {
    assert_matrix(m);
    DOUBLE(m)[i * m.ncols + j] = val;
}

VECP MATRIX_ROW(const VECP m, size_t i) {
    assert_matrix(m);
    VECP row = alloc_vector(m.ncols, DOUBLES_VEC);
    size_t rowstart = i * m.ncols;
    for (size_t j = 0; j < m.ncols; ++j) {
        set_doubles_elt(row, j, DOUBLE(m)[rowstart + j]);
    }
    return row;
}

/*use ncols as upper index boundary*/
double *MATRIX_ROW_PTR(const VECP m, size_t i) {
    assert_matrix(m);
    return DOUBLE(m) + i * m.ncols;
}

void SET_MATRIX_ROW(VECP m, size_t i, const double *row) {
    assert_matrix(m);
    size_t rowstart = i * m.ncols;
    for (size_t j = 0; j < m.ncols; ++j) {
        DOUBLE(m)[rowstart + j] = row[j];
    }
}

VECP MATRIX_COL(const VECP m, size_t j) {
    assert_matrix(m);
    VECP col = alloc_vector(m.nrows, DOUBLES_VEC);
    for (size_t i = 0; i < m.nrows; ++i) {
        set_doubles_elt(col, i, MATRIX_ELT(m, i, j));
    }
    return col;
}

void SET_MATRIX_COL(VECP m, size_t j, const double *col) {
    assert_matrix(m);
    for (size_t i = 0; i < m.nrows; ++i) {
        SET_MATRIX_ELT(m, i, j, col[i]);
    }
}


VECP matmul(const VECP m1, const VECP m2) {
    if (m1.ncols != m2.nrows) {
        fprintf(stderr, "matmul: m1.ncols != m2.nrows\n");
        exit(1);
    }
    VECP out = alloc_matrix(m1.nrows, m2.ncols);
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
     new values for ncols and nrows requires input to be a pointer (ie, *VECP)
     for the functions to work without assignment. So these produce diff behavior:
        set_matmul(X, Y)     // ncols and nrows not updated
        X = set_matmul(X, Y) // ncols and nrows updates as expected
    Could add a dimension feature to the underlying vector struct to clean this
    all up... this could be a good idea anyway.
*/
VECP set_matmul(VECP m1, const VECP m2) {
    VECP m3 = matmul(m1, m2);
    m1 = resize_matrix(&m1, m3.nrows, m3.ncols);
    for (size_t i = 0; i < LENGTH(m1); ++i) {
        DOUBLE(m1)[i] = DOUBLE(m3)[i];
    }
    free_matrix(&m3);
    return m1;
}


VECP transp(const VECP m) {
    VECP mt = alloc_matrix(m.ncols, m.nrows);
    for (size_t i = 0; i < m.nrows; ++i) {
        for (size_t j = 0; j < m.ncols; ++j) {
            SET_MATRIX_ELT(mt, j, i, MATRIX_ELT(m, i, j));
        }
    }
    return mt;
}

VECP set_transp(VECP m) {
    size_t n = nrow(m);
    size_t p = ncol(m);
    // special cases, avoid allocating temporary matrix
    if (n == 0 || n == 1) {
        // vector (not yet a matrix) or row vector
        CAST_DOUBLE(m); // in case input is an int vector
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
    VECP mt = transp(m);
    m.nrows = mt.nrows;
    m.ncols = mt.ncols;
    for (size_t i = 0; i < LENGTH(m); ++i) {
        DOUBLE(m)[i] = DOUBLE(mt)[i];
    }
    free_matrix(&mt);
    return m;
}

/*Q^T Q*/
VECP crossprod(const VECP m1, const VECP m2) {
    VECP mt = transp(m1);
    set_matmul(mt, m2);
    return mt;
}

/*Q Q^T*/
VECP tcrossprod(const VECP m1, const VECP m2) {
    VECP mt = transp(m2);
    set_matmul(m1, mt);
    return mt;
}


void print_matrix(const VECP m) {
    // if not matrix, print vector horizontally
    size_t nrow = (m.nrows) ? m.nrows : 1;
    size_t ncol = (m.ncols) ? m.ncols : LENGTH(m);
    for (size_t i = 0; i < nrow; ++i) {
        for (size_t j = 0; j < ncol; ++j) {
            printf("%f\t", DOUBLE(m)[i * m.ncols + j]);
        }
        putchar('\n');
    }
}


/*
    Solve R u = y for u where R is an upper right triangular matrix, using back
    substitution. Doesn't check for upper triangularity.
*/
VECP backsub_uppertri(const VECP R, const VECP y) {
    size_t p = ncol(R);
    VECP beta = alloc_vector(p, DOUBLES_VEC);
    double summ;
    for (size_t j = p; j > 0; --j) {
        summ = as_double(y, j - 1);
        for (size_t i = j; i < p; ++i) {
            summ -= MATRIX_ELT(R, j - 1, i) * as_double(beta, i);
        }
        DOUBLE(beta)[j - 1] = summ / MATRIX_ELT(R, j - 1, j - 1);
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
VECP invert_upper_right(const VECP R) {
    assert_matrix(R);
    size_t p = ncol(R);
    /*
        Create p x p diag matrix of 1s to solve
          R u = I
        Minimize allocations by setting Rinv = I to start. 
    */
    VECP Rinv = alloc_matrix(p, p);
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
                  row = set_add(row, set_mul_dbl(rowprev, -elt));
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
              row = set_div_dbl(row, MATRIX_ELT(R, j - 1, j - 1));
              SET_MATRIX_ROW(Rinv, j - 1, row);
              free_vector(&row);
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
    VECP X = alloc_matrix(5, 3);
    set_fill_dbl(X, 1);

    VECP Y = alloc_matrix(3, 2);
    set_fill_dbl_rng(Y, 1, 1);

    VECP Z = matmul(X, Y);
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
    const VECP X = alloc_matrix(4, 3);
        for (size_t i = 0; i < LENGTH(X); ++i) DOUBLE(X)[i] = data[i]; 
        printf("X:\n"); print_matrix(X); putchar('\n');
    const VECP y = alloc_vector(4, DOUBLES_VEC); set_fill_dbl(y, 1);
        for (size_t i = 0; i < LENGTH(y); ++i) DOUBLE(y)[i] = i + 1;
        printf("y:\n"); print_matrix(y); putchar('\n');
// void qr_decomp(const VECP X, const VECP y, VECP *Qin, VECP *Rin) {
    assert_matrix(X);
    VECP ymat;
    int free_y = 0;
    if (is_matrix(y)) {
        ymat = y;
    } else {
        ymat = copyvec(y);
        ymat = set_asmatrix(ymat, nrow(X), 1);
        free_y = 1;
    }
    size_t n = nrow(X);
    size_t p = ncol(X);
    VECP Q; // = *Qin;
    Q = alloc_matrix(n, p);
    VECP R; // = *Rin;
    R = alloc_matrix(p, p);
    /*
        Gram-Schmidt orthonormalization
    */
    printf("1: memstack.len: %zu\n", memstack.len);
    VECP v, u, Rij, tmp;
    double val;
    for (size_t j = 0; j < p; ++j) {
        v = set_asmatrix(MATRIX_COL(X, j), n, 1); // n x 1
        u = copyvec(v);
        if (j > 0) {
            for (size_t i = 0; i < j; ++i) {
                // calculate R[i, j] = t(Q[, i]) %*% v
                Rij = set_matmul(set_asmatrix(MATRIX_COL(Q, i), 1, n), // get transpose of col (1 x n)
                                 v); // 1 x n  X  n x 1
                val = DOUBLE(Rij)[0];
                SET_MATRIX_ELT(R, i, j, val);
                free_vector(&Rij);
                // update u = u + (- R[i, j] * Q[, i])
                tmp = MATRIX_COL(Q, i);
                set_add(u, set_mul_dbl(tmp, -val));
                free_vector(&tmp);
            }
        }
        // calculate R[j, j]
        tmp = set_vsqrt(crossprod(u, u)); // 1 x n  X  n x 1
        val = DOUBLE(tmp)[0];
        SET_MATRIX_ELT(R, j, j, val);
        // calculate Q[, j]
        SET_MATRIX_COL(Q, j, DOUBLE(set_div_dbl(u, val)));
        free_vector(&tmp);
        free_vector(&v);
        free_vector(&u);
    }
    // calculate coefficients
    u = crossprod(Q, ymat);
    VECP beta = backsub_uppertri(R, u);
    printf("beta:\n"); print_matrix(beta); putchar('\n');

    // calculate fitted values and residuals
    /*yhat = Q Q^\top y*/
    VECP yhat = transp(Q); // Qt
    yhat = set_matmul(set_matmul(Q, yhat), ymat); // consumes Q
    // set_asvector(yhat); // convert back to vector
    printf("yhat:\n"); print_matrix(yhat); putchar('\n');
    /*resid = ymat - yhat*/
    // set_asvector(ymat); // convert back to vector
    VECP resid = subtract(ymat, yhat);
    printf("resid:\n"); print_matrix(resid); putchar('\n');
    if (free_y) free_vector(&ymat);

    // calculate sigma^2
    set_div_dbl(set_pow2(resid), n - p);
    double sigma2 = 0;
    for (size_t i = 0; i < LENGTH(resid); ++i) sigma2 += DOUBLE(resid)[i];
    printf("sigma: %f\n", sqrt(sigma2));
    // calculate (X^t X)^-1 and SEs
    /*(X^tX)^{-1} = R^{-1} (R^{-1})^t*/
    VECP Rinv = invert_upper_right(R);
    printf("Rinv:\n"); print_matrix(Rinv); putchar('\n');
    VECP XtXinv = set_matmul(Rinv, transp(Rinv)); // consumes Rinv
    printf("XtXinv:\n"); print_matrix(XtXinv); putchar('\n');
    // calculate SEs
    VECP SEs = alloc_vector(p, DOUBLES_VEC);
    for (size_t i = 0; i < p; ++i) {
        DOUBLE(SEs)[i] = sqrt(MATRIX_ELT(XtXinv, i, i) * sigma2);
        printf("beta[%zu]: %f\t", i, DOUBLE(beta)[i]);
        printf("SEs[%zu]: %f\n", i, DOUBLE(SEs)[i]);
    }
    printf("\n2: memstack.len: %zu\n", memstack.len);
    return 0;
}


int main4() {
    init_memstack();
    // test matrix
    VECP m1 = alloc_matrix(3, 3);
    set_fill_dbl(m1, 2);
    print_matrix(m1);
    VECP m0 = reciprocal(m1);
    print_matrix(m0);
    putchar('\n');

    printf("memstack.len: %zu\n", memstack.len);
    for (size_t i = 0; i < m1.nrows; ++i) {
        VECP row = alloc_vector(m1.ncols, DOUBLES_VEC);
        VECP row2 = alloc_vector(1, DOUBLES_VEC); set_fill_dbl(row2, i * 3);
        set_fill_dbl(row, i * 3);
        set_mul(set_add(row, row), row2);
        SET_MATRIX_ROW(m1, i, DOUBLE(row));
        free_vector(&row);
        free_vector(&row2);
        free_vector(&row2); // no effect
        printf("memstack.len: %zu\n", memstack.len);
        // for (size_t j = 0; j < m1.ncols; ++j) {
        //     SET_MATRIX_ELT(m1, i, j, i * m1.ncols + j);
        // }
    }
    print_matrix(m1);
    putchar('\n');

    VECP m2 = transp(m1);
    print_matrix(m2);
    putchar('\n');

    VECP m3 = matmul(m2, m1);
    print_matrix(m3);
    return 0;
}



int main3() {
    /*
            SETUP
    */
    if (atexit(free_memstack)) {
        fprintf(stderr, "Failed to register 'free_memstack'\n");
        return 1;
    }
    if (atexit(free_sqlite_table)) {
        fprintf(stderr, "Failed to register 'free_sqlite_table'\n");
        return 1;
    }

    init_sqlite_table("test.db", "birthwt");
    /*
            QUERY TABLE INFO
    */
    print_head(tab.db, 10);
    printf("nrows: %zu\n", tab.nrows);
    printf("ncols: %zu\n", tab.ncols);
    /*
            READ DATA INTO VECTORS...
    */
    for (size_t i = 0; i < tab.ncols; i++) {
        printf("%s (%s) \n",
               STRING_ELT(tab.colnames, i),
               vectype_str(as_int(tab.coltypes, i))
               );
    }
    return 0;
}
