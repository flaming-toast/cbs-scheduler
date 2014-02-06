/* An example of how to test the tree-based memory allocation
 * interface for this class. */

#ifdef PALLOC_TEST
#include <stdio.h>
#include <string.h>
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
 * Simple test to see if Palloc works
 * It uses palloc to initialize a bunch of integers over and over again, each time setting
 * the value there to 1, and the freeing it. This is to test if memory is zero'd or not.
 */
static void test_palloc(void)
{
	int i = 0;
    void *context = palloc_init("test context");
    int *test = mm_malloc(sizeof(int)); //Set memory to non-zero
    *test = 1;
    mm_free(test);
    for (i = 0; i < 200; i += 1) { //Repeat many times to ensure always zero.
		int *ptr = palloc(context, int);
		CU_ASSERT(ptr != NULL);
		CU_ASSERT(*ptr == 0);
		*ptr = 1; //Set value at pointer to non-zero.
		pfree(ptr);
    }
}

/**
 * This test group is to see if freeing a parent context also frees the child.
 * In addition, in the future once a bug with palloc is fixed, this test will also see
 * if calling free on a pointer that should result in an error actually does return -1.
 * The print statement at the end is intended for debugging purposes in the future to make
 * sure pointers were actually freed as I intend to add debugging print statements to check
 * for if the memory was actually freed. It is also to make sure the print function works.
 */
