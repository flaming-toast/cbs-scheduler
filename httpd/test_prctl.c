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
int num_ext = 1;
int BASIC_STACK_SIZE = 500;


static int nop_init_suite(void) {
  return 0;
}

static int nop_clean_suite(void) {
  startFlag = 0;
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

void check_limit_block_stats(int expLim, int expCur) {
  int lim = -2;
  int cur = -2;
  prctl(GET_TL_DUMMY, &lim, &cur);
  CU_ASSERT(lim == expLim);
  CU_ASSERT(cur == expCur);
}

/* Test routines */

void* run_test_prctl_limit1(void* ign) {
  prctl(SET_TL_DUMMY, 1);
  check_limit_block_stats(1, 1);
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
  check_limit_block_stats(2, 1);
  int* npid = gen_mp_int(fork());
  check_limit_block_stats(2, 2);
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
  check_limit_block_stats(2, 1);
  pthread_t th1;
  int resTh = pthread_create(&th1, NULL, clone_new_and_ret_pid, NULL);
  int* res;
  if (resTh == 0) {
    pthread_join(th1, ((void**)&res));
  }
  check_limit_block_stats(2, 1);
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
  check_limit_block_stats(2, 1);
  pthread_t th1;
  pthread_t th2;
  int resTh1 = pthread_create(&th1, NULL, wait_for_start_and_exit, NULL);
  int resTh2 = pthread_create(&th2, NULL, wait_for_start_and_exit, NULL);
  int* ign2;
  int* rval;
  check_limit_block_stats(2, 2);
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
  check_limit_block_stats(2, 1);
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

void* ch_test_prctl_large_alloc(void* ign) {
  while(!startFlag) { 
    /* spin, we don't really care */
  }
  return NULL;
}

void* run_test_prctl_large_alloc(void* ign) {
  prctl(SET_TL_DUMMY, 12);
  check_limit_block_stats(12, 1);
  int i = 0;
  int lastStat = 0;
  pthread_t threads[12];
  while ((lastStat == 0) && i < 12) {
    lastStat = pthread_create((threads + i), NULL, ch_test_prctl_large_alloc, NULL);
    i++;
  }
  CU_ASSERT(lastStat != 0);
  check_limit_block_stats(12, 12);
  startFlag = 1;
  int* ign;
  for (i = 0; i < 11; i++) {
    pthread_join(threads[i], ((void**)&ign));
  }
  if (lastStat == 0) {
    pthread_join(threads[11], ((void**)&ign));
  }
  check_limit_block_stats(12, 1);
  return NULL;
}

static void test_prctl_limit2_failsi(void) {
  pthread_t th1;
  int resTh = pthread_create(&th1, NULL, run_test_prctl_large_alloc, NULL);
  int* ign;
  CU_ASSERT(resTh == 0);
  if (resTh == 0) {
    pthread_join(th1, ((void**)&ign));
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
