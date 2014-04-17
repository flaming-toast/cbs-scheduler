#ifdef CBS_TEST
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include "cbs.h"
#include "cbs_proc.h"
#include "CUnit/Basic.h"


static cbs_t *task1;
static cbs_t *task2;
static cbs_t *task3;

static struct timeval test_period = {
        .tv_sec = 1,
        .tv_usec = 0,
};

static int test_worker1(void* arg)
{
        return CBS_CONTINUE+1;  // Not CBS_CONTINUE
}

static int test_worker2(void* arg)
{
        int myarg = *((int *)arg);
        return myarg;
}

static int test_worker3(void* arg)
{
        int myarg = *((int *)arg);
        usleep(3000);
        return myarg;
}






static int init_suite_example(void)
{
        task1 = (cbs_t *)malloc(sizeof(cbs_t));
        task2 = (cbs_t *)malloc(sizeof(cbs_t));
        task3 = (cbs_t *)malloc(sizeof(cbs_t));
        return 0;
}

static int clean_suite_example(void)
{
        if (task1 != NULL) {
                free(task1);
        }
        if (task2 != NULL) {
                free(task2);
        }
        if (task3 != NULL) {
                free(task3);
        }
        return 0;
}



static void test_cbs_create_1(void)
{
        int res;
        int code;

        CU_ASSERT(task1 != NULL);
        if (task1 == NULL) {
                return; // Cannot run test, memory full.
        }

        res = cbs_create(task1, CBS_RT, 100, &test_period, test_worker1, NULL);
        CU_ASSERT(res == 0);
        if (!res) {
                res = cbs_join(task1, &code);
                CU_ASSERT(res == 0);
                CU_ASSERT(res < 1);
                CU_ASSERT(res > -1);
                CU_ASSERT(code == CBS_CONTINUE+1);
                CU_ASSERT(code < CBS_CONTINUE+2);
                CU_ASSERT(code > CBS_CONTINUE);
        }
        return;
}

static void test_cbs_create_2(void)
{
        int res;
        int code;

        CU_ASSERT(task1 != NULL);
        if (task1 == NULL) {
                return; // Cannot run test, memory full.
        }

        res = cbs_create(task1, CBS_BW, 100, &test_period, test_worker1, NULL);
        CU_ASSERT(res == 0);
        if (!res) {
                res = cbs_join(task1, &code);
                CU_ASSERT(res == 0);
                CU_ASSERT(code == CBS_CONTINUE+1);
        }
        return;
}

static void test_cbs_create_3(void)
{
        int res;
        int code;
        int myarg = CBS_CONTINUE+3;

        CU_ASSERT(task1 != NULL);
        if (task1 == NULL) {
                return; // Cannot run test, memory full.
        }

        res = cbs_create(task1, CBS_RT, 100, &test_period, test_worker2, &myarg);
        CU_ASSERT(res == 0);
        if (!res) {
                res = cbs_join(task1, &code);
                CU_ASSERT(res == 0);
                CU_ASSERT(code == myarg);
        }
        return;
}

static void test_cbs_create_4(void)
{
        int res;
        int code;
        int myarg = CBS_CONTINUE+3;

        CU_ASSERT(task1 != NULL);
        if (task1 == NULL) {
                return; // Cannot run test, memory full.
        }

        res = cbs_create(task1, CBS_BW, 100, &test_period, test_worker2, &myarg);
        CU_ASSERT(res == 0);
        if (!res) {
                res = cbs_join(task1, &code);
                CU_ASSERT(res == 0);
                CU_ASSERT(code == myarg);
        }
        return;
}

static void test_cbs_create_5(void)
{
        int res;
        int code1;
        int code2;
        int myarg = CBS_CONTINUE+3;

        CU_ASSERT(task1 != NULL);
        if (task1 == NULL) {
                return; // Cannot run test, memory full.
        }
        CU_ASSERT(task2 != NULL);
        if (task2 == NULL) {
                return; // Cannot run test, memory full.
        }

        res = cbs_create(task1, CBS_BW, 75, &test_period, test_worker3, &myarg);
        CU_ASSERT(res == 0);
        if (res) {
                goto end5_1;
        }

        res = cbs_create(task2, CBS_BW, 75, &test_period, test_worker3, &myarg);
        CU_ASSERT(res != 0);
        if (res) {
                goto end5_2;
        }

        res = cbs_join(task2, &code2);
        CU_ASSERT(res == 0);
        CU_ASSERT(code2 == myarg);
end5_2:
        res = cbs_join(task1, &code1);
        CU_ASSERT(res == 0);
        CU_ASSERT(code1 == myarg);
end5_1:
        return;
}

