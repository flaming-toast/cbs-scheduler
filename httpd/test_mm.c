/* An example of how to test the memory allocation interface for this
 * class. */

#ifdef MM_TEST
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "mm_alloc.h"
#include "CUnit/Basic.h"

#define NUM_PTRS 10000

static int * ptr = NULL;
static int * ptr2 = NULL;
static int *ptrs[NUM_PTRS];

/* Maximum size of an allocation. */
const size_t MAX_SIZE = 1 << 10;

static int init_suite_example(void)
{
        int j;
        for (j = 0; j < NUM_PTRS; j++)
        {
                ptrs[j] = NULL;
        }
        return 0;
}

static int clean_suite_example(void)
{
        int j;
        if (ptr != NULL)
        {
                mm_free(ptr);
                ptr = NULL;
        }
        if (ptr2 != NULL)
        {
                mm_free(ptr2);
                ptr2 = NULL;
        }
        for (j = 0; j < NUM_PTRS; j++)
        {
                if (ptrs[j] != NULL)
                {
                        mm_free(ptrs[j]);
                        ptrs[j] = NULL;
                }
        }
        for (j = 0; j < NUM_PTRS; j++)
        {
                ptrs[j] = NULL;
        }
        return 0;
}

static void test_malloc(void)
{
        ptr = mm_malloc(sizeof(*ptr));
        CU_ASSERT(ptr != NULL);
        mm_free(ptr);
        ptr = NULL;
}

static void test_malloc_1(void)
{
        return;            // NOTE: This test is deprecated.
        ptr = mm_malloc(0);
        CU_ASSERT(ptr == NULL);
        if (ptr != NULL)
                ptr[0] = 1;
}

static void test_malloc_2(void)
{
        int j = 0;
        return;            // NOTE: This test is deprecated.
        ptr = mm_malloc(sizeof(int)*MAX_SIZE);
        if (ptr != NULL)
                ptr[0] = 1;
        while (ptr != NULL && j < NUM_PTRS)
        {
                ptrs[j] = ptr;
                ptr = mm_malloc(sizeof(int)*MAX_SIZE);
                if (ptr != NULL)
                        ptr[0] = 1;
                j++;
        }
        CU_ASSERT(ptr == NULL);
}

static void test_malloc_3(void)
{
        ptr = mm_malloc(5);
        CU_ASSERT(ptr != NULL);
}

static void test_malloc_4(void)
{
        ptr = mm_malloc(12);
        ptr2 = mm_malloc(12);
        *ptr2 = 2;
        *(ptr2 + 1) = 4;
        *(ptr2 + 2) = 6;
        *ptr = 1;
        *(ptr + 1) = 3;
        *(ptr + 2) = 5;


        CU_ASSERT(ptr[0] == 1);
        CU_ASSERT(ptr[1] == 3);
        CU_ASSERT(ptr[2] == 5);
        CU_ASSERT(ptr2[0] == 2);
        CU_ASSERT(ptr2[1] == 4);
        CU_ASSERT(ptr2[2] == 6);
}

static void test_malloc_5(void)
{
        unsigned long ptr_addr;
        ptr = mm_malloc(5);
        ptr_addr = (unsigned long)ptr;
        CU_ASSERT(ptr_addr % 4 == 0);
}

static void test_malloc_6(void)
{
        unsigned long ptr_addr;
        unsigned long ptr2_addr;
        return;            // NOTE: This test is deprecated.
        ptr = mm_malloc(5);
        ptr_addr = (unsigned long)(ptr+5);
        ptr2_addr = (unsigned long)sbrk(0);
        CU_ASSERT(ptr_addr < ptr2_addr);
}

static void test_realloc_1(void)
{
        ptr = mm_malloc(5);
        ptr2 = mm_realloc(ptr, 0);
        CU_ASSERT(ptr2 == NULL);
        ptr2 = mm_realloc(ptr2, 5);
        CU_ASSERT(ptr2 != NULL);
}

static void test_realloc_2(void)
{
        ptr = mm_malloc(5);
        ptr[0] = 6;
        ptr2 = mm_realloc(ptr, 4);
        CU_ASSERT(ptr2 != NULL);
        CU_ASSERT(ptr2[0] == 6);
}

