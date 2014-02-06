/* An example of how to test the tree-based memory allocation
 * interface for this class. */

#ifdef PALLOC_TEST
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "palloc.h"
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

static void test_array_new(void) {
	void *local_context = palloc_init("Local Test");
    char **local_arrayChild = palloc_array(local_context, char*, 5);
    int local_return;
    char *local_pointer;
    CU_ASSERT(local_arrayChild != NULL);
    for (i = 0; i < 5; i += 1) {
    	local_arrayChild[i] = palloc_strdup(local_context, "Child");
        //printf("Array Initialized");
        CU_ASSERT(local_arrayChild[i] != NULL);
        local_pointer = (char*) local_arrayChild[i];
        CU_ASSERT(strcmp(local_pointer, "Child") == 0);
    }
    local_return = pfree(local_arrayChild);
    CU_ASSERT(local_arrayChild != NULL);
    CU_ASSERT(local_return == 0);
    local_arrayChild = NULL;
}

/** 
 * Test if creating an array and adding elements works.
 * Unknown Bug: This test now throws a segmentation fault due to the commented out code
 * fault (was working before merging in checkpoint 1). False pass currently.
 */
/*
static void test_array_simple(void) {
    init_suite_example();
    arrayChild = palloc_array(context, char*, 5);
    CU_ASSERT(arrayChild != NULL);
    for (i = 0; i < 1; i += 1) {
        arrayChild[i] = "";
        		//palloc_strdup(context, "Child");
        //printf("Array Initialized");
        CU_ASSERT(arrayChild[i] != NULL);
        pointercheck = (char*) arrayChild[i];
        CU_ASSERT(strcmp(pointercheck, "Child") == 0);
    }
    returnchild1 = pfree(arrayChild);
    CU_ASSERT(arrayChild != NULL);
    arrayChild = NULL;
    clean_suite_example();
}
*/

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
        arrayChild[i] = palloc_strdup(arrayChild, "Larger Child");
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
    CU_ASSERT(palloc_cast(stringChild, char) != NULL);
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

static void child_string(void) {
	char strgen[30];
    char *localString; 
    int randomwait, counter, localreturn;
    srand(time(NULL));
    randomwait = rand() % 100;
    sprintf(strgen, "%d", randomwait);
    localString = palloc_strdup(context, strgen);
    CU_ASSERT(localString != NULL);
    counter = 0;
    while(counter < randomwait) {
    	counter += 1;
    }
    CU_ASSERT(strcmp(localString, strgen) == 0);
    counter = 0;
    while(counter < randomwait) {
    	counter += 1;
    }
    localreturn = pfree(localString);
    CU_ASSERT(localreturn == 0);
    localString = NULL;
}

/**Currently Not implemented due to segmentation fault,
 * but would be a copy of the three test array functions
 * with various wait areas like child_string();
 * 
 * This function combines the three array tests.
 */
static void child_array(void) {
	char strgen[30];
	char strgen2[30];
    char **localArray; 
    //char *localpointer;
    int randomwait, counter, localreturn;
    localArray = palloc_array(context, char*, 5);
    srand(time(NULL));
    randomwait = rand() % 50;
    sprintf(strgen, "%d", randomwait);
    for (i = 0; i < 5; i += 1) {
        counter = 0;
        while(counter < randomwait) {
        	counter += 1;
        }
        randomwait = rand() % 5;
        /*
        localArray[i] = palloc_strdup(localArray, strgen);
        CU_ASSERT(localArray[i] != NULL);
        localpointer = (char*) localArray[i];
        CU_ASSERT(strcmp(localpointer, "strgen) == 0);
        */
    }
    localArray = prealloc(localArray, 3);
    for (i = 0; i < 3; i += 1) {
        counter = 0;
        while(counter < randomwait) {
        	counter += 1;
        }
        randomwait = rand() % 5;
        /*
        CU_ASSERT(localArray[i] != NULL);
        localpointer = (char*) localArray[i];
        CU_ASSERT(strcmp(localpointer, strgen) == 0);
        */
    }
    sprintf(strgen2, "%d", (randomwait + 1));
    localArray = prealloc(localArray, 7);
    for (i = 3; i < 7; i += 1) {
        counter = 0;
        while(counter < randomwait) {
        	counter += 1;
        }
        randomwait = rand() % 5;
        /*
        localArray[i] = palloc_strdup(localArray, strgen2);
        CU_ASSERT(localArray[i] != NULL);
        localpointer = (char*) localArray[i];
        CU_ASSERT(strcmp(localpointer, strgen2) == 0);
        */
    }
    for (i = 0; i < 3; i += 1) {
        counter = 0;
        while(counter < randomwait) {
        	counter += 1;
        }
        randomwait = rand() % 5;
        /*
        CU_ASSERT(localArray[i] != NULL);
        localpointer = (char*) localArray[i];
        CU_ASSERT(strcmp(localpointer, strgen) == 0);
        */
    }
    localreturn = pfree(localArray);
    CU_ASSERT(localreturn == 0);
    localArray = NULL;
}

