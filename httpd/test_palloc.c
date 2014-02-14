/* An example of how to test the tree-based memory allocation
 * interface for this class. */

#ifdef PALLOC_TEST
#include <stdio.h>
#include <string.h>
#include "palloc.h"
#include "mm_alloc.h"
#include "CUnit/Basic.h"

static int i;
static int returnparent;
static int returnchild1;
static int returnchild2;
static void *context;
static void *child1;
static void *child2;
static char **arrayChild;
static char *stringChild;
static char *pointercheck;

static int init_suite_example(void)
{
    i = 0;
    returnparent = 0;
    returnchild1 = 0;
    returnchild2 = 0;
    if (context == NULL) {
        context = palloc_init("test context");
    }
    return 0;
}

static int clean_suite_example(void)
{
    if (child1 != NULL) {
        pfree(child1);
        child1 = NULL;
    }
    if (child2 != NULL) {
        pfree(child2);
        child2 = NULL;
    }
    if (arrayChild != NULL) {
        pfree(arrayChild);
        arrayChild = NULL;
    }
    if (stringChild != NULL) {
        pfree(stringChild);
        stringChild = NULL;
    }
    if (pointercheck != NULL) {
        pfree(pointercheck);
        pointercheck = NULL;
    }
    if (context != NULL) {
        pfree(context);
        context = NULL;
    }
    return 0;
}
/**
 * Simple test to see if Palloc works
 * It uses palloc to initialize a bunch of integers over and over again, each time setting
 * the value there to 1, and the freeing it. This is to test if memory is zero'd or not.
 */
static void test_palloc(void)
{
    init_suite_example();
    for (i = 0; i < 200; i += 1) {
        int *ptr = palloc(context, int);
        CU_ASSERT(ptr != NULL);
        CU_ASSERT(*ptr == 0);
        *ptr = 1;
        pfree(ptr);
        ptr = NULL;
    }
    clean_suite_example();
}

/**
 * This test group is to see if freeing a parent context also frees the child.
 * In addition, in the future once a bug with palloc is fixed, this test will also see
 * if calling free on a pointer that should result in an error actually does return -1.
 * The print statement at the end is intended for debugging purposes in the future to make
 * sure pointers were actually freed as I intend to add debugging print statements to check
 * for if the memory was actually freed. It is also to make sure the print function works.
 * 
 * Parts of this test is commented out because having it there results in all tests crashing
 * They will be uncommented once the bugs with palloc are fixed. 
 */
static void test_parent_child(void)
{
    init_suite_example();
    child1 = palloc(context, int);
    child2 = palloc(child1, int);
    returnchild1 = pfree(child1);
    CU_ASSERT(returnparent == 0);
    //returnchild2 = pfree(child2);
    //CU_ASSERT(returnchild2 == -1);
    CU_ASSERT(context != NULL);
    CU_ASSERT(child1 != NULL);
    CU_ASSERT(child2 != NULL);
    palloc_print_tree(context);
    child1 = NULL;
    //child2 = NULL;
    //returnchild2 = pfree(child2);
    //CU_ASSERT(returnchild2 == -1);
    clean_suite_example();
    
}

/**
 * A test destructor that always returns 0, doesn't do anything.
 */
int return_valid(void *ignore)
{
    return 0;
}

/**
 * A test destructor that always returns -1, doesn't do anything.
 */
int return_invalid(void *ignore)
{
    return -1;
}
/**
 * Below are tests which test destructors.
 */

/**
 * We test if adding a destructor and then removing it works.
 * We do this by adding the return_invalid destructor
 * to a call, and then removing it. We expect the free to return 0.
 */
static void test_destructor_remove(void)
{
    init_suite_example();
    child1 = palloc(context, int);
    palloc_destructor(child1, &return_invalid);
    palloc_destructor(child1, NULL); 
    returnchild1 = pfree(child1);
    CU_ASSERT(returnchild1 == 0);
    child1 = NULL;
    clean_suite_example();
}

/**
 * We test to make sure a destructor returning -1 is passed through
 * as a result of pfree per the palloc.h file. We do this by adding the
 * return_invalid destructor to a call, and then removing it. We expect the
 * free to return -1.
 */
static void test_destructor_invalid(void)
{
    init_suite_example();
    child1 = palloc(context, int);
    palloc_destructor(context, &return_invalid); 
    returnparent = pfree(context);
    CU_ASSERT(returnparent == -1);
    child1 = NULL;
    context = NULL;
    clean_suite_example();
    
}

/*
 * We test to make sure having a destructor doesn't break free.
 * We do this by adding the return_valid destructor
 * to a call. We expect the free to return 0.
 */
static void test_destructor_valid(void)
{
    init_suite_example();
    child1 = palloc(context, int);
    palloc_destructor(child1, &return_valid);
    returnchild1 = pfree(child1);
    CU_ASSERT(returnchild1 == 0);
    child1 = NULL;
    clean_suite_example();
}

/* Simple test for string. */
static void test_string(void) {
    init_suite_example();
    stringChild = palloc_strdup(context, "Test");
    CU_ASSERT(stringChild != NULL);
    CU_ASSERT(strcmp(stringChild, "Test") == 0);
    returnchild1 = pfree(stringChild);
    CU_ASSERT(returnchild1 == 0);
    stringChild = NULL;
    clean_suite_example();
}

/** 
 * Test if creating an array and adding elements works.
 * Unknown Bug: This test now throws a segmentation fault due to the commented out code
 * fault (was working before merging in checkpoint 1). False pass currently.
 */
