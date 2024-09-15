#include <stdio.h>
#include <stdarg.h>  // va_list, va_start, va_end
#include <string.h> // strcmp

#include "global.h"
#include "memory.h"
#include "sqlite/sqlite3.h"

#include "examples/read_sqlite_table.h"



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
        ctypes[col_ix] = INTS_ARR;
    } else if (strcmp(data[2], "REAL") == 0) {
        ctypes[col_ix] = REALS_ARR;
    } else if (strcmp(data[2], "TEXT") == 0) {
        ctypes[col_ix] = STRINGS_ARR;
    } else if (strcmp(data[2], "BLOB") == 0) {
        ctypes[col_ix] = STRINGS_ARR;
    } else {
        // TODO
        ctypes[col_ix] = STRINGS_ARR;
        fprintf(stderr,
                "Warning: col_type_callback: unexpected sqlite column type: %s\n",
                data[2]);
    }
    return 0;
}


ARRP db_coltypes(sqlite3 *db, size_t ncols) {
    ARRP v = alloc_row_array(INTS_ARR, ncols);
    const char *query = "PRAGMA table_info(birthwt)";
    exec_sqlite_query(db, query, col_type_callback, integer(v));
    return v;
}


int colnames_callback(void *vec, int argc, char **data, char **columns) {
    ARRP *v = (ARRP*)vec;
    for (int i = 0; i < argc; i++) {
        set_strings_elt(*v, 0, i, columns[i]);
    }
    return 0;
}

ARRP db_colnames(sqlite3 *db, size_t ncols) {
    ARRP v = alloc_row_array(STRINGS_ARR, ncols);
    const char *query = "SELECT * FROM birthwt LIMIT 1";
    exec_sqlite_query(db, query, colnames_callback, &v);
    return v;
}


typedef struct sqlite_table {
    char *name;
    char *dbpath;
    size_t nrows;
    size_t ncols;
    ARRP colnames;
    ARRP coltypes;
    sqlite3 *db;
    int ini;
} sqlite_table;


sqlite_table TAB = {
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
    
    if (TAB.ini) {
        fprintf(stderr, "Warning: init_sqlite_table: already initialized, freeing...\n");
        free_sqlite_table();
    }
    printf("Connecting to %s for table '%s'...\n\n", dbpath, table);
    int rc = sqlite3_open(dbpath, &TAB.db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(TAB.db));
        exit(1);
    }
    TAB.nrows = db_nrows(TAB.db);
    TAB.ncols = db_ncols(TAB.db);
    TAB.colnames = db_colnames(TAB.db, TAB.ncols);
    TAB.coltypes = db_coltypes(TAB.db, TAB.ncols);
    chk_strcpy(&TAB.name, table);
    chk_strcpy(&TAB.dbpath, dbpath);
    TAB.ini = 1;
}


void free_sqlite_table(void) {
    printf("\nFreeing sqlite_table...\n");
    free_array(&TAB.colnames);
    free_array(&TAB.coltypes);
    chk_free(TAB.name);
    chk_free(TAB.dbpath);
    printf("Closing database connection...\n");
    if (TAB.db) {
        int rc = sqlite3_close(TAB.db);
        if (rc != SQLITE_OK) {
            // TODO ?
            fprintf(stderr, ":( Failed to close database: %s\n", sqlite3_errmsg(TAB.db));
        }
        TAB.db = NULL;
    } else{
        printf("Nothing to do :O\n");
    }
    TAB.ini = 0;
}





int example__read_sqlite_table(void) {
    /*
            SETUP
    */
   init_memstack();
    
    if (atexit(free_sqlite_table)) {
        fprintf(stderr, "Failed to register 'free_sqlite_table'\n");
        return 1;
    }

    init_sqlite_table("test.db", "birthwt");
    /*
            QUERY TABLE INFO
    */
    print_head(TAB.db, 10);
    printf("nrows: %zu\n", TAB.nrows);
    printf("ncols: %zu\n", TAB.ncols);
    /*
            READ DATA INTO VECTORS...
    */
    for (size_t i = 0; i < TAB.ncols; i++) {
        printf("%s (%s) \n",
               strings_elt(TAB.colnames, 0, i),
               arrtype_str(ints_elt(TAB.coltypes, 0, i))
               );
    }
    return 0;
}


// TODO - can wrap this up into a function that initializes a data_frame
//     that is an array with the additional data set information