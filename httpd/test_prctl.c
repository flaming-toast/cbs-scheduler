#ifdef PRCTL_TEST
#include "CUnit/Basic.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/prctl.h>
#include <sched.h>
#include <unistd.h>
#include <stdlib.h>
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

int wait_for_start_and_exit(void* ign) {
  while(!startFlag) { 
    /* spin, we don't really care */
  }
  return 0;
}

int fork_new_and_ret_pid() {
  int npid = fork();
  if (npid == 0) {
    exit(0);
    return 0;
  }
  return npid;
}

int clone_new_dummy_fn(void* fn) {
  return 0;
}

int clone_new_and_ret_pid(void* fn) {
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CU_ASSERT(1 == 0);
    return 0;
  }
  int npid = clone(clone_new_dummy_fn, c_stack, SIGCHLD, NULL);
  int ign = 0;
  if (npid > 0) {
    wait(&ign);
  }
  free(c_stack);
  return npid;
}

/* Test routines */

int run_test_prctl_limit1(void* ign) {
  prctl(SET_TL_DUMMY, 1);
  int npid = fork();
  if (npid == 0) {
    exit(0);
    return npid;
  }
  return npid;
}

static void test_prctl_limit1_fail(void) {
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CU_ASSERT(1 == 0);
    return;
  }
  int npid = clone(run_test_prctl_limit1, c_stack, SIGCHLD, NULL);
  int res = 0;
  CU_ASSERT(npid > 0);
  if (npid > 0) {
    wait(&res);
    CU_ASSERT(res == -1);
  }   
  free(c_stack);
}

int run_test_prctl_limit2_success(void* ign) {
  prctl(SET_TL_DUMMY, 2);
  int npid = fork();
  if (npid == 0) {
    exit(0);
    return npid;
  }
  return npid;
}

static void test_prctl_limit2_success(void) {
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CU_ASSERT(1 == 0);
    return;
  }
  int npid = clone(run_test_prctl_limit2_success, c_stack, SIGCHLD, NULL);
  int res = 0;
  CU_ASSERT(npid > 0);
  if (npid > 0) {
    wait(&res);
    CU_ASSERT(res > 0);
  }
  free(c_stack);
}

int run_test_prctl_limit2_failch(void* ign) {
  prctl(SET_TL_DUMMY, 2);
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CU_ASSERT(1 == 0);
    return 0;
  }
  int npid = clone(clone_new_and_ret_pid, c_stack, SIGCHLD, NULL);
  int res = 0;
  if (npid > 0) {
    wait(&res);
  }
  free(c_stack);
  return res;
}

static void test_prctl_limit2_failch(void) {
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CU_ASSERT(1 == 0);
    return;
  }
  int npid = clone(run_test_prctl_limit2_failch, c_stack, SIGCHLD, NULL);
  int res = 0;
  CU_ASSERT(npid > 0);
  if (npid > 0) {
    wait(&res);
    CU_ASSERT(res == -1);
  }
  free(c_stack);
}

int run_test_prctl_limit2_failsi(void* ign) {
  prctl(SET_TL_DUMMY, 2);
  void* c_stack1 = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack1 == NULL) {
    CU_ASSERT(1 == 0);
    return 0;
  }
  void* c_stack2 = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack2 == NULL) {
    free(c_stack1);
    CU_ASSERT(1 == 0);
    return 0;
  }
  int pid1 = clone(wait_for_start_and_exit, c_stack1, SIGCHLD, NULL);
  int pid2 = clone(wait_for_start_and_exit, c_stack2, SIGCHLD, NULL);
  int ign2 = 0;
  int rval = 0;
  startFlag = 1;
  if (pid1 > 0) {
    wait(&ign2);
    if (pid2 == -1) {
      rval = -2;
    } else {
      wait(&ign2);
      rval = -1;
    }
  }
  free(c_stack1);
  free(c_stack2);
  return rval;
}

static void test_prctl_limit2_failsi(void) {
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CU_ASSERT(1 == 0);
    return;
  }
  int npid = clone(run_test_prctl_limit2_failsi, c_stack, SIGCHLD, NULL);
  int res = 0;
  CU_ASSERT(npid > 0);
  if (npid > 0) {
    wait(&res);
    CU_ASSERT(res < 0); /* The first task was created successfully */
    CU_ASSERT(res == -2); /* The second task was not created */
  }
  free(c_stack);
}

int main(int argc, char **argv)
{
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    {
        CU_pSuite prctlSuite = CU_add_suite("prctl tests",
                                   &nop_init_suite,
                                   &nop_clean_suite);

        CU_add_test(prctlSuite, "Test of prctl(1) disabling new thread formation", &test_prctl_limit1_fail);
	CU_add_test(prctlSuite, "Test of prctl(2) allowing a child thread to be formed", &test_prctl_limit2_success);
	CU_add_test(prctlSuite, "Test of prctl(2) preventing child thread from forming another child", &test_prctl_limit2_failch);
	CU_add_test(prctlSuite, "Test of prctl(2) preventing a second child of the original thread from being formed", &test_prctl_limit2_failsi);
    }

    /* Actually run your tests here. */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
#endif
