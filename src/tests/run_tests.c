#include <stdio.h>
#include <stdlib.h> // calloc
#include <string.h> // strcmp
#include <stdarg.h> // va_list, va_start, va_end
#include <math.h> // sqrt
#include "tests/run_tests.h"

// #include <signal.h>
// #include <assert.h>

#include "global.h"
#include "array.h"
#include "list.h"
#include "memory.h"
#include "rand/rng.h"


void print_array(const ARRP m) {
    size_t nrow = dims(m)[0];
    size_t ncol = dims(m)[1];
    for (size_t i = 0; i < nrow; ++i) {
        for (size_t j = 0; j < ncol; ++j) {
            printf("%f\t", real(m)[i * ncol + j]);
        }
        putchar('\n');
    }
}


int check_dbls_equal(double a, double b, const char *msg) {
    // return 1 if a and b are not equal
    int res = fabs(a - b) > 1e-6;
    printf("%s %s\n", res ? "  [XX]" : "  [:)]", msg);
    return res;
}

int check_arrp_equal(ARRP a, ARRP b, const char *msg) {
    // return 1 if a and b are not equal
    int fail;
    if (arrtype(a) != arrtype(b))
        fail = 1;
    else if (arrtype(a) == INTS_ARR)
        fail = !ints_eq(a, b);
    else if (arrtype(a) == REALS_ARR)
        fail = !reals_eq_tol(a, b, 1e-6);
    printf("%s %s\n", fail ? " [XX]" : " [:)]", msg);
    return fail; // flag if not equal
}



/*
*
* Test functions
*
*/



void _test_title(const char *msg) {
    int sl = strlen(msg);
    for (int i=0; i < sl; ++i) putchar('-');
    printf("\n%s\n", msg);
    for (int i=0; i < sl; ++i) putchar('-');
    putchar('\n');
}
void _test_summary(int failed) {
    if (failed > 0) {
        printf(" xxx   %d %s failed\n", failed, failed == 1 ? "test" : "tests");
    } else {
        printf(" ooo   All tests passed!\n");
    }
}

int test_scalar_ops() {
    _test_title("SCALAR OPS");
    int test = 0;
    ARRP x=empty(), y=empty(), z=empty();
    // add
    x = alloc_array(REALS_ARR, 1, 1); real(x)[0] = 3.0;
    set_add_num(x, 3.0);
        test += check_dbls_equal(reals_elt(x, 0, 0), 6.0, "set_add_num");
    y = add_num(x, 3.0);
        test += check_dbls_equal(reals_elt(y, 0, 0), 9.0, "add_num");
    free_array(&x); free_array(&y); free_array(&z);

    // subtract
    x = alloc_array(REALS_ARR, 1, 1); real(x)[0] = 3.0;
    set_add_num(x, -1.0);
        test += check_dbls_equal(reals_elt(x, 0, 0), 2.0, "substract set_add_num");
    y = add_num(x, -1.0);
        test += check_dbls_equal(reals_elt(y, 0, 0), 1.0, "subtract add_num");
    free_array(&x); free_array(&y); free_array(&z);

    // mul
    x = alloc_array(REALS_ARR, 1, 1); real(x)[0] = 3.0;
    set_mul_num(x, 3.0);
        test += check_dbls_equal(reals_elt(x, 0, 0), 9.0, "set_mul_num");
    y = mul_num(x, 3.0);
        test += check_dbls_equal(reals_elt(y, 0, 0), 27.0, "mul_num");
    free_array(&x); free_array(&y); free_array(&z);

    // div
    x = alloc_array(REALS_ARR, 1, 1); real(x)[0] = 9.0;
    set_div_num(x, 3.0);
    test += check_dbls_equal(reals_elt(x, 0, 0), 3.0, "set_div_num");
    y = divide(x, x);
    test += check_dbls_equal(reals_elt(y, 0, 0), 1.0, "divide");
    free_array(&x); free_array(&y); free_array(&z);

    _test_summary(test);
    return test;
}

