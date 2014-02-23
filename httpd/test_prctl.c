#ifdef PRCTL_TEST
#include "CUnit/Basic.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <pthread.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#define SET_TL_DUMMY 41
#define GET_TL_DUMMY 42

int startFlag = 0;
int BASIC_STACK_SIZE = 500;


static int nop_init_suite(void) {
  return 0;
}

static int nop_clean_suite(void) {
  return 0;
}

/* Utility functions used in test routines */

/* Needed for passing ints around in pthread functions. */
int* gen_mp_int(int val) {
  int* v = (int*) malloc(sizeof(int));
  *v = val;
  return v;
}

void* wait_for_start_and_exit(void* ign) {
  while(!startFlag) { 
    /* spin, we don't really care */
  }
  return (void*)gen_mp_int(0);
}

void* clone_new_dummy_fn(void* fn) {
  return (void*)gen_mp_int(0);
}

void* clone_new_and_ret_pid(void* fn) {
  pthread_t th1;
  int* resTh = gen_mp_int(pthread_create(&th1, NULL, clone_new_dummy_fn, NULL));
  int* ign;
  if (*resTh > 0) {
    pthread_join(th1, ((void**)&ign));
    free(ign);
  }
  return (void*)resTh;
}

/* Test routines */

void* run_test_prctl_limit1(void* ign) {
  prctl(SET_TL_DUMMY, 1);
  return clone_new_and_ret_pid(NULL);
}

static void test_prctl_limit1_fail(void) {
  pthread_t th1;
  int resTh = pthread_create(&th1, NULL, run_test_prctl_limit1, NULL);
  int* res;
  CU_ASSERT(resTh == 0);
  if (resTh == 0) {
    pthread_join(th1, ((void**)&res));
    CU_ASSERT(*res != 0);
    free(res);
  }   
}

void* run_test_prctl_limit2_success(void* ign) {
  prctl(SET_TL_DUMMY, 2);
  int* npid = gen_mp_int(fork());
  if (*npid == 0) {
    return (void*)npid;
  }
  return (void*)npid;
}

static void test_prctl_limit2_success(void) {
  pthread_t th1;
  int resTh = pthread_create(&th1, NULL, run_test_prctl_limit2_success, NULL);
  int* res;
  CU_ASSERT(resTh == 0);
  if (resTh == 0) {
    pthread_join(th1, ((void**)&res));
    CU_ASSERT(*res > 0);
  }
}

void* run_test_prctl_limit2_failch(void* ign) {
  prctl(SET_TL_DUMMY, 2);
  pthread_t th1;
  int resTh = pthread_create(&th1, NULL, clone_new_and_ret_pid, NULL);
  int* res;
  if (resTh == 0) {
    pthread_join(th1, ((void**)&res));
  }
  return (void*)res;
}

static void test_prctl_limit2_failch(void) {
  pthread_t th1;
  int resTh = pthread_create(&th1, NULL, run_test_prctl_limit2_failch, NULL);
  int* res;
  CU_ASSERT(resTh == 0);
  if (resTh == 0) {
    pthread_join(th1, ((void**)&res));
    CU_ASSERT(*res != 0);
    free(res);
  }
}

void* run_test_prctl_limit2_failsi(void* ign) {
  prctl(SET_TL_DUMMY, 2);
  pthread_t th1;
  pthread_t th2;
  int resTh1 = pthread_create(&th1, NULL, wait_for_start_and_exit, NULL);
  int resTh2 = pthread_create(&th2, NULL, wait_for_start_and_exit, NULL);
  int* ign2;
  int* rval;
  startFlag = 1;
  if (resTh1 == 0) {
    pthread_join(th1, ((void**)&ign2));
    free(ign2);
    if (resTh2 != 0) {
      rval = gen_mp_int(-2);
    } else {
      pthread_join(th2, ((void**)&ign2));
      free(ign2);
      rval = gen_mp_int(-1);
    }
  } else {
    rval = gen_mp_int(0);
  }
  return (void*)rval;
}

static void test_prctl_limit2_failsi(void) {
  pthread_t th1;
  int resTh = pthread_create(&th1, NULL, run_test_prctl_limit2_failsi, NULL);
  int* res;
  CU_ASSERT(resTh == 0);
  if (resTh == 0) {
    pthread_join(th1, ((void**)&res));
    CU_ASSERT(*res < 0); /* The first task was created successfully */
    CU_ASSERT(*res == -2); /* The second task was not created */
    free(res);
  }
}

int main(int argc, char **argv)
{
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    {
        CU_pSuite prctlSuite = CU_add_suite("prctl tests",
                                   &nop_init_suite,
                                   &nop_clean_suite);

        CU_add_test(prctlSuite, "Test of prctl(1) disabling new thread formation", test_prctl_limit1_fail);
	CU_add_test(prctlSuite, "Test of prctl(2) allowing a child thread to be formed", test_prctl_limit2_success);
	CU_add_test(prctlSuite, "Test of prctl(2) preventing child thread from forming another child", test_prctl_limit2_failch);
	CU_add_test(prctlSuite, "Test of prctl(2) preventing a second child of the original thread from being formed", test_prctl_limit2_failsi);
    }

    /* Actually run your tests here. */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
#endif
