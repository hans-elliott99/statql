
#include <stdio.h>

#include <stdlib.h> // calloc
#include <string.h> // strcmp
#include <stdarg.h> // va_list, va_start, va_end

#include "sqlite/sqlite3.h"

int mem = 0;
/////////////////////////////////////////

void *chk_malloc(size_t memsize) {
    void *p;
    if (memsize == 0)
        return NULL;
    p = malloc(memsize);
    if (!p) {
        fprintf(stderr, "chk_malloc: memory allocation failed!\n");
        exit(1);
    }
    mem++;
    return p;
}
void *chk_calloc(size_t num, size_t elem_size) {
    void *p;
    if (num == 0)
        return NULL;
    p = calloc(num, elem_size);
    if (!p) {
        fprintf(stderr, "chk_calloc: memory allocation failed!\n");
        exit(1);
    }
    mem++;
    return p;
}
void chk_free(void *p) {
    if (p) {
        free(p);
        mem--;
    }
}


typedef enum {
    INTS_VEC = 0,
    DOUBLES_VEC,
    STRINGS_VEC,
    NULL_VEC
} vectype_t;

const char *vectype_str(vectype_t t) {
    switch (t) {
        case INTS_VEC:
            return "INTS_VEC";
        case DOUBLES_VEC:
            return "DOUBLES_VEC";
        case STRINGS_VEC:
            return "STRINGS_VEC";
        case NULL_VEC:
            return "NULL_VEC";
        default:
            return "UNKOWN";
    }
}


typedef struct VectorStruct {
    vectype_t type;         // type of data contained by vector
    void *data;             // pointer to the memory allocated for the vector
    int *ints;              // pointer to data, if type is INTS_VEC
    double *doubles;        // pointer to data, if type is DOUBLES_VEC
    char **strings;         // pointer to data, if type is STRINGS_VEC
    size_t capacity;        // vector capacity / length
    size_t n;               // number of allocated elements (differs from capacity only for STRINGS_VEC)
} VectorStruct;

/*Doubly Linked List*/
struct DLNode {
    VectorStruct *vec;
    struct DLNode *next;
    struct DLNode *prev;
};

struct DLList {
    struct DLNode *head;
    struct DLNode *tail;
    size_t len;
};


void alloc_vector_struct(VectorStruct *v, size_t nelem, vectype_t type) {
    v->type = type;
    v->capacity = nelem;
    v->n = (type == STRINGS_VEC) ? 0 : nelem; // inidiv strings need allocation
    v->data = NULL;
    v->ints = NULL;
    v->doubles = NULL;
    v->strings = NULL;
    switch (type) {
        case INTS_VEC:
            v->data = chk_calloc(nelem, sizeof(int));
            v->ints = (int*)v->data;
            break;
        case DOUBLES_VEC:
            v->data = chk_calloc(nelem, sizeof(double));
            v->doubles = (double*)v->data;
            break;
        case STRINGS_VEC:
            v->data = chk_calloc(nelem, sizeof(char*));
            v->strings = (char**)v->data;
        case NULL_VEC:
            break;
        default:
            fprintf(stderr, "alloc_vector_struct: unknown type: %d\n", type);
            exit(1);
    }
}

void free_vector_struct(VectorStruct *v) {
    if (v->type == STRINGS_VEC) {
        for (size_t i = 0; i < v->n; i++) {
            chk_free(v->strings[i]);
        }
    }
    chk_free(v->data);
    v->data = NULL;
    v->ints = NULL;
    v->doubles = NULL;
    v->strings = NULL;
    v->n = 0;
    v->capacity = 0;
}


void dllist_append(struct DLList *list, struct DLNode *newnode) {
    if (list->len == 0) {
        // first node
        list->head = newnode;
        list->tail = newnode;
        newnode->prev = NULL;
        newnode->next = NULL;
    } else {
        if (list->tail->next != NULL) {
            fprintf(stderr, "dllist_append: tail->next not NULL\n");
            exit(1);
        }
        list->tail->next = newnode;
        newnode->prev = list->tail;
        list->tail = newnode;
    }
    list->len++;
}

void dllist_remove(struct DLList *list, struct DLNode *node) {
    if (node->prev == NULL) { // if node is head
        list->head = node->next;
    } else {
        node->prev->next = node->next;
    }
    if (node->next == NULL) { // if node is tail
        list->tail = node->prev;
    } else {
        node->next->prev = node->prev;
    }
    free_vector_struct(node->vec);
    chk_free(node->vec);
    chk_free(node);
    list->len--;
}