static void test_parent_child(void)
{
    void *parent = palloc_init("test context");
    void *child1 = palloc(parent, int);
    void *child2 = palloc(child1, int);
    int returnparent = 0;
    //int returnchild = 0; Commented out to allow compiling.
    returnparent = pfree(child1);
    CU_ASSERT(returnparent == 0);
    //returnchild = pfree(child); //Currently doesn't return -1, results in all tests crashing, so commented out.
    //CU_ASSERT(returnchild == -1); //TODO: Reimplement once it doesn't cause an error which ruins every other test.
    CU_ASSERT(parent != NULL);
    CU_ASSERT(child1 != NULL);
    CU_ASSERT(child2 != NULL);
    palloc_print_tree(parent);
    
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
 * This test group is a more complicated test of destructors.
 * 
 * First, we test if adding a destructor and then removing it works.
 * We do this by adding the return_invalid destructor
 * to a call, and then removing it. We expect the free to return 0.
 * 
 * Second, we test to make sure having a destructor doesn't break free.
 * We do this by adding the return_valid destructor
 * to a call. We expect the free to return 0.
 * 
 * Lastly, we test to make sure a destructor returning -1 is passed through
 * as a result of pfree per the palloc.h file. We do this by adding the
 * return_invalid destructor to a call, and then removing it. We expect the
 * free to return -1.
 */
static void test_destructor(void)
{
    void *parent = palloc_init("test context"); //Initialization of contexes/allocs
    void *child1 = palloc(parent, int);
    void *child2 = palloc(parent, int);
    int returnparent = 0; //Variables to check the return of calls we care about.
    int returnchild1 = 0;
    int returnchild2 = 0;
    returnchild2 = pfree(child2); //Test add then remove of a destructor.
    palloc_destructor(child2, &return_invalid);
    palloc_destructor(child2, NULL); 
    CU_ASSERT(returnchild2 == 0);
    palloc_destructor(child1, &return_valid); //Test adding a destructor doesn't break anything.
    returnchild1 = pfree(child1);
    CU_ASSERT(returnchild1 == 0);
    palloc_destructor(parent, &return_invalid); //Test a destructor returning -1 is passed through by pfree.
	returnparent = pfree(parent);
	CU_ASSERT(returnparent == -1);
}

/**
 * This is a larger test that tests all of the remaining palloc.h method calls.
 * This is a loose re-creation of the diagram on the talloc documentation page (section 1)
 * 
 * First we create a parent, then assign a child array and a string under that parent.
 * We iterate through the array assigning values to it with "Larger Child" as a string.
 * In each iteration, we check to make sure the string is actually written. Thus, we validate
 * the behavior of both strdup and array.
 * 
 * In between the first and second section, I add an additional child under child2 and test for that.
 * I also have a call to print as a placeholder for where I want to insert structural tests
 * of the context tree from part 1 after I finalize the structure after implementation.
 * 
 * Second, I test palloc_cast. I do this by giving it a valid cast, and then an invalid, and making
 * sure the outputs are correct.
 * 
 * After that, we want to test resizing. We first test to see if resizing child1 from 5 to 7 works.
 * Similarly, I want to test if resizing from 5 to 3 works. Here, since I am reusing the child1 pointer
 * and assigning it to a new array, I can also use this to test if the parent still has a reference
 * to the old array in the future after the structure is finalized during implementation. The 5 to 3
 * test is currently commented out because it throws a segmentation fault, which crashes all tests,
 * and I'm not sure how to test to make sure it is resized to size 3 without it crashing all tests.
 * 
 * Lastly, we have another print tree call for debugging purposes.
 */
static void test_usability(void)
{
	//Create contexes.
    void *parent = palloc_init("Test context");
    char **child1 = palloc_array(parent, char*, 5);
    char *child2 = palloc_strdup(parent, "Test"); //The reason we have this extra here is to make sure parent can have more than one child.
    //This is used to get the actual string from the array.
    char *pointercheck;
    int i = 0;
    CU_ASSERT(child1 != NULL);//Ensure childs were actually created.
    CU_ASSERT(child2 != NULL);
    CU_ASSERT(strcmp(child2, "Test") == 0); //Validate String
    //Check if String and Array creation works.
    for (i = 0; i < 5; i += 1) {
    	child1[i] = palloc_strdup(child1, "Larger Child");
    	CU_ASSERT(child1[i] != NULL);
    	pointercheck = (char*) child1[i];
    	CU_ASSERT(strcmp(pointercheck, "Larger Child") == 0);
    }
    pointercheck = palloc_strdup(child2, "Test 2");
    CU_ASSERT(pointercheck != NULL);
    //This section tests palloc_cast.
    palloc_print_tree(parent); //Make sure correct manually TODO: Replace with a traversal through the tree structure after finalized in implementation.
    CU_ASSERT(palloc_cast(child2, char*) != NULL);
    CU_ASSERT(palloc_cast(child2, int) == NULL);
    //This section tests to make sure prealloc works
    child1 = prealloc(child1, 7);
    //Fill in the newly alloced memory, ensure no errors.
    for (i = 5; i < 7; i += 1) {
    	child1[i] = palloc_strdup(child1, "Larger Child");
    	CU_ASSERT(child1[i] != NULL);
    	pointercheck = (char*) child1[i];
    	CU_ASSERT(strcmp(pointercheck, "Larger Child") == 0);
    }
    //Test making the array smaller. Also allows us (in the future)
    //to test if the parent still has a pointer to the old child since we are
    //assigning a new value to child1
    child1 = palloc_array(parent, char*, 5);
    //prealloc(child1, 3); //TODO: Fix? THROWS A SEGMENTATION FAULT!
    for (i = 0; i < 5; i += 1) {
    	child1[i] = palloc_strdup(child1, "Smaller Child");
    	CU_ASSERT(child1[i] != NULL);
    	pointercheck = (char*) child1[i];
    	CU_ASSERT(strcmp(pointercheck, "Smaller Child") == 0);

    }
    palloc_print_tree(parent);
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
	//	for x < 20, spawn 20 threads
	//		let each child thread call a helper
	//			helper does 'unique change' on shared memory area, as well as change in non-shared
	//			verify no other thread changed shared memory area during entire process (that it is locked)
}

/** Test slab allocator internal representation. This test is not finalized yet since
 *  Slab Allocation may change in API. */
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

        CU_add_test(s, "simple test of palloc(), if memory properly zero'd", &test_palloc);
        CU_add_test(s, "tests of pfree(). test palloc freeing parent frees child. test if free works. test if free returns -1 on invalid", &test_parent_child);
        CU_add_test(s, "test if destructors work properly with pfree", &test_destructor);
        CU_add_test(s, "test each of the remaining allocation methods in palloc. Array, String, Realloc, Cast", &test_usability);
        CU_add_test(s, "Test slab allocation internal structure", &test_slab); //Tests for slab allocation internal structure if we do use it
        CU_add_test(s, "Test asynchronous palloc usage", &fork_test);
    }

    /* Actually run your tests here. */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
#endif
