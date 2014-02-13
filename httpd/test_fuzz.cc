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

static vector< pthread_t > pthread_vector;

/* Maximum size of the pointer store. */
const size_t NPTRS = 1000;

/* Maximum size of an allocation. */
const size_t MAX_SIZE = 1 << 10;

/* (Pointer, Allocation Size) store. */
//static vector< pair<char*, size_t> > ptrs;
static vector<vector< pair<char*, size_t> >> ptrs;

static vector<int> thread_nums;

static char magic(size_t n)
{
    return (char) ((n % 254) + 1);
}

/* Add a new pointer. */
static void genptr(int thread_num)
{
    size_t n = ((size_t) rand()) % MAX_SIZE;
    char* p = (char*) mm_malloc(n);

    CU_ASSERT(p != NULL);
    CU_ASSERT(((uint64_t) (p-4)) % 4 == 0);

    for (size_t i=0; i < n; ++i) {
        CU_ASSERT(p[i] == 0);
    }

    for (size_t i=0; i < n; ++i) {
        p[i] = magic(i);
    }

    ptrs[thread_num].push_back(pair<char*, size_t>(p, n));
}

/* Remove and free a random pointer. */ 
void rmptr(int thread_num)
{
    size_t s = ptrs[thread_num].size();
    size_t idx = ((size_t) rand()) % s;

    char* p = ptrs[thread_num][idx].first;
    size_t size = ptrs[thread_num][idx].second;

    for (size_t i=0; i < size; ++i) {
        CU_ASSERT(p[i] == magic(i));
    }
    mm_free(p);

    ptrs[thread_num][idx] = ptrs[thread_num][s - 1];
    ptrs[thread_num].pop_back();
}

/* Have exactly N pointers in the store. */
void fixup_pointers(size_t N, int thread_num)
{
    while (ptrs[thread_num].size() < N) {
        genptr(thread_num);
    }

    while (ptrs[thread_num].size() > N) {
        rmptr(thread_num);
    }
}

int init_fuzz(void)
{
    int j;
    srand(0);
    ptrs.reserve(MAX_THREADS);
    thread_nums.reserve(MAX_THREADS);
    for (j = 0; j < MAX_THREADS; j++)
        thread_nums.push_back(-1);
    for (j = 0; j < MAX_THREADS; j++)
    {
        vector< pair<char*, size_t> > new_ptr_vector;
        new_ptr_vector.reserve(NPTRS);
        ptrs.push_back(new_ptr_vector);
    }
    pthread_vector.reserve(MAX_THREADS);
    return 0;
}

int clean_fuzz(void)
{
    int j;
    for (j = 0; j < (int)(ptrs.size()); j++)
        ptrs[j].clear();
    ptrs.clear();
    for (j = 0; j < (int)(pthread_vector.size()); j++)
        pthread_cancel(pthread_vector[j]);
    pthread_vector.clear();
    thread_nums.clear();
    return 0;
}

void *per_thread_pointers(void *thread_num_ptr)
{
    size_t N = ((size_t) rand()) % NPTRS;
    int thread_num = *((int *)thread_num_ptr);
    fixup_pointers(N, thread_num);
    return NULL;
}

void test(void)
{
    int j;
    for (int i = 0; i < ROUNDS; ++i)
    {
        //size_t N = ((size_t) rand()) % NPTRS;
        //fixup_pointers(N);
        int total_threads = ((int) rand()) % MAX_THREADS;
        for (j = 0; j < total_threads; j++)
        {
            thread_nums[j] = j;
            pthread_t new_thread;
            pthread_create(&new_thread, NULL, per_thread_pointers, &thread_nums[j]);
            pthread_vector.push_back(new_thread);
        }
        for (j = 0; j < (int)(pthread_vector.size()); j++)
        {
            if (pthread_join(pthread_vector[j], NULL))
                CU_ASSERT(false);  /* Something went wrong joining */
        }
        pthread_vector.clear();
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