void dllist_pop(struct DLList *list) {
    if (list->len == 0) {
        return;
    }
    struct DLNode *last = list->tail;
    if (list->len == 1) {
        list->head = NULL;
        list->tail = NULL;
    } else {
        list->tail = last->prev;
        list->tail->next = NULL;
    }
    free_vector_struct(last->vec);
    chk_free(last->vec);
    chk_free(last);
    list->len--;
}



struct DLList memstack = {NULL, NULL, 0};

typedef struct VECP {
    struct DLNode *node;
    vectype_t type;
    size_t *n;
    size_t *capacity;
} VECP;

VECP alloc_vector(size_t n, vectype_t type) {
    VECP v;
    v.type = type;
    v.node = chk_malloc(sizeof(struct DLNode));
    v.node->vec = chk_malloc(sizeof(VectorStruct));
    alloc_vector_struct(v.node->vec, n, type);
    dllist_append(&memstack, v.node);
    v.n = &v.node->vec->n;
    v.capacity = &v.node->vec->capacity;
    return v;
}

void print_vec_elt(VECP v, size_t idx) {
    switch (v.type) {
    case INTS_VEC:
        printf("%d", v.node->vec->ints[idx]);
        break;
    case DOUBLES_VEC:
        printf("%f", v.node->vec->doubles[idx]);
        break;
    case STRINGS_VEC:
        printf("%s", v.node->vec->strings[idx]);
        break;
    default:
        break;
    }
}

int as_int(VECP v, size_t idx) {
    if (idx >= *v.n) {
        fprintf(stderr, "as_int: index out of bounds: %zu\n", idx);
        exit(1);
    }
    if (v.type == STRINGS_VEC) {
        fprintf(stderr, "as_int: not implemented for STRINGS_VEC");
        exit(1);
    }
    // if double, casted to integer
    return ((int *)v.node->vec->data)[idx];
}

void set_ints_elt(VECP v, size_t idx, int val) {
    if (v.type != INTS_VEC) {
        fprintf(stderr, "set_int: val is of type %s, expected INTS_VEC\n",
                vectype_str(v.type));
        exit(1);
    }
    if (idx >= *v.n) {
        fprintf(stderr, "set_int: index out of bounds: %zu\n", idx);
        exit(1);
    }
    v.node->vec->ints[idx] = val;
}

double as_double(VECP v, size_t idx) {
    if (idx >= *v.n) {
        fprintf(stderr, "as_double: index out of bounds: %zu\n", idx);
        exit(1);
    }
    if (v.node->vec->type == STRINGS_VEC) {
        fprintf(stderr, "as_double: not implemented for STRINGS_VEC");
        exit(1);
    }
    // if int, casted to double
    return ((double *)v.node->vec->data)[idx];
}

void set_doubles_elt(VECP v, size_t idx, double val) {
    if (v.type != DOUBLES_VEC) {
        fprintf(stderr, "set_double: val is of type %s, expected DOUBLES_VEC\n",
                vectype_str(v.type));
        exit(1);
    }
    if (idx >= *v.n) {
        fprintf(stderr, "set_double: index out of bounds: %zu\n", idx);
        exit(1);
    }
    v.node->vec->doubles[idx] = val;
}

const char *as_string(VECP v, size_t idx) {
    if (idx >= *v.n) {
        fprintf(stderr, "as_string: index out of bounds: %zu\n", idx);
        exit(1);
    }
    if (v.type != STRINGS_VEC) {
        fprintf(stderr, "as_string: not implemented for non-STRINGS_VEC");
        exit(1);
    }
    return ((char **)v.node->vec->data)[idx];
}

void set_strings_elt(VECP v, size_t idx, const char *val) {
    if (v.type != STRINGS_VEC) {
        fprintf(stderr, "set_strings_elt: val is of type %s, expected STRINGS_VEC\n",
                vectype_str(v.type));
        exit(1);
    }
    if (idx >= *v.capacity) {
        fprintf(stderr, "set_strings_elt: index out of bounds: %zu\n", idx);
        exit(1);
    }
    v.node->vec->strings[idx] = strdup(val); mem++;
    v.node->vec->n++;
}


// atexit free all consumed memory
void free_memstack(void) {
    for (struct DLNode *n = memstack.tail; n != NULL; n = n->prev) {
        dllist_remove(&memstack, n);
    }
    printf("  Final memstack len: %zu\n", memstack.len);
    printf("  Memory leaks: %d\n", mem);
}

