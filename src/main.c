#include <stdio.h>
#include <stdlib.h> // calloc
#include <string.h> // strcmp
#include <stdarg.h> // va_list, va_start, va_end
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

    free_vector(v3);
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
    free_vector(tab.colnames);
    free_vector(tab.coltypes);
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

void fill_dbl(VECP v, double val) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v, i, val);
    }
}

void fill_int(VECP v, int val) {
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v, i, val);
    }
}


VECP addvec(VECP v1, VECP v2) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (TYPEOF(v1) == STRINGS_VEC || TYPEOF(v2) == STRINGS_VEC) {
        fprintf(stderr, "addvec: not implemented for STRINGS_VEC\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "addvec: lengths are not compatible\n");
        exit(1);
    }
    VECP v3 = alloc_vector(n1 > n2 ? n1 : n2, DOUBLES_VEC);
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


/*add v2 to v1, in place*/
VECP addveci(VECP v1, VECP v2) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (TYPEOF(v1) == INTS_VEC) {
        CAST_DOUBLE(v1);
    }
    if (TYPEOF(v1) == STRINGS_VEC || TYPEOF(v2) == STRINGS_VEC) {
        fprintf(stderr, "addvec: not implemented for STRINGS_VEC\n");
        exit(1);
    }
    if (n1 != n2 && n2 != 1) {
        fprintf(stderr, "addvec: lengths are not compatible\n");
        exit(1);
    }
    if (n1 > n2) {
        // n2 is the scalar
        for (size_t i = 0; i < n1; ++i) {
            // as_double casts integer vector data to double
            DOUBLE(v1)[i] = as_double(v1, i) + as_double(v2, 0);
        }
    } else {
        // both vectors, equal length
        for (size_t i = 0; i < n1; ++i) {
            DOUBLE(v1)[i] = as_double(v1, i) + as_double(v2, i);
        }
    }
    return v1;
}


VECP mulvec(VECP v1, VECP v2) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (TYPEOF(v1) == STRINGS_VEC || TYPEOF(v2) == STRINGS_VEC) {
        fprintf(stderr, "mulvec: not implemented for STRINGS_VEC\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "mulvec: lengths are not compatible\n");
        exit(1);
    }
    VECP v3 = alloc_vector(n1 > n2 ? n1 : n2, DOUBLES_VEC);
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

VECP divvec(VECP v1, VECP v2) {
    size_t n1 = LENGTH(v1);
    size_t n2 = LENGTH(v2);
    if (TYPEOF(v1) == STRINGS_VEC || TYPEOF(v2) == STRINGS_VEC) {
        fprintf(stderr, "divvec: not implemented for STRINGS_VEC\n");
        exit(1);
    }
    if (n1 != n2 && n1 != 1 && n2 != 1) {
        fprintf(stderr, "divvec: lengths are not compatible\n");
        exit(1);
    }
    VECP v3 = alloc_vector(n1 > n2 ? n1 : n2, DOUBLES_VEC);
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

VECP add_dbl(VECP v, double scalar) {
    VECP v2 = alloc_vector(LENGTH(v), DOUBLES_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v2, i, as_double(v, i) + scalar);
    }
    return v2;
}

VECP mul_dbl(VECP v, double scalar) {
    VECP v2 = alloc_vector(LENGTH(v), DOUBLES_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v2, i, as_double(v, i) * scalar);
    }
    return v2;
}

VECP add_int(VECP v, int scalar) {
    VECP v2 = alloc_vector(LENGTH(v), INTS_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v2, i, as_int(v, i) + scalar);
    }
    return v2;
}

VECP mul_int(VECP v, int scalar) {
    VECP v2 = alloc_vector(LENGTH(v), INTS_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_ints_elt(v2, i, as_int(v, i) * scalar);
    }
    return v2;
}

VECP reciprocal(VECP v) {
    VECP v2 = alloc_vector(LENGTH(v), DOUBLES_VEC);
    for (size_t i = 0; i < LENGTH(v); ++i) {
        set_doubles_elt(v2, i, 1.0 / as_double(v, i));
    }
    return v2;
}


typedef struct MATP {
    struct DLNode *node;
    size_t nrows;
    size_t ncols;
} MATP;

MATP alloc_matrix(size_t nrows, size_t ncols) {
    MATP m;
    m.node = chk_malloc(sizeof(struct DLNode));
    m.node->vec = chk_malloc(sizeof(VectorStruct));
    alloc_vector_struct(m.node->vec, nrows * ncols, DOUBLES_VEC);
    m.nrows = nrows;
    m.ncols = ncols;
    dllist_append(&memstack, m.node);
    return m;
}