static void test_cbs_create_6(void)
{
        int res;
        int code1;
        int code2;
        int myarg = CBS_CONTINUE+3;

        CU_ASSERT(task1 != NULL);
        if (task1 == NULL) {
                return; // Cannot run test, memory full.
        }
        CU_ASSERT(task2 != NULL);
        if (task2 == NULL) {
                return; // Cannot run test, memory full.
        }

        res = cbs_create(task1, CBS_BW, 75, &test_period, test_worker3, &myarg);
        CU_ASSERT(res == 0);
        if (res) {
                goto end6_1;
        }

        res = cbs_create(task2, CBS_BW, 75, &test_period, test_worker3, &myarg);
        CU_ASSERT(res == 0);
        if (res) {
                goto end6_2;
        }

        res = cbs_join(task2, &code2);
        CU_ASSERT(res == 0);
        CU_ASSERT(code2 == myarg);
end6_2:
        res = cbs_join(task1, &code1);
        CU_ASSERT(res == 0);
        CU_ASSERT(code1 == myarg);
end6_1:
        return;
}

static void test_cbs_create_7(void)
{
        int res;
        int code1;
        int code2;
        int code3;        
        int myarg = CBS_CONTINUE+3;

        CU_ASSERT(task1 != NULL);
        if (task1 == NULL) {
                return; // Cannot run test, memory full.
        }
        CU_ASSERT(task2 != NULL);
        if (task2 == NULL) {
                return; // Cannot run test, memory full.
        }
        CU_ASSERT(task3 != NULL);
        if (task3 == NULL) {
                return; // Cannot run test, memory full.
        }

        res = cbs_create(task1, CBS_RT, 75, &test_period, test_worker3, &myarg);
        CU_ASSERT(res == 0);
        if (res) {
                goto end7_1;
        }

        res = cbs_create(task2, CBS_BW, 50, &test_period, test_worker3, &myarg);
        CU_ASSERT(res == 0);
        if (res) {
                goto end7_2;
        }

        res = cbs_create(task3, CBS_BW, 50, &test_period, test_worker3, &myarg);
        CU_ASSERT(res == 0);
        if (res) {
                goto end7_3;
        }
        
        res = cbs_join(task3, &code3);
        CU_ASSERT(res == 0);
        CU_ASSERT(code3 == myarg);
end7_3:
        res = cbs_join(task2, &code2);
        CU_ASSERT(res == 0);
        CU_ASSERT(code2 == myarg);
end7_2:
        res = cbs_join(task1, &code1);
        CU_ASSERT(res == 0);
        CU_ASSERT(code1 == myarg);
end7_1:
        return;
}

int main(int argc, char **argv)
{

        /* Initialize CUnit. */
        if (CUE_SUCCESS != CU_initialize_registry())
                return CU_get_error();

        /* Here's an example test suite.  First we initialize the suit. */
        {
                CU_pSuite s = CU_add_suite("CBS test suite",
                                &init_suite_example,
                                &clean_suite_example);

                CU_add_test(s, "test #1 of cbs_create()", &test_cbs_create_1);
                CU_add_test(s, "test #2 of cbs_create()", &test_cbs_create_2);
                CU_add_test(s, "test #3 of cbs_create()", &test_cbs_create_3);
                CU_add_test(s, "test #4 of cbs_create()", &test_cbs_create_4);
                CU_add_test(s, "test #5 of cbs_create()", &test_cbs_create_5);
                CU_add_test(s, "test #6 of cbs_create()", &test_cbs_create_6);
                CU_add_test(s, "test #7 of cbs_create()", &test_cbs_create_7);
        }

        /* Actually run your tests here. */
        CU_basic_set_mode(CU_BRM_VERBOSE);
        CU_basic_run_tests();
        CU_cleanup_registry();
        return CU_get_error();
}
#endif