int main() {
    if (atexit(free_memstack)) {
        fprintf(stderr, "Failed to register 'free_memstack'\n");
        return 1;
    }

    VECP v1 = alloc_vector(1, INTS_VEC);
    set_ints_elt(v1, 0, 41);
    printf("v1: %d\n", as_int(v1, 0));

    VECP v2 = alloc_vector(10, DOUBLES_VEC);
    set_doubles_elt(v2, 0, 43.33);
    printf("v2: %f\n", as_double(v2, 0));

    VECP v3 = alloc_vector(1, STRINGS_VEC);
    set_strings_elt(v3, 0, "hello");
    printf("v3: %s\n", as_string(v3, 0));

    printf("initial len: %zu\n", memstack.len);
    for (struct DLNode *n = memstack.tail; n != NULL; n = n->prev) {
        switch (n->vec->type)
        {
        case INTS_VEC:
            printf("data: %d\n", n->vec->ints[0]);
            break;
        case DOUBLES_VEC:
            printf("data: %f\n", n->vec->doubles[0]);
            break;
        case STRINGS_VEC:
            printf("data: %s\n", n->vec->strings[0]);
            break;
        default:
            break;
        }
    }

    dllist_remove(&memstack, v2.node);
    printf("len: %zu\n", memstack.len);
    // for (struct DLNode *n = memstack.tail; n != NULL; n = n->prev) {
    //     printf("data: %d\n", n->data->ints[0]);
    // }

    return 0;
}





void *chk_realloc(void *ptr, size_t new_size) {
    void *p;
    if (new_size == 0) {
        free(ptr);
        return NULL;
    }
    if (ptr)
        p = realloc(ptr, new_size);
    if (!p) {
        fprintf(stderr, "chk_realloc: memory reallocation failed!\n");
        exit(1);
    }
    return p;
}



/*query_buff must be freed*/
void build_sql_query(char **query_buff, const char *format_string, ...) {
    va_list argptr;
    va_start(argptr, format_string);
    // determine length needed for query
    int req_len = snprintf(NULL, 0, format_string, argptr); 
    // allocate memory for query and write it
    *query_buff = chk_calloc(req_len + 1, sizeof(char));
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


/*
    VECTORS
*/


vectype_t chk_vectype(int t) {
    if (t != INTS_VEC && t != DOUBLES_VEC && t != STRINGS_VEC && t != NULL_VEC) {
        fprintf(stderr, "chk_vectype: invalid vectype: %d\n", t);
        exit(1);
    }
    return t;
}


typedef struct VECSTRUCT {
    vectype_t type;
    void *data;
    int *ints;
    double *doubles;
    size_t n;
} VECSTRUCT;


VECSTRUCT alloc_vector_old(size_t n, vectype_t type) {
    VECSTRUCT v;
    v.type = type;
    v.n = n;
    v.data = NULL;
    v.ints = NULL;
    v.doubles = NULL;
    switch (type) {
        case INTS_VEC:
            v.data = chk_calloc(n, sizeof(int));
            v.ints = (int*)v.data;
            break;
        case DOUBLES_VEC:
            v.data = chk_calloc(n, sizeof(double));
            v.doubles = (double*)v.data;
            break;
        case STRINGS_VEC:
            fprintf(stderr, "alloc_vector_old: strings not implemented, use alloc_strings\n");
            exit(1);
        case NULL_VEC:
            break;
        default:
            fprintf(stderr, "alloc_vector_old: unknown type: %d\n", type);
            exit(1);
    }
    return v;
}

void free_vector(VECSTRUCT *v) {
    free(v->data);
    v->data = NULL;
    v->ints = NULL;
    v->doubles = NULL;
    v->n = 0;
}


void set_vector_elt(VECSTRUCT *v, size_t idx, void *val) {
    if (idx >= v->n) {
        fprintf(stderr, "set_vector_elt: index out of bounds: %zu\n", idx);
        exit(1);
    }
    switch (v->type) {
    case INTS_VEC:
        v->ints[idx] = *(int*)val;
        break;
    case DOUBLES_VEC:
        v->doubles[idx] = *(double*)val;
        break;
    case STRINGS_VEC:
        fprintf(stderr, "set_vector_elt: strings not implemented, use set_strings_elt_old\n");
        exit(1);
    default:
        break;
    }
}


typedef struct STRSTRUCT {
    char **strings;
    size_t n;
    size_t capacity;
} STRSTRUCT;

STRSTRUCT alloc_strings(size_t capacity) {
    STRSTRUCT s;
    s.strings = chk_calloc(capacity, sizeof(char*));
    s.capacity = capacity;
    return s;
}

void set_strings_elt_old(STRSTRUCT *s, size_t i, const char *str) {
    if (i >= s->capacity) {
        fprintf(stderr, "set_strings_elt_old: index out of bounds: %zu\n", i);
        exit(1);
    }
    s->strings[i] = strdup(str);
    s->n++;
}

void free_strings(STRSTRUCT *s) {
    for (size_t i = 0; i < s->n; i++) {
        free(s->strings[i]);
    }
    free(s->strings);
    s->strings = NULL;
    s->capacity = 0;
    s->n = 0;
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
    free(query);
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
        ctypes[col_ix] = NULL_VEC;
        printf("BLOB - not implemented\n");
    } else {
        // TODO
        ctypes[col_ix] = NULL_VEC;
        printf("UNKNOWN - not implemented\n");
    }
    return 0;
}


