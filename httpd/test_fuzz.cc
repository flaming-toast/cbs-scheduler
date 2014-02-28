/*
 * test_fuzz.cc
 *
 * Here's a basic fuzz tester for mm_alloc. It isn't necessarily compatible
 * with libc malloc.
 */
// old

#ifdef MM_TEST

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <vector>
#include <stdexcept>
#include <utility>
using namespace std;

#include "CUnit/Basic.h"

#include "mm_alloc.h"

/* Number of test iterations. */
const int ROUNDS = 1000;

/* Maximum size of the pointer store. */
const size_t NPTRS = 1000;

/* Maximum size of an allocation. */
const size_t MAX_SIZE = 1 << 10;

/* (Pointer, Allocation Size) store. */
static vector< pair<char*, size_t> > ptrs;

static char magic(size_t n)
{
    return (char) ((n % 254) + 1);
}

/* Add a new pointer. */
static void genptr()
{
    size_t n = ((size_t) rand()) % MAX_SIZE;
    if(n == 0)
    {
            return;
    }
    char* p = (char*) mm_malloc(n);

    CU_ASSERT(p != NULL);
    CU_ASSERT(((uint64_t) (p-4)) % 4 == 0);

    for (size_t i=0; i < n; ++i) {
        CU_ASSERT(p[i] == 0);
    }

    for (size_t i=0; i < n; ++i) {
        p[i] = magic(i);
    }

    ptrs.push_back(pair<char*, size_t>(p, n));
}

/* Remove and free a random pointer. */
void rmptr()
{
    size_t s = ptrs.size();
    size_t idx = ((size_t) rand()) % s;

    char* p = ptrs[idx].first;
    size_t size = ptrs[idx].second;

    for (size_t i=0; i < size; ++i) {
        CU_ASSERT(p[i] == magic(i));
    }
    mm_free(p);

    ptrs[idx] = ptrs[s - 1];
    ptrs.pop_back();
}

/* Have exactly N pointers in the store. */
void fixup_pointers(size_t N)
{
    while (ptrs.size() < N) {
        genptr();
    }

    while (ptrs.size() > N) {
        rmptr();
    }
}

int init_fuzz(void)
{
    srand(0);
    ptrs.reserve(NPTRS);
    return 0;
}

int clean_fuzz(void)
{
    ptrs.clear();
    return 0;
}

void test(void)
{
    for (int i=0; i < ROUNDS; ++i) {
        size_t N = ((size_t) rand()) % NPTRS;
        fixup_pointers(N);
    }
    fixup_pointers(0);
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