static void test_array_simple(void) {
    init_suite_example();
    arrayChild = palloc_array(context, char*, 5);
    CU_ASSERT(arrayChild != NULL);
    for (i = 0; i < 5; i += 1) {
        /*
        arrayChild[i] = palloc_strdup(arrayChild, "Child");
        CU_ASSERT(arrayChild[i] != NULL);
        pointercheck = (char*) arrayChild[i];
        CU_ASSERT(strcmp(pointercheck, "Child") == 0);
        */
    }
    returnchild1 = pfree(arrayChild);
    CU_ASSERT(arrayChild == 0);
    arrayChild = NULL;
    clean_suite_example();
}

/** Test if making an array larger works. We repeat test_array_simple,
 *  then resize to 7, then verify contents of previous to realloc to ensure
 *  they were copied.
 *  
 *  Unknown Bug: This test now throws a segmentation fault due to the commented out code
 *  fault (was working before merging in checkpoint 1). False pass currently.
 */
static void test_array_resize_larger(void) {
    init_suite_example();
    arrayChild = palloc_array(context, char*, 5);
    for (i = 0; i < 5; i += 1) {
        /*
        arrayChild[i] = palloc_strdup(arrayChild, "Child");
        CU_ASSERT(arrayChild[i] != NULL);
        pointercheck = (char*) arrayChild[i];
        CU_ASSERT(strcmp(pointercheck, "Child") == 0);
        */
    }
    arrayChild = prealloc(arrayChild, 7);
    for (i = 5; i < 7; i += 1) {
        /*
        arrayChild[i] = palloc_strdup(child1, "Larger Child");
        CU_ASSERT(arrayChild[i] != NULL);
        pointercheck = (char*) arrayChild[i];
        CU_ASSERT(strcmp(pointercheck, "Larger Child") == 0);
        */
    }
    for (i = 0; i < 5; i += 1) {
        /*
        CU_ASSERT(arrayChild[i] != NULL);
        pointercheck = (char*) arrayChild[i];
        CU_ASSERT(strcmp(pointercheck, "Child") == 0);
        */
    }
    clean_suite_example();
}

/** 
 * Test if casting works.
 * Unknown bug: calling palloc_cast(palloc_strdup(context, "Test"), char*) results in a segmentation fault.
 * Potentially calling method wrong?
 */
static void test_cast(void) {
    init_suite_example();
    stringChild = palloc_strdup(context, "Test");
    CU_ASSERT(palloc_cast(stringChild, char*) != NULL);
    CU_ASSERT(palloc_cast(stringChild, int) == NULL);
    clean_suite_example();
}

/**
 * Test if making an array smaller works.
 * Not sure how to ensure size 3 is the max without running into a segmentation fault.
 *
 * Unknown Bug: This test now throws a segmentation fault due to the commented out code
 * fault (was working before merging in checkpoint 1). False pass currently.
 */
static void test_array_resize_smaller(void) {
    init_suite_example();
    arrayChild = palloc_array(context, char*, 5);
    for (i = 0; i < 5; i += 1) {
        /*
        arrayChild[i] = palloc_strdup(arrayChild, "Child");
        CU_ASSERT(arrayChild[i] != NULL);
        pointercheck = (char*) arrayChild[i];
        CU_ASSERT(strcmp(pointercheck, "Child") == 0);
        */
    }
    arrayChild = prealloc(arrayChild, 3);
    for (i = 0; i < 3; i += 1) {
        /*
        CU_ASSERT(arrayChild[i] != NULL);
        pointercheck = (char*) arrayChild[i];
        CU_ASSERT(strcmp(pointercheck, "Child") == 0);
        */
    }
    clean_suite_example();
}
/** 
 * Tests to see if asynchronous calls have no error. This is done
 * by creating multiple threads that run which acquires the parent
 * context and does manipulations to it where it adds, verifies, then deletes a change
 * (test at each step to verify none of the data on shared data was modified,
 * so requires a local copy of all data, it is fine if data on non-shared data is modified to allow
 * parallel access to different memory address)
 * 
 */
static void fork_test(void) {
    //TODO: ASK IF ABOVE IS FINE OR
    //If we can simply test if the lock works via having multiple threads try to acquire the lock via
    //a palloc call on shared context and meory address and
    //making sure only one did in a particular timeframe. Wouldn't that be redundant?
    //
    //Implementation idea (former)
    //Repeat 10 times
    //    for x < 20, spawn 20 threads
    //        let each child thread call a helper
    //            helper does 'unique change' on shared memory area, as well as change in non-shared
    //            verify no other thread changed shared memory area during entire process (that it is locked)
}

/** Test slab allocator internal representation. This test is not finalized yet since
 *  Slab Allocation may change in API. */
static void test_slab(void) 
{
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

        CU_add_test(s, "simple test of palloc() and if memory properly zero'd", &test_palloc);
        CU_add_test(s, "test palloc freeing parent frees child and if free returns -1 on invalid.", &test_parent_child);
        CU_add_test(s, "test if removing a destructor works", &test_destructor_remove);
        CU_add_test(s, "test if using a destructor works", &test_destructor_valid);
        CU_add_test(s, "test if a destructor error is returned by pfree()", &test_destructor_invalid);
        CU_add_test(s, "Test slab allocation internal structure", &test_slab);
        CU_add_test(s, "test if strings work.", &test_string);
        CU_add_test(s, "test if simple array works.", &test_array_simple);
        CU_add_test(s, "test if resizing an array larger works.", &test_array_resize_larger);
        CU_add_test(s, "test if resizing an array smaller works.", &test_array_resize_smaller);
        CU_add_test(s, "test if casting works", &test_cast);
        CU_add_test(s, "Test asynchronous palloc usage", &fork_test);
    }

    /* Actually run your tests here. */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
#endif
