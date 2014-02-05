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

/**
 * Test to see if Palloc works
 */
static void test_palloc(void)
{
	int i = 0;
    void *context = palloc_init("test context");
    int *test = mm_alloc(sizeof(int)); //Set memory to non-zero
    *test = 1;
    mm_free(test);
    for (i = 0; i < 200; i += 1) { //Repeat many times.
		int *ptr = palloc(context, int);
		CU_ASSERT(ptr != NULL);
		CU_ASSERT(ptr == 0);
		*ptr = 1;
		pfree(ptr);
    }
}

/**
 * Test to see if freeing parent frees child, tests if deletion works as well.
 */
static void test_parent_child(void)
{
    void *parent = palloc_init("test context");
    void *child = palloc(parent, sizeof(int), "int");
    int returnparent = 0;
    int returnchild = 0;
    returnparent = pfree(parent);
    CU_ASSERT(returnparent == 0);
    returnchild = pfree(child);
    CU_ASSERT(returnchild == -1);
    CU_ASSERT(!parent);
    CU_ASSERT(!child);
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

        CU_add_test(s, "test of palloc(), if memory properly zero'd", &test_palloc);
        CU_add_test(s, "test palloc free child/parent(), if deletion works, deletion twice error", &test_parent_child);
        //Test if destructor works
        //Test if realloc works
        //Test for slab allocation
        //Test for palloc cast
        //Test for palloc printtree
    }

    /* Actually run your tests here. */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
#endif
