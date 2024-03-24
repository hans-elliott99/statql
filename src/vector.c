#include "vector.h"
#include "memory.h"
#include "list.h"

#include <stdio.h> 
#include <string.h>

#include "global.h"

/*
    vectype_t - Vector Type
*/

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

/*
    VectorStruct
*/

void alloc_vector_struct(VectorStruct *vs, size_t nelem, vectype_t type) {
    vs->type = type;
    vs->capacity = nelem;
    vs->n = (type == STRINGS_VEC) ? 0 : nelem; // inidiv strings need allocation
    vs->data = NULL;
    vs->ints = NULL;
    vs->doubles = NULL;
    vs->strings = NULL;
    switch (type) {
        case INTS_VEC:
            vs->data = chk_calloc(nelem, sizeof(int));
            vs->ints = (int*)vs->data;
            break;
        case DOUBLES_VEC:
            vs->data = chk_calloc(nelem, sizeof(double));
            vs->doubles = (double*)vs->data;
            break;
        case STRINGS_VEC:
            vs->data = chk_malloc(nelem * sizeof(char*));
            vs->strings = (char**)vs->data;
        case NULL_VEC:
            break;
        default:
            fprintf(stderr, "alloc_vector_struct: unknown type: %d\n", type);
            exit(1);
    }
}

void free_vector_struct(VectorStruct *vs) {
    if (vs->type == STRINGS_VEC) {
        for (size_t i = 0; i < vs->n; i++) {
            chk_free(vs->strings[i]);
        }
    }
    chk_free(vs->data);
    vs->data = NULL;
    vs->ints = NULL;
    vs->doubles = NULL;
    vs->strings = NULL;
    vs->n = 0;
    vs->capacity = 0;
}


/*
    VECP - vector pointer
*/
VECP alloc_vector(size_t n, vectype_t type) {
    VECP v;
    v.node = chk_malloc(sizeof(struct DLNode));
    v.node->vec = chk_malloc(sizeof(VectorStruct));
    alloc_vector_struct(v.node->vec, n, type);
    dllist_append(&memstack, v.node);
    return v;
}

void free_vector(VECP v) {
    dllist_remove(&memstack, v.node);
}

size_t LENGTH(VECP v) {
    return v.node->vec->n;
}

size_t CAPACITY(VECP v) {
    return v.node->vec->capacity;
}


vectype_t TYPEOF(VECP v) {
    return v.node->vec->type;
}



/*INTEGER*/
int *INTEGER(VECP v) {
    if (TYPEOF(v) != INTS_VEC) {
        fprintf(stderr, "INTEGER: vector is of type %s, expected INTS_VEC\n",
                vectype_str(TYPEOF(v)));
        exit(1);
    }
    return v.node->vec->ints;
}

int as_int(VECP v, size_t idx) {
    if (idx >= LENGTH(v)) {
        fprintf(stderr, "as_int: index out of bounds: %zu\n", idx);
        exit(1);
    }
    if (TYPEOF(v) == STRINGS_VEC) {
        fprintf(stderr, "as_int: not implemented for STRINGS_VEC");
        exit(1);
    }
    // if double, casted to integer
    return ((int *)v.node->vec->data)[idx];
}

void set_ints_elt(VECP v, size_t idx, int val) {
    if (TYPEOF(v) != INTS_VEC) {
        fprintf(stderr, "set_int: val is of type %s, expected INTS_VEC\n",
                vectype_str(TYPEOF(v)));
        exit(1);
    }
    if (idx >= LENGTH(v)) {
        fprintf(stderr, "set_int: index out of bounds: %zu\n", idx);
        exit(1);
    }
    v.node->vec->ints[idx] = val;
}

/*DOUBLE*/
double *DOUBLE(VECP v) {
    if (TYPEOF(v) != DOUBLES_VEC) {
        fprintf(stderr, "DOUBLE: vector is of type %s, expected DOUBLES_VEC\n",
                vectype_str(TYPEOF(v)));
        exit(1);
    }
    return v.node->vec->doubles;
}

double as_double(VECP v, size_t idx) {
    if (idx >= LENGTH(v)) {
        fprintf(stderr, "as_double: index out of bounds: %zu\n", idx);
        exit(1);
    }
    if (TYPEOF(v) == STRINGS_VEC) {
        fprintf(stderr, "as_double: not implemented for STRINGS_VEC");
        exit(1);
    }
    // if int, casted to double
    return ((double *)v.node->vec->data)[idx];
}

void set_doubles_elt(VECP v, size_t idx, double val) {
    if (TYPEOF(v) != DOUBLES_VEC) {
        fprintf(stderr, "set_doubles_elt: val is of type %s, expected DOUBLES_VEC\n",
                vectype_str(TYPEOF(v)));
        exit(1);
    }
    if (idx >= LENGTH(v)) {
        fprintf(stderr, "set_doubles_elt: index out of bounds: %zu\n", idx);
        exit(1);
    }
    v.node->vec->doubles[idx] = val;
}

/*STRING*/
const char *STRING_ELT(VECP v, size_t idx) {
    if (TYPEOF(v) != STRINGS_VEC) {
        fprintf(stderr, "STRING_ELT: vector is of type %s, expected STRINGS_VEC\n",
                vectype_str(TYPEOF(v)));
        exit(1);
    }
    return v.node->vec->strings[idx];
}

const char *as_string(VECP v, size_t idx) {
    if (idx >= LENGTH(v)) {
        fprintf(stderr, "as_string: index out of bounds: %zu\n", idx);
        exit(1);
    }
    if (TYPEOF(v) != STRINGS_VEC) {
        fprintf(stderr, "as_string: not implemented for non-STRINGS_VEC");
        exit(1);
    }
    return ((char **)v.node->vec->data)[idx];
}

void set_strings_elt(VECP v, size_t idx, const char *val) {
    if (TYPEOF(v) != STRINGS_VEC) {
        fprintf(stderr, "set_strings_elt: val is of type %s, expected STRINGS_VEC\n",
                vectype_str(TYPEOF(v)));
        exit(1);
    }
    if (idx >= CAPACITY(v)) {
        fprintf(stderr, "set_strings_elt: index out of bounds: %zu\n", idx);
        exit(1);
    }
    chk_strcpy(&v.node->vec->strings[idx], val);
    v.node->vec->n++;
}


void CAST_DOUBLE(VECP v) {
    if (TYPEOF(v) == DOUBLES_VEC) {
        return;
    } else if (TYPEOF(v) == STRINGS_VEC) {
        fprintf(stderr, "CAST_DOUBLE: not implemented for STRINGS_VEC");
        exit(1);
    }
    v.node->vec->type = DOUBLES_VEC;
    v.node->vec->doubles = chk_malloc(v.node->vec->capacity * sizeof(double));
    for (size_t i = 0; i < v.node->vec->n; i++) {
        v.node->vec->doubles[i] = (double)v.node->vec->ints[i];
    }
    chk_free(v.node->vec->ints);
    v.node->vec->ints = NULL;
    v.node->vec->data = v.node->vec->doubles;
}