MATP free_matrix(MATP m) {
    dllist_remove(&memstack, m.node);
    return m;
}

double MATRIX_ELT(MATP m, size_t i, size_t j) {
    return m.node->vec->doubles[i * m.ncols + j];
}

void SET_MATRIX_ELT(MATP m, size_t i, size_t j, double val) {
    m.node->vec->doubles[i * m.ncols + j] = val;
}

VECP MATRIX_ROW(MATP m, size_t i) {
    VECP row = alloc_vector(m.ncols, DOUBLES_VEC);
    size_t rowstart = i * m.ncols;
    for (size_t j = 0; j < m.ncols; ++j) {
        set_doubles_elt(row, j, m.node->vec->doubles[rowstart + j]);
    }
    return row;
}

double *MATRIX_ROW_PTR(MATP m, size_t i) {
    return m.node->vec->doubles + i * m.ncols;
}

void SET_MATRIX_ROW(MATP m, size_t i, double *row) {
    size_t rowstart = i * m.ncols;
    for (size_t j = 0; j < m.ncols; ++j) {
        m.node->vec->doubles[rowstart + j] = row[j];
    }
}

VECP MATRIX_COL(MATP m, size_t j) {
    VECP col = alloc_vector(m.nrows, DOUBLES_VEC);
    for (size_t i = 0; i < m.nrows; ++i) {
        set_doubles_elt(col, i, MATRIX_ELT(m, i, j));
    }
    return col;
}

void SET_MATRIX_COL(MATP m, size_t j, double *col) {
    for (size_t i = 0; i < m.nrows; ++i) {
        SET_MATRIX_ELT(m, i, j, col[i]);
    }
}



void fill_matrix(MATP m, double val) {
    for (size_t i = 0; i < m.nrows; ++i) {
        for (size_t j = 0; j < m.ncols; ++j) {
            SET_MATRIX_ELT(m, i, j, val);
        }
    }
}


MATP matmul(MATP m1, MATP m2) {
    if (m1.ncols != m2.nrows) {
        fprintf(stderr, "matmul: m1.ncols != m2.nrows\n");
        exit(1);
    }
    MATP m3 = alloc_matrix(m1.nrows, m2.ncols);
    for (size_t i = 0; i < m1.nrows; ++i) {
        for (size_t j = 0; j < m2.ncols; ++j) {
            double sum = 0;
            for (size_t k = 0; k < m1.ncols; ++k) {
                sum += MATRIX_ELT(m1, i, k) * MATRIX_ELT(m2, k, j);
            }
            SET_MATRIX_ELT(m3, i, j, sum);
        }
    }
    return m3;
}

MATP transpose(MATP m) {
    MATP mt = alloc_matrix(m.ncols, m.nrows);
    for (size_t i = 0; i < m.nrows; ++i) {
        for (size_t j = 0; j < m.ncols; ++j) {
            SET_MATRIX_ELT(mt, j, i, MATRIX_ELT(m, i, j));
        }
    }
    return mt;
}


void print_matrix(MATP m) {
    for (size_t i = 0; i < m.nrows; ++i) {
        for (size_t j = 0; j < m.ncols; ++j) {
            printf("%f\t", MATRIX_ELT(m, i, j));
        }
        putchar('\n');
    }
}

int main() {
    init_memstack();
    // test matrix
    printf("memstack.len: %zu\n", memstack.len);
    MATP m1 = alloc_matrix(3, 3);
    for (size_t i = 0; i < m1.nrows; ++i) {
        VECP row = alloc_vector(m1.ncols, DOUBLES_VEC);
        fill_dbl(row, i * 3);
        VECP row2 = mulvec(addveci(row, row), row);
        SET_MATRIX_ROW(m1, i, DOUBLE(row2));
        free_vector(row);
        free_vector(row2);
        printf("memstack.len: %zu\n", memstack.len);
        // for (size_t j = 0; j < m1.ncols; ++j) {
        //     SET_MATRIX_ELT(m1, i, j, i * m1.ncols + j);
        // }
    }
    print_matrix(m1);
    putchar('\n');

    MATP m2 = transpose(m1);
    print_matrix(m2);
    putchar('\n');

    MATP m3 = matmul(m2, m1);
    print_matrix(m3);
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