void db_coltypes(sqlite3 *db, size_t ncols, VECSTRUCT *v) {
    const char *query = "PRAGMA table_info(birthwt)";
    exec_sqlite_query(db, query, col_type_callback, v->ints);
}


int colnames_callback(void *strings, int argc, char **data, char **columns) {
    char **cols = *(char ***)strings;
    for (int i = 0; i < argc; i++) {
        cols[i] = strdup(columns[i]);
    }
    return 0;
}

void db_colnames(sqlite3 *db, STRSTRUCT *s) {
    size_t ncols = db_ncols(db);
    if (s->capacity != ncols) {
        fprintf(stderr, "db_colnames: length mismatch\n");
        exit(1);
    }
    const char *query = "SELECT * FROM birthwt LIMIT 1";
    exec_sqlite_query(db, query, colnames_callback, &s->strings);
    s->n = ncols;
}


typedef struct sqlite_table {
    char *name;
    char *dbpath;
    size_t nrows;
    size_t ncols;
    STRSTRUCT colnames;
    VECSTRUCT coltypes;
    sqlite3 *db;
    int ini;
} sqlite_table;


sqlite_table currtab = {
    NULL,
    NULL,
    0,
    0,
    {NULL, 0, 0},
    {NULL_VEC, NULL, NULL, NULL, 0},
    NULL,
    0
};


void free_sqlite_table(void);

void init_sqlite_table(const char *dbpath,
                       const char *table) {
    
    if (currtab.ini) {
        fprintf(stderr, "Warning: init_sqlite_table: already initialized, freeing...\n");
        free_sqlite_table();
    }
    printf("Connecting to %s for table '%s'...\n\n", dbpath, table);
    int rc = sqlite3_open(dbpath, &currtab.db);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(currtab.db));
        exit(1);
    }
    currtab.nrows = db_nrows(currtab.db);
    currtab.ncols = db_ncols(currtab.db);
    currtab.colnames = alloc_strings(currtab.ncols);
    db_colnames(currtab.db, &currtab.colnames);
    currtab.coltypes = alloc_vector_old(currtab.ncols, INTS_VEC);
    db_coltypes(currtab.db, currtab.ncols, &currtab.coltypes);
    currtab.name = strdup(table);
    currtab.dbpath = strdup(dbpath);
    currtab.ini = 1;
}


void free_sqlite_table(void) {
    free_strings(&currtab.colnames);
    free_vector(&currtab.coltypes);
    free(currtab.name);
    free(currtab.dbpath);
    printf("\nClosing database connection...\n");
    if (currtab.db) {
        int rc = sqlite3_close(currtab.db);
        if (rc != SQLITE_OK) {
            // TODO
            fprintf(stderr, ":( Failed to close database: %s\n", sqlite3_errmsg(currtab.db));
        }
        currtab.db = NULL;
    } else{
        printf("Nothing to do :O\n");
    }
    currtab.ini = 0;
}



int main2() {
    /*
            SETUP
    */
    if (atexit(free_sqlite_table)) {
        fprintf(stderr, "Failed to register 'free_sqlite_table'\n");
        return 1;
    }
    init_sqlite_table("test.db", "birthwt");
    /*
            QUERY TABLE INFO
    */
    print_head(currtab.db, 10);
    printf("nrows: %zu\n", currtab.nrows);
    printf("ncols: %zu\n", currtab.ncols);
    /*
            READ DATA INTO VECTORS...
    */
    // vectype_t t;
    // for (size_t i = 0; i < ncols; i++) {
    //     t = chk_vectype(coltypes.ints[i]);
    // }
    for (size_t i = 0; i < currtab.ncols; i++) {
        printf("%s (%s) \n",
               currtab.colnames.strings[i],
               vectype_str(currtab.coltypes.ints[i]));
    }
    return 0;
}
