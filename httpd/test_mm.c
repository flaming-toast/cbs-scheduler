/* An example of how to test the memory allocation interface for this
 * class. */

#ifdef MM_TEST
#include <stdio.h>
#include "mm_alloc.h"
#include "CUnit/Basic.h"

static int init_suite_example(void)
{
    return 0;
}

static int clean_suite_example(void)
{
    return 0;
}

static void test_malloc(void)
{
    int *ptr = mm_malloc(sizeof(*ptr));
    CU_ASSERT(ptr != NULL);
    mm_free(ptr);
}


int main(int argc, char **argv)
{

    /* Initialize CUnit. */
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    /* Here's an example test suite.  First we initialize the suit. */
    {
        CU_pSuite s = CU_add_suite("example test suite",
                                   &init_suite_example,
                                   &clean_suite_example);

        CU_add_test(s, "test of mm_malloc()", &test_malloc);
    }

    /* Actually run your tests here. */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
#endif
