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



static int pstart;
/**
 * Simple test to see if Palloc works
 * It uses palloc to initialize a bunch of integers over and over again, each time setting
 * the value there to 1, and the freeing it. This is to test if memory is zero'd or not.
 */
static void test_palloc(void)
{
	void *local_context = palloc_init("Local Test");
    int i;
    for (i = 0; i < 200; i += 1) {
        int *ptr = palloc(local_context, int);
        CU_ASSERT(ptr != NULL);
        CU_ASSERT(*ptr == 0);
        *ptr = 1;
        pfree(ptr);
        ptr = NULL;
    }
    pfree(local_context);
    local_context = NULL;
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
	void *local_context = palloc_init("Local Test");
    void *local_child1 = palloc(local_context, int);
    void *local_child2 = palloc(local_child1, int);
    int local_return = pfree(local_child1);
    CU_ASSERT(local_return == 0);
    CU_ASSERT(local_context != NULL);
    CU_ASSERT(local_child1 != NULL);
    CU_ASSERT(local_child2 != NULL);
    palloc_print_tree(local_context);
    local_child1 = NULL;
    local_child2 = NULL;
    local_return = pfree(local_child2);
    CU_ASSERT(local_return == -1);
    pfree(local_context);
    local_context = NULL;

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
	void *local_context = palloc_init("Local Test");
    void *local_child1 = palloc(local_context, int);
    int local_return;
    palloc_destructor(local_child1, &return_invalid);
    palloc_destructor(local_child1, NULL); 
    local_return = pfree(local_child1);
    CU_ASSERT(local_return == 0);
    local_child1 = NULL;
    pfree(local_context);
    local_context = NULL;
}

/**
 * We test to make sure a destructor returning -1 is passed through
 * as a result of pfree per the palloc.h file. We do this by adding the
 * return_invalid destructor to a call, and then removing it. We expect the
 * free to return -1.
 */
static void test_destructor_invalid(void)
{
	void *local_context = palloc_init("Local Test");
    int local_return;
    palloc_destructor(local_context, &return_invalid); 
    local_return = pfree(local_context);
    CU_ASSERT(local_return == -1);
    //palloc_destructor(local_context, NULL); TODO: Modify based on Piazza reply
    local_context = NULL;
    
}

/*
 * We test to make sure having a destructor doesn't break free.
 * We do this by adding the return_valid destructor
 * to a call. We expect the free to return 0.
 */
static void test_destructor_valid(void)
{
	void *local_context = palloc_init("Local Test");
    void *local_child1 = palloc(local_context, int);
    int local_return;
    palloc_destructor(local_child1, &return_valid);
    local_return = pfree(local_child1);
    CU_ASSERT(local_return == 0);
    local_child1 = NULL;
    //palloc_destructor(local_child1, NULL);
    pfree(local_context);

    local_context = NULL;
}

/* Simple test for string. */
static void test_string(void) {
	void *local_context = palloc_init("Local Test");
    char *local_stringChild = palloc_strdup(local_context, "Test");
    int local_return;
    CU_ASSERT(local_stringChild != NULL);
    CU_ASSERT(strcmp(local_stringChild, "Test") == 0);
    local_return = pfree(local_stringChild);
    CU_ASSERT(local_return == 0);
    local_stringChild = NULL;
    pfree(local_context);
    local_context = NULL;
}

/** 
 * Test if creating an array and adding elements works.
 */
static void test_array_simple(void) {
	void *local_context = palloc_init("Local Test");
    char **local_arrayChild = palloc_array(local_context, char*, 5);
    int local_return, i;
    char *local_pointer;
    CU_ASSERT(local_arrayChild != NULL);
    for (i = 0; i < 5; i += 1) {
    	local_arrayChild[i] = palloc_strdup(local_context, "Child");
        //printf("Array Initialized");
        CU_ASSERT(local_arrayChild[i] != NULL);
        local_pointer = (char*) local_arrayChild[i];
        CU_ASSERT(strcmp(local_pointer, "Child") == 0);
    }
    local_return = pfree(local_context);
    CU_ASSERT(local_arrayChild != NULL);
    CU_ASSERT(local_return == 0);
    local_arrayChild = NULL;
    local_context = NULL;
    local_pointer = NULL;
}




