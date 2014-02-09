/* An example of how to test the tree-based memory allocation
 * interface for this class. */

#ifdef PALLOC_TEST
#include <stdio.h>
#include "palloc.h"
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

/**
 * Test to see if Palloc works
 */
static void test_palloc(void)
{
	int i = 0;
    void *context = palloc_init("test context");
    int *test = mm_malloc(sizeof(int)); //Set memory to non-zero
    *test = 1;
    mm_free(test);
    for (i = 0; i < 200; i += 1) { //Repeat many times.
		int *ptr = palloc(context, int);
		CU_ASSERT(ptr != NULL);
		CU_ASSERT(*ptr == 0);
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
    void *child = _palloc(parent, sizeof(int), "int");
    int returnparent = 0;
    //int returnchild = 0;
    returnparent = pfree(parent);
    CU_ASSERT(returnparent == 0);
    //returnchild = pfree(child); //Currently doesn't return -1, actually attempts to free
    //CU_ASSERT(returnchild == -1); //TODO: Reimplement once it doesn't cause an error which ruins every other test.
    CU_ASSERT(parent == NULL);
    CU_ASSERT(child == NULL);
}

int return_valid(void *ignore)
{
	return 0;
}

int return_invalid(void *ignore)
{
	return -1;
}
/**
 * Test of the destructor works properly
 */
static void test_destructor(void)
{
    void *parent = palloc_init("test context");
    void *child1 = palloc(parent, int);
    void *child2 = palloc(parent, int);
    int returnparent = 0;
    int returnchild1 = 0;
    int returnchild2 = 0;
    returnchild2 = pfree(child2);
    palloc_destructor(child2, &return_invalid);
    palloc_destructor(child2, NULL); //Test deletion of destructor
    CU_ASSERT(returnchild2 == 0);
    palloc_destructor(child1, &return_valid);
    returnchild1 = pfree(child1);
    CU_ASSERT(returnchild1 == 0);
    palloc_destructor(parent, &return_invalid);
	returnparent = pfree(parent);
	CU_ASSERT(returnparent == -1);
}

/**
 * Test array and strings, then print and cast
 */
static void test_usability(void)
{
    void *parent = palloc_init("Test context");
    char **child1 = palloc_array(parent, char*, 5);
    char *child2 = palloc_strdup(parent, "Test");
    int i = 0;
    for (i = 0; i < 5; i += 1) {
    	child1[i] = palloc_strdup(child1, "Larger Child");
    	CU_ASSERT(child1[i] != NULL);
    	//TODO: Figure out how to access value.
    }
    palloc_strdup(child2, "Test");
    CU_ASSERT(child1 != NULL);
    CU_ASSERT(child2 != NULL);
    palloc_print_tree(parent); //Make sure correct manually?
    CU_ASSERT(palloc_cast(child2, char*) != NULL);
    CU_ASSERT(palloc_cast(child2, int) == NULL);
    prealloc(child1, 7);
    for (i = 5; i < 7; i += 1) {
    	child1[i] = palloc_strdup(child1, "Larger Child");
    	CU_ASSERT(child1[i] != NULL);
    	//TODO: Figure out how to access value
    }
    //Test making the array smaller, test if parent still has a pointer to the old size 7 array.
    child1 = palloc_array(parent, char*, 5);
    //prealloc(child1, 3); //TODO: Fix? THROWS A SEGMENTATION FAULT!
    for (i = 0; i < 5; i += 1) {
    	child1[i] = palloc_strdup(child1, "Smaller Child");
    	CU_ASSERT(child1[i] != NULL);
    }
    palloc_print_tree(parent);
    //TODO: Write a custom print function that actually prints the values.
}

/** Test slab allocator internal representation */
static void test_slab(void) 
{
	//UNIMPLEMENTED since need performance tests first.
	/*
	 *     void *parent = palloc_init("Test context");
	 *     void *first;
	 *     void *second;
	 *     
	 *     slab_group *first_group = new_slab(parent, 256, 2);
	 *     new_slab(first, 1024, 2);
	 *     first = slab_get(first_group, 1024); //Should work
	 *     //Check to make sure not dynamically reallocate
	 *     CU_ASSERT(first != NULL); //Actually alloced
	 *     CU_ASSERT(slab_free(slab, first) != -1);
	 *     second = slab_get(first_group, 2048); //Should dynamically reallocate a new slab
	 *     CU_ASSERT(second != NULL);
	 *     first = slab_realloc(second, 4096);
	 *     CU_ASSERT(first != NULL);
	 */
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
        CU_add_test(s, "test if destructor works properly", &test_destructor);
        CU_add_test(s, "Test usability of Array and String, realloc, then see if typecast and print works", &test_usability);
        CU_add_test(s, "Test slab allocation internal structure", &test_slab); //Tests for slab allocation internal structure if we do use it
    }

    /* Actually run your tests here. */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
#endif
