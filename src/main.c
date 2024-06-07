#include <stdio.h>
#include <stdlib.h> // calloc
#include <string.h> // strcmp
#include <stdarg.h> // va_list, va_start, va_end
#include <math.h> // sqrt
#include "sqlite/sqlite3.h"


#include <signal.h>
#include <assert.h>

#include "global.h"
#include "array.h"
#include "list.h"
#include "memory.h"


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

void test_title(const char *msg) {
    int sl = strlen(msg);
    for (int i=0; i < sl; ++i) putchar('-');
    printf("\n%s\n", msg);
    for (int i=0; i < sl; ++i) putchar('-');
    putchar('\n');
}
void test_summary(int failed) {
    if (failed > 0) {
        printf(" xxx   %d %s failed\n", failed, failed == 1 ? "test" : "tests");
    } else {
        printf(" ooo   All tests passed!\n");
    }
}

int test_scalar_ops() {
    test_title("SCALAR OPS");
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

    test_summary(test);
    return test;
}

int test_array_fillers() {
    test_title("FILLERS");
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

    test_summary(test);
    return test;

}

int test_elt_functions() {
    test_title("ELEMENT-WISE FUNCTIONS");
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

    test_summary(test);
    return test;

}

int test_array_ops() {
    test_title("ARRAY OPS");
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

    test_summary(test);
    return 0;
}


int main() {
    init_memstack();

    int failed = 0;
    failed += test_scalar_ops();
    failed += test_array_fillers();
    failed += test_elt_functions();
    failed += test_array_ops();

    printf("%s %d %s failed\n",
            failed == 0 ? "" : "!!!",
            failed,
            failed == 1 ? "test" : "tests");
    return 0;
}