/** Test if making an array larger works. We repeat test_array_simple,
 *  then resize to 7, then verify contents of previous to realloc to ensure
 *  they were copied.
 */
static void test_array_resize_larger(void) {
	void *local_context = palloc_init("Local Test");
    char **local_arrayChild = palloc_array(local_context, char*, 5);
    int local_return, i;
    char *local_pointer;
    CU_ASSERT(local_arrayChild != NULL);
    for (i = 0; i < 5; i += 1) {
    	local_arrayChild[i] = palloc_strdup(local_arrayChild, "Child");
        CU_ASSERT(local_arrayChild[i] != NULL);
        local_pointer = (char *) local_arrayChild[i];
        CU_ASSERT(strcmp(local_pointer, "Child") == 0);
    }
    local_arrayChild = prealloc(local_arrayChild, sizeof(char *) * 7);
    for (i = 5; i < 7; i += 1) {
    	local_arrayChild[i] = palloc_strdup(local_arrayChild, "Larger Child");
        CU_ASSERT(local_arrayChild[i] != NULL);
        local_pointer = (char *) local_arrayChild[i];
        CU_ASSERT(strcmp(local_pointer, "Larger Child") == 0);
    }
    for (i = 0; i < 5; i += 1) {
        CU_ASSERT(local_arrayChild[i] != NULL);
        local_pointer = (char *) local_arrayChild[i];
        CU_ASSERT(strcmp(local_pointer, "Child") == 0);
    }
    local_return = pfree(local_context);
    CU_ASSERT(local_arrayChild != NULL);
    CU_ASSERT(local_return == 0);
    local_arrayChild = NULL;
    local_context = NULL;
    local_pointer = NULL;
}

/**
 * Test if making an array smaller works.
 * Not sure how to ensure size 3 is the max without running into a segmentation fault.
 */
static void test_array_resize_smaller(void) {
	void *local_context = palloc_init("Local Test");
    char **local_arrayChild = palloc_array(local_context, char*, 5);
    int local_return, i;
    char *local_pointer;
    for (i = 0; i < 5; i += 1) {
    	local_arrayChild[i] = palloc_strdup(local_arrayChild, "Child");
        CU_ASSERT(local_arrayChild[i] != NULL);
        local_pointer = (char*) local_arrayChild[i];
        CU_ASSERT(strcmp(local_pointer, "Child") == 0);
    }
    local_arrayChild = prealloc(local_arrayChild, sizeof(char *) * 3);
    for (i = 0; i < 3; i += 1) {
        CU_ASSERT(local_arrayChild[i] != NULL);
        local_pointer = (char*) local_arrayChild[i];
        CU_ASSERT(strcmp(local_pointer, "Child") == 0);
    }
    local_return = pfree(local_context);
    CU_ASSERT(local_arrayChild != NULL);
    CU_ASSERT(local_return == 0);
    local_arrayChild = NULL;
    local_context = NULL;
    local_pointer = NULL;
}


/** 
 * Test if casting works.
 */
static void test_cast(void) {
	void *local_context = palloc_init("Local Test");
    char *local_stringChild = palloc_strdup(local_context, "Test");
    CU_ASSERT(palloc_cast(local_stringChild, char) != NULL);
    CU_ASSERT(palloc_cast(local_stringChild, int) == NULL);
    pfree(local_context);
    local_context = NULL;
}

/**
 * Tests asynchronous string actions on a shared context.
 */