int test_array_fillers() {
    _test_title("FILLERS");
    int test = 0;
    ARRP x=empty(), y=empty(), z=empty();

    // fill num
    x = alloc_array(INTS_ARR, 1, 3);
    set_fill_num(x, 1, 1); // 1, 2, 3
    y = alloc_array(INTS_ARR, 1, 3);
    integer(y)[0] = 1; integer(y)[1] = 2; integer(y)[2] = 3;
        test += check_arrp_equal(x, y, "set_fill_num ints");
    free_array(&x); free_array(&y); free_array(&z);

    // fill num
    x = alloc_array(REALS_ARR, 1, 3);
    set_fill_num(x, 0.33, 0.33); // 1, 2, 3
    y = alloc_array(REALS_ARR, 1, 3);
    real(y)[0] = 0.33; real(y)[1] = 0.33*2; real(y)[2] = 0.33*3;
        test += check_arrp_equal(x, y, "set_fill_num reals");
    free_array(&x); free_array(&y); free_array(&z);

    _test_summary(test);
    return test;

}


int test_rand() {
    _test_title("RAND");
    int test = 0;
    ARRP x1, x2;
    // fill x1 manually with random numbers from the below seed
    x1 = alloc_array(REALS_ARR, 1, 10);
    MTRand r = seedRand(1234);
    for (int i=0; i < length(x1); ++i)
        real(x1)[i] = genRand(&r);
    
    // fill x2 using the same seed
    x2 = alloc_array(REALS_ARR, 1, 10);
    set_rand_unif(x2, 1234);

    test += check_arrp_equal(x1, x2, "set_rand_unif");
    free_array(&x1); free_array(&x2);

    _test_summary(test);
    return test;
}

int test_elt_functions() {
    _test_title("ELEMENT-WISE FUNCTIONS");
    int test = 0;
    ARRP x=empty(), y=empty(), z=empty();

    // pow2
    x = alloc_array(REALS_ARR, 1, 3); set_fill_num(x, 1, 1); // 1, 2, 3
    set_arrp_pow2(x);
    y = alloc_array(REALS_ARR, 1, 3);
    real(y)[0] = 1; real(y)[1] = 4; real(y)[2] = 9;
        test += check_arrp_equal(x, y, "arrp_pow2");
    free_array(&x); free_array(&y); free_array(&z);

    // sqrt
    x = alloc_array(REALS_ARR, 1, 3); set_fill_num(x, 1, 1); // 1, 2, 3
    set_arrp_sqrt(x);
    y = alloc_array(REALS_ARR, 1, 3);
    real(y)[0] = 1; real(y)[1] = sqrt(2); real(y)[2] = sqrt(3);
        test += check_arrp_equal(x, y, "arrp_sqrt");
    free_array(&x); free_array(&y); free_array(&z);

    _test_summary(test);
    return test;

}

int test_array_ops() {
    _test_title("ARRAY OPS");
    int test = 0;
    ARRP x=empty(), y=empty(), z=empty();
    // add
    x = alloc_array(REALS_ARR, 1, 1); real(x)[0] = 3.0;
    set_add(x, x);
        test += check_dbls_equal(reals_elt(x, 0, 0), 6.0, "set_add");
    y = add(x, x);
        test += check_dbls_equal(reals_elt(y, 0, 0), 12.0, "add");
    free_array(&x); free_array(&y); free_array(&z);

    // subtract
    x = alloc_array(REALS_ARR, 1, 1); real(x)[0] = 3.0;
    z = alloc_array(REALS_ARR, 1, 1); real(z)[0] = 1.0;
    set_subtract(x, z);
        test += check_dbls_equal(reals_elt(x, 0, 0), 2.0, "set_subtract");
    y = subtract(x, z);
        test += check_dbls_equal(reals_elt(y, 0, 0), 1.0, "subtract");
    free_array(&x); free_array(&y); free_array(&z);

    // mul
    x = alloc_array(REALS_ARR, 1, 1); real(x)[0] = 3.0;
    set_mul(x, x);
        test += check_dbls_equal(reals_elt(x, 0, 0), 9.0, "set_mul");
    y = mul(x, x);
        test += check_dbls_equal(reals_elt(y, 0, 0), 81.0, "mul");
    free_array(&x); free_array(&y); free_array(&z);

    // div
    x = alloc_array(REALS_ARR, 1, 1); real(x)[0] = 9.0;
    z = alloc_array(REALS_ARR, 1, 1); real(z)[0] = 3.0;
    set_divide(x, z);
        test += check_dbls_equal(reals_elt(x, 0, 0), 3.0, "set_divide");
    y = divide(x, z);
        test += check_dbls_equal(reals_elt(y, 0, 0), 1.0, "divide");
    free_array(&x); free_array(&y); free_array(&z);

    _test_summary(test);
    return 0;
}