static void test_realloc_3(void)
{
        unsigned long ptr2_addr;
        ptr = mm_malloc(5);
        ptr2 = mm_realloc(ptr, 17);
        ptr2_addr = (unsigned long)ptr2;
        CU_ASSERT(ptr2_addr % 4 == 0);
}

static void test_realloc_4(void)
{
        ptr = mm_malloc(5);
        ptr[0] = 9;
        ptr2 = mm_realloc(ptr, 17);
        CU_ASSERT(ptr2[0] == 9);
}

static void test_realloc_5(void)
{
        ptr = mm_malloc(5);
        *ptr = 2;
        ptr2 = mm_realloc(ptr, 16);
        *(ptr2 + 1) = 4;
        *(ptr2 + 2) = 6;
        *(ptr2 + 3) = 8;
        CU_ASSERT(ptr2[0] == 2);
        CU_ASSERT(ptr2[1] == 4);
        CU_ASSERT(ptr2[2] == 6);
        CU_ASSERT(ptr2[3] == 8);
}

static void test_free_1(void)
{
        int j = 0;
        int size_j = 0;

        ptr = mm_malloc(sizeof(int)*MAX_SIZE);
        while (ptr != NULL && j < 50)
        {
                ptrs[j] = ptr;
                ptr = mm_malloc(sizeof(int)*MAX_SIZE);
                j++;
        }
        size_j = j;
        if (ptr != NULL)
        {
                mm_free(ptr);
                ptr = NULL;
        }
        for (j = 0; j < size_j; j++)
        {
                mm_free(ptrs[j]);
                ptrs[j] = NULL;
        }

        ptr = mm_malloc(5);
        CU_ASSERT(ptr != NULL);
        ptr[0] = 6;
        CU_ASSERT(ptr[0] == 6);
}

static void test_free_2(void)
{
        int i = 0;
        int j = 0;
        size_t my_malloc_size = 1;
        return;            // NOTE: This test is deprecated.
        printf("\n");

        for (my_malloc_size = 1; my_malloc_size < MAX_SIZE; my_malloc_size++)
        {
                if (my_malloc_size % 1 == 0)
                {
                        printf("My malloc size is now: %ld\n", my_malloc_size);
                }
                for (i = 0; i < NUM_PTRS; i++)
                {
                        ptrs[i] = mm_malloc(my_malloc_size);
                        /* Random filling to prevent optimization. */
                        for (j = 0; (j+1)*((int)sizeof(int)) <= (int)my_malloc_size; j++)
                            ptrs[i][j] = ((int)my_malloc_size) - j;
                        if ((int)my_malloc_size > 4)
                            j = ptrs[i][0];
                }
                for (i = 0; i < NUM_PTRS; i++)
                {
                        mm_free(ptrs[i]);
                }
        }
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
                CU_add_test(s, "test #1 of mm_malloc()", &test_malloc_1);
                CU_add_test(s, "test #2 of mm_malloc()", &test_malloc_2);
                CU_add_test(s, "test #3 of mm_malloc()", &test_malloc_3);
                CU_add_test(s, "test #4 of mm_malloc()", &test_malloc_4);
                CU_add_test(s, "test #5 of mm_malloc()", &test_malloc_5);
                CU_add_test(s, "test #6 of mm_malloc()", &test_malloc_6);
                CU_add_test(s, "test #1 of mm_realloc()", &test_realloc_1);
                CU_add_test(s, "test #2 of mm_realloc()", &test_realloc_2);
                CU_add_test(s, "test #3 of mm_realloc()", &test_realloc_3);
                CU_add_test(s, "test #4 of mm_realloc()", &test_realloc_4);
                CU_add_test(s, "test #5 of mm_realloc()", &test_realloc_5);
                CU_add_test(s, "test #1 of mm_free()", &test_free_1);
                CU_add_test(s, "test #2 of mm_free()", &test_free_2);
        }

        /* Actually run your tests here. */
        CU_basic_set_mode(CU_BRM_VERBOSE);
        CU_basic_run_tests();
        CU_cleanup_registry();
        return CU_get_error();
}
#endif