static void child_string(void *local_context) {
	char strgen[30];
    char *localString; 
    int randomwait, counter, localreturn;
    srand(time(NULL));
    randomwait = rand() % 100;
    sprintf(strgen, "%d", randomwait);
    localString = palloc_strdup(local_context, strgen);
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

/**
 * Tests various array actions on a shared context
 * This function combines the three array tests.
 */
static void child_array(void *local_context) {
	char strgen[30];
	char strgen2[30];
    char **localArray = palloc_array(local_context, char*, 5); 
    char *localpointer;
    int randomwait, localreturn, i;
    srand(time(NULL));
    randomwait = rand() % 50;
    sprintf(strgen, "%d", randomwait);
    for (i = 0; i < 5; i += 1) {
        localArray[i] = palloc_strdup(localArray, strgen);
        CU_ASSERT(localArray[i] != NULL);
        localpointer = (char*) localArray[i];
        CU_ASSERT(strcmp(localpointer, strgen) == 0);
    }
    localArray = prealloc(localArray, sizeof(char *) * 3);
    for (i = 0; i < 3; i += 1) {
        CU_ASSERT(localArray[i] != NULL);
        localpointer = (char*) localArray[i];
        CU_ASSERT(strcmp(localpointer, strgen) == 0);
    }
    sprintf(strgen2, "%d", (randomwait + 1));
    localArray = prealloc(localArray, sizeof(char *) * 7);
    for (i = 3; i < 7; i += 1) {
        localArray[i] = palloc_strdup(localArray, strgen2);
        CU_ASSERT(localArray[i] != NULL);
        localpointer = (char*) localArray[i];
        CU_ASSERT(strcmp(localpointer, strgen2) == 0);
    }
    for (i = 0; i < 3; i += 1) {
        CU_ASSERT(localArray[i] != NULL);
        localpointer = (char*) localArray[i];
        CU_ASSERT(strcmp(localpointer, strgen) == 0);
    }
    localreturn = pfree(localArray);
    CU_ASSERT(localreturn == 0);
    localArray = NULL;
    localpointer = NULL;
}

/**
 * Tests various destructor actions on a shared context.
 * Combination of the three destructor tests.
 */
static void child_destructor(void *local_context) {
    void *localInt = palloc(local_context, int);
    int localreturn;
    palloc_destructor(localInt, &return_valid);
    localreturn = pfree(localInt);
    CU_ASSERT(localreturn == 0);
    localInt = palloc(local_context, int);
    palloc_destructor(localInt, &return_invalid);
    localreturn = pfree(localInt);
    CU_ASSERT(localreturn == -1);
    localInt = palloc(local_context, int);
    palloc_destructor(localInt, &return_invalid);
    palloc_destructor(localInt, NULL);
    localreturn = pfree(localInt);
    CU_ASSERT(localreturn == 0);
    localInt = NULL;
}

/**
 * Tests casting a child on a shared context
 */

static void child_cast(void *local_context) {
    int localreturn;
    char *localString = palloc_strdup(local_context, "Test");

    srand(time(NULL));
    
    CU_ASSERT(palloc_cast(localString, char) != NULL);
    
    CU_ASSERT(palloc_cast(localString, int) == NULL);
    localreturn = pfree(localString);
    CU_ASSERT(localreturn == 0);
    localString = NULL;
}


/** What each child does, a combination of other previous tests. */
static void * children_tests(void *ptr) {
	int order = 0;
	while(pstart == 0) {
		//Wait to start same time
	}
    srand(time(NULL));
	order = rand() % 3;
	if(order == 0) {
		child_string(ptr);
		child_array(ptr);
		child_destructor(ptr);
		child_cast(ptr);
	} else if(order == 1) {
		child_cast(ptr);
		child_destructor(ptr);
		child_array(ptr);
		child_string(ptr);
	} else if(order == 2) {
		child_array(ptr);
		child_string(ptr);
		child_cast(ptr);
		child_destructor(ptr);
	} else if(order == 3) {
		child_destructor(ptr);
		child_cast(ptr);
		child_string(ptr);
		child_array(ptr);
	}

	return NULL;
}

/** Method to create children.
 *
 */
static void * create_children(void *ptr) {
	pthread_t children[15];
	int count = 0;
	pstart = 0;
	for (count = 0; count < 15; count += 1) {
		pthread_create(&children[count], NULL, &children_tests, ptr);
	}
	pstart = 1;
	for (count = 0; count < 15; count += 1) {
		pthread_join(children[count], NULL);
	}

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
    void *local_context;
    int i;
	for(i = 0; i < 5; i += 1) {
		local_context = palloc_init("Local Test");
		create_children(local_context);
		pfree(local_context);
		local_context = NULL;
	}

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
