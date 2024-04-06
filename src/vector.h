#ifndef _VECTOR_H
#define _VECTOR_H


#include <stdlib.h> // size_t

typedef enum {
    INTS_VEC = 0,
    DOUBLES_VEC,
    STRINGS_VEC,
    NULL_VEC
} vectype_t;


const char *vectype_str(vectype_t t);


typedef struct VectorStruct {
    vectype_t type;         // type of data contained by vector
    void *data;             // pointer to the memory allocated for the vector
    int *ints;              // pointer to data, if type is INTS_VEC
    double *doubles;        // pointer to data, if type is DOUBLES_VEC
    char **strings;         // pointer to data, if type is STRINGS_VEC
    size_t capacity;        // vector capacity / length
    size_t n;               // number of allocated elements (differs from capacity only for STRINGS_VEC)
    size_t dims[2];         // dimensions - used for matrices - TODO: implement?
} VectorStruct;

void alloc_vector_struct(VectorStruct *vs, size_t nelem, vectype_t type);
void free_vector_struct(VectorStruct *vs);


typedef struct VECP {
    struct DLNode *node;
    size_t nrows;
    size_t ncols;
} VECP;

VECP alloc_vector(size_t n, vectype_t type);
VECP resize_vector(VECP *v, size_t newsize);
void free_vector(VECP *v);

int *INTEGER(VECP v);
void set_ints_elt(VECP v, size_t idx, int val);
int as_int(VECP v, size_t idx);
//
double *DOUBLE(VECP v);
double as_double(VECP v, size_t idx);
void set_doubles_elt(VECP v, size_t idx, double val);
void CAST_DOUBLE(VECP v);
//
// TODO: why not just STRING?
// because then you just get the pointer to the first string in arr
const char *STRING_ELT(VECP v, size_t idx);
const char *as_string(VECP v, size_t idx);
void set_strings_elt(VECP v, size_t idx, const char *val);


size_t LENGTH(VECP v);
size_t CAPACITY(VECP v);
vectype_t TYPEOF(VECP v);

#endif // _VECTOR_H