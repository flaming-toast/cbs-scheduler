/* An example of how to test the tree-based memory allocation
 * interface for this class. */

#ifdef PALLOC_TEST
#include <stdio.h>
#include "palloc.h"
#include "CUnit/Basic.h"

static int init_suite_example(void)
{
    return 0;
}

static int clean_suite_example(void)
{
    return 0;
}

static void test_palloc(void)
{
    void *context = palloc_init("test context");
    int *ptr = palloc(context, int);
    CU_ASSERT(ptr != NULL);

    pfree(ptr);
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

        CU_add_test(s, "test of palloc()", &test_palloc);
    }

    /* Actually run your tests here. */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
#endif
