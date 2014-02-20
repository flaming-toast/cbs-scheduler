#ifdef PRCTL_TEST
#include "Cunit/Basic.h"
#include <sys/prctl.h>

int startFlag = 0;

static int nop_init_suite(void) {
  return 0;
}

static int nop_clean_suite(void) {
  return 0;
}

static void test_prctl_limit1_fail(void) {
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CUASSERT(1 == 0);
    return;
  }
  int npid = clone(run_test_prctl_limit1, c_stack, SIGCHLD, NULL);
  int res = 0;
  CUASSERT(npid > 0);
  if (npid > 0) {
    wait(&res);
    CUASSERT(res == -1);
  }   
  free(c_stack);
}

static void test_prctl_limit2_success(void) {
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CUASSERT(1 == 0);
    return;
  }
  int npid = clone(run_test_prctl_limit2_success, c_stack, SIGCHLD, NULL);
  int res = 0;
  CUASSERT(npid > 0);
  if (npid > 0) {
    wait(&res);
    CUASSERT(res > 0);
  }
  free(c_stack);
}

static void test_prctl_limit2_failch(void) {
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CUASSERT(1 == 0);
    return;
  }
  int npid = clone(run_test_prctl_limit2_failch, c_stack, SIGCHLD, NULL);
  int res = 0;
  CUASSERT(npid > 0);
  if (npid > 0) {
    wait(&res);
    CUASSERT(res == -1);
  }
  free(c_stack);
}

static void test_prctl_limit2_failsi(void) {
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CUASSERT(1 == 0);
    return;
  }
  int npid = clone(run_test_prctl_limit2_failsi, c_stack, SIGCHLD, NULL);
  int res = 0;
  CUASSERT(npid > 0);
  if (npid > 0) {
    wait(&res);
    CUASSERT(res < 0); /* The first task was created successfully */
    CUASSERT(res == -2); /* The second task was not created */
  }
  free(c_stack);
}

int run_test_prctl_limit1(void* ign) {
  prctl(PR_SET_THREADLIMIT, 1);
  int npid = fork();
  if (npid == 0) {
    exit();
    return npid;
  }
  return npid;
}

int run_test_prctl_limit2_success(void* ign) {
  prctl(PR_SET_THREADLIMIT, 2);
  int npid = fork();
  if (npid == 0) {
    exit();
    return npid;
  }
  return npid;
}

int run_test_prctl_limit2_failch(void* ign) {
  prctl(PR_SET_THREADLIMIT, 2);
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CUASSERT(1 == 0);
    return;
  }
  int npid = clone(clone_new_and_ret_pid, c_stack, SIGCHLD, NULL);
  int res = 0;
  if (npid > 0) {
    wait(&res);
  }
  free(c_stack);
  return res;
}

int run_test_prctl_limit2_failsi(void* ign) {
  prctl(PR_SET_THREADLIMIT, 2);
  void* c_stack1 = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack1 == NULL) {
    CUASSERT(1 == 0);
    return;
  }
  void* c_stack2 = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack2 == NULL) {
    free(c_stack1);
    CUASSERT(1 == 0);
    return;
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

int wait_for_start_and_exit(void* ign) {
  while(!startFlag) { 
    /* spin, we don't really care */
  }
  return 0;
}

int clone_new_and_ret_result(void* fn) {
  void* c_stack = malloc(BASIC_STACK_SIZE*sizeof(int));
  if (c_stack == NULL) {
    CUASSERT(1 == 0);
    return;
  }
  int npid = clone((int (*fn)(void *))fn, c_stack, SIGCHLD, NULL);
  int res = 0;
  if (npid > 0) {
    wait(&res);
  }
  free(c_stack);
  return res;
}

int fork_new_and_ret_pid() {
  int npid = fork();
  if (npid == 0) {
    exit();
    return 0;
  }
  return npid;
}

int clone_new_and_ret_pid(void* fn) {
  void* c_stack = malloc(BASIC_STACK_SIZE*sieof(int));
  if (c_stack == NULL) {
    CUASSERT(1 == 0);
    return;
  }
  int npid = clone(clone_new_dummy_fn, c_stack, SIGCHLD, NULL);
  int ign = 0;
  if (npid > 0) {
    wait(&ign);
  }
  free(c_stack);
  return npid;
}

int clone_new_dummy_fn(void* fn) {
  return 0;
}


int main(int argc, char **argv)
{
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    {
        CU_pSuite prctlSuite = CU_add_suite("prctl tests",
                                   &nop_init_suite,
                                   &nop_clean_suite);

        CU_add_test(prctlSuite, "Test of fork() disabling with prctl()", &test_prctl_limit1_fail);
	CU_add_test(prctlSuite, "Test of clone() disabling with prctl()", &test_prctl_limit2_success);
	CU_add_test(prctlSuite, "Test of fork() reenabling after disabling", &test_prctl_limit2_failch);
    }

    /* Actually run your tests here. */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
#endif
