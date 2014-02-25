/*
 * test_fuzz.cc
 *
 * Here's a basic fuzz tester for mm_alloc. It isn't necessarily compatible
 * with libc malloc.
 */

#ifdef MM_TEST

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <pthread.h>

#include <vector>
#include <stdexcept>
#include <utility>
using namespace std;

#include "CUnit/Basic.h"

#include "mm_alloc.h"

/* Number of test iterations. */
const int ROUNDS = 100;

/* Maximum number of test threads. */
const int MAX_THREADS = 16;

static pthread_t pthread_vector[MAX_THREADS];
static int pthread_vector_size = 0;

/* Maximum size of the pointer store. */
const size_t NPTRS = 1000;

/* Maximum size of an allocation. */
const size_t MAX_SIZE = 1 << 10;

/* (Pointer, Allocation Size) store. */
//static vector< pair<char*, size_t> > ptrs;
static pair<char*, size_t> ptrs[MAX_THREADS][NPTRS];
static size_t ptrs_sizes[MAX_THREADS];

static int thread_nums[MAX_THREADS];

static char magic(size_t n)
{
    return (char) ((n % 254) + 1);
}

/* Add a new pointer. */
static void genptr(int thread_num)
{
    size_t n = 1 + (((size_t) rand()) % MAX_SIZE);
    char* p = (char*) mm_malloc(n);
    size_t s = ptrs_sizes[thread_num];

    if ((uint64_t)p % 4 != 0) {
        printf("%lu %d\n ss\n ",(unsigned long)p, thread_num); exit(1);}
    CU_ASSERT(p != NULL);
    CU_ASSERT(((uint64_t) (p-4)) % 4 == 0);

    for (size_t i=0; i < n; ++i) {
        CU_ASSERT(p[i] == 0);
    }

    for (size_t i=0; i < n; ++i) {
        p[i] = magic(i);
    }

    ptrs[thread_num][s] = pair<char*, size_t>(p, n);
    ptrs_sizes[thread_num]++;
}

/* Remove and free a random pointer. */
void rmptr(int thread_num)
{
    size_t s = ptrs_sizes[thread_num];
    size_t idx = ((size_t) rand()) % s;

    char* p = ptrs[thread_num][idx].first;
    size_t size = ptrs[thread_num][idx].second;

    for (size_t i=0; i < size; ++i) {
        CU_ASSERT(p[i] == magic(i));
    }
    mm_free(p);

    ptrs[thread_num][idx].first = ptrs[thread_num][s - 1].first;
    ptrs[thread_num][idx].second = ptrs[thread_num][s - 1].second;
    //ptrs[thread_num][s - 1] = NULL;
    ptrs_sizes[thread_num]--;
}

/* Have exactly N pointers in the store. */
void fixup_pointers(size_t N, int thread_num)
{
    while (ptrs_sizes[thread_num] < N) {
        genptr(thread_num);
    }

    while (ptrs_sizes[thread_num] > N) {
        rmptr(thread_num);
    }
}

int init_fuzz(void)
{
    int j;
    srand(0);
    for (j = 0; j < MAX_THREADS; j++)
        thread_nums[j] = -1;
    return 0;
}

int clean_fuzz(void)
{
    int j;
    for (j = 0; j < MAX_THREADS; j++)
      if (thread_nums[j] != -1)
        pthread_cancel(pthread_vector[j]);
    return 0;
}

void *per_thread_pointers(void *thread_num_ptr)
{
    size_t N = 1 + (((size_t) rand()) % NPTRS);
    int thread_num = *((int *)thread_num_ptr);
    fixup_pointers(N, thread_num);
    return NULL;
}

void test(void)
{
    int j;
    for (int i = 0; i < ROUNDS; ++i)
    {
        //size_t N = 1 + (((size_t) rand()) % NPTRS);
        //fixup_pointers(N);
        int total_threads = 1 + (((int) rand()) % MAX_THREADS);
        for (j = 0; j < total_threads; j++)
        {
            thread_nums[j] = j;
            pthread_t new_thread;
            pthread_create(&new_thread, NULL, per_thread_pointers, &thread_nums[j]);
            pthread_vector[pthread_vector_size] = new_thread;
            pthread_vector_size++;
        }
        for (j = 0; j < pthread_vector_size; j++)
        {
            if (pthread_join(pthread_vector[j], NULL))
                CU_ASSERT(false);  /* Something went wrong joining */
            thread_nums[j] = -1;
        }
        pthread_vector_size = 0;
    }
    for (j = 0; j < MAX_THREADS; j++)
    {
        fixup_pointers(0, j);
    }
    CU_ASSERT(true);
}

int main()
{
    CU_pSuite pSuite = NULL;
    if (CUE_SUCCESS != CU_initialize_registry()) {
        return CU_get_error();
    }

    pSuite = CU_add_suite("fuzz", init_fuzz, clean_fuzz);
    if (NULL == pSuite) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    if (NULL == CU_add_test(pSuite, "fuzz test", test)) {
        CU_cleanup_registry();
        return CU_get_error();
    }

    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}

#endif /* MM_TEST */