int test_matmul() {
    _test_title("MATMUL");
    int test = 0;
    ARRP x=empty(), y=empty(), z=empty(), z_tru=empty();

    // matmul
    x = alloc_array(REALS_ARR, 2, 2); set_fill_num(x, 1, 1); // 1, 2, 3, 4
    z = matmul(x, x);
    z_tru = alloc_array(REALS_ARR, 2, 2);
        real(z_tru)[0] =  7; real(z_tru)[1] = 10;
        real(z_tru)[2] = 15; real(z_tru)[3] = 22; 
    test += check_arrp_equal(z, z_tru, "matmul");
    free_array(&z); free_array(&z_tru);

    y = alloc_array(REALS_ARR, 2, 3); set_fill_num(y, 2, 2); // 2,4,6,8,10,12
    set_matmul(x, y);
    z_tru = alloc_array(REALS_ARR, 2, 3);
        real(z_tru)[0] = 18; real(z_tru)[1] = 24; real(z_tru)[2] = 30;
        real(z_tru)[3] = 38; real(z_tru)[4] = 52; real(z_tru)[5] = 66;

    print_array(x);
    
    test += check_arrp_equal(x, z_tru, "set_matmul");
    free_array(&x); free_array(&y); free_array(&z_tru);

    _test_summary(test);
    return test;

}


int test_transpose() {
    _test_title("TRANSPOSE");
    int test = 0;

    ARRP x, y;
    x = alloc_array(REALS_ARR, 2, 3);
    x = set_fill_num(x, 1, 1); // 1, 2, 3, 4, 5, 6
    y = transpose(x);

    test += (dims(x)[0] != dims(y)[1]) ||
            (dims(x)[1] != dims(y)[0]);

    set_transpose(y);
    test += check_arrp_equal(x, y, "transpose");
    
    _test_summary(test);
    return test;
}

int test_crossprod() {
    _test_title("CROSSPROD");
    int test = 0;

    ARRP x, y, z;
    x = alloc_array(REALS_ARR, 3, 1); // 3 x 1
        set_fill_num(x, 1, 1); // 1, 2, 3
    y = crossprod(x, x); // 1 x 3 %*% 3 x 1  = 1 x 1
    z = tcrossprod(x, x); // 3 x 1 %*% 1 x 3 = 3 x 3

    test += (dims(y)[0] != 1) || (dims(y)[1] != 1);
    test += check_dbls_equal(reals_elt(y, 0, 0), 14.0, "crossprod");
    test += (dims(z)[0] != 3) || (dims(z)[1] != 3);
    test += check_dbls_equal(reals_elt(z, 2, 2), 9.0, "tcrossprod");

    _test_summary(test);
    return test;
}




int run_tests(void) {
    init_memstack();

    int failed = 0;
    failed += test_scalar_ops();
    failed += test_array_fillers();
    failed += test_rand();
    failed += test_elt_functions();
    failed += test_array_ops();
    failed += test_matmul();
    failed += test_transpose();
    failed += test_crossprod();

    printf("\n%s %d %s failed\n",
            failed == 0 ? "   " : "!!!",
            failed,
            failed == 1 ? "test" : "tests");

    return (failed > 0) ? 1 : 0;
}