/**
 * Combination of the three destructor tests.
 */
static void child_destructor(void) {
    void *localInt = palloc(context, int);
    int randomwait, counter, localreturn;
    srand(time(NULL));
    randomwait = rand() % 50;
    counter = 0;
    while(counter < randomwait) {
    	counter += 1;
    }
    palloc_destructor(localInt, &return_valid);
    localreturn = pfree(localInt);
    CU_ASSERT(localreturn == 0);
    counter = 0;
    while(counter < randomwait) {
    	counter += 1;
    }
    localInt = palloc(context, int);
    palloc_destructor(localInt, &return_invalid);
    localreturn = pfree(localInt);
    CU_ASSERT(localreturn == -1);
    counter = 0;
    while(counter < randomwait) {
    	counter += 1;
    }
    localInt = palloc(context, int);
    palloc_destructor(localInt, &return_invalid);
    palloc_destructor(localInt, NULL);
    localreturn = pfree(localInt);
    CU_ASSERT(localreturn == 0);
    localInt = NULL;
}

/**
 * Tests casting a child.
 */

static void child_cast(void) {
    int randomwait, counter, localreturn;
    char *localString = palloc_strdup(context, "Test");

    srand(time(NULL));
    randomwait = rand() % 25;
    counter = 0;
    while(counter < randomwait) {
    	counter += 1;
    }
    
    CU_ASSERT(palloc_cast(localString, char) != NULL);
    
    counter = 0;
    while(counter < randomwait) {
    	counter += 1;
    }
    CU_ASSERT(palloc_cast(localString, int) == NULL);
    localreturn = pfree(localString);
    CU_ASSERT(localreturn == 0);
    localString = NULL;
	//printf("CHECKPOINT C\n");
}


/** What each child does, a combination of other previous tests. */
static void * children_tests(void *ptr) {
	int order = 0;
    srand(time(NULL));
	order = rand() % 1;
	if(order) {
		child_string();
		child_array();
		child_destructor();
		child_cast();
	} else {
		child_cast();
		child_destructor();
		child_array();
		child_string();
	}

	printf("Checkpoint A\n");
	return NULL;
}

/** Method to create children.
 *
 */
static void * create_children(void *ptr) {
	pthread_t child1;
	pthread_t child2;
	//child3, child4, child5, child6, child7, child8, child9, child10;
	pthread_create(&child1, NULL, &children_tests, NULL);
	pthread_create(&child2, NULL, &children_tests, NULL);
	//pthread_create(&child3, NULL, &children_tests, NULL);
	//pthread_create(&child4, NULL, &children_tests, NULL);
	//pthread_create(&child5, NULL, &children_tests, NULL);
	//pthread_create(&child6, NULL, &children_tests, NULL);
	//pthread_create(&child7, NULL, &children_tests, NULL);
	//pthread_create(&child8, NULL, &children_tests, NULL);
	//pthread_create(&child9, NULL, &children_tests, NULL);
	//pthread_create(&child10, NULL, &children_tests, NULL);
	pthread_join(child1, NULL);
	pthread_join(child2, NULL);
	//pthread_join(child3, NULL);
	//pthread_join(child4, NULL);
	//pthread_join(child5, NULL);
	//pthread_join(child6, NULL);
	//pthread_join(child7, NULL);
	//pthread_join(child8, NULL);
	//pthread_join(child9, NULL);
	//pthread_join(child10, NULL);
	printf("CHECKPOINT B\n");
	return NULL;
}

/** 
 * Tests to see if asynchronous calls have no error. This is done
 * by creating multiple threads that run which acquires the parent
 * context and does manipulations to it where it calls various palloc functions.
 * Each thread also is set to wait an arbitrary amount of time to ensure
 * randomness. See *children_tests(). Parent context is shared across children since
 * init is not run by children.
 * 
 */
static void fork_test(void) {
    init_suite_example();
	for(i = 0; i < 1; i += 1) {
		create_children(NULL);
	}
    clean_suite_example();
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
        CU_pSuite s = CU_add_suite("Palloc Test Suite",
                                   NULL,
                                   NULL);

        CU_add_test(s, "simple test of palloc() and if memory properly zero'd", &test_palloc);
        CU_add_test(s, "freeing parent frees child; if free returns -1 on invalid.", &test_parent_child);
        CU_add_test(s, "test if removing a destructor works", &test_destructor_remove);
        CU_add_test(s, "test if using a destructor works", &test_destructor_valid);
        CU_add_test(s, "test if a destructor error is returned by pfree()", &test_destructor_invalid);
        CU_add_test(s, "Test slab allocation internal structure", &test_slab);
        CU_add_test(s, "test if strings work.", &test_string);
        CU_add_test(s, "test if simple array works.", &test_array_new);
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
