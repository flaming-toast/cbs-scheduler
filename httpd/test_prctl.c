#ifdef PRCTL_TEST
#include "Cunit/Basic.h"
#include <sys/prctl.h>

static int nop_init_suite(void) {
  return 0;
}

static int nop_clean_suite(void) {
  return 0;
}

static void test_prctl_stop_fork(void) {
  prctl(PR_SET_THREADLIMIT, 0);
  int fval = fork();
  CU_ASSERT(fval == -1);
  prctl(PR_SET_THREADLIMIT, 2);
}

static void test_prctl_stop_clone(void) {
  prctl(PR_SET_THREADLIMIT, 0);
  int fval = clone();
  CU_ASSERT(fval == -1);
  prctl(PR_SET_THREADLIMIT, 2);
}

static void test_prctl_reenable(void) {
  prctl(PR_SET_THREADLIMIT, 0);
  prctl(PR_SET_THREADLIMIT, 2);
  unsigned long npid = fork();
  if (npid == 0) {
    exit();
  }
  CU_ASSERT(npid != 0)
}


int main(int argc, char **argv)
{
    if (CUE_SUCCESS != CU_initialize_registry())
        return CU_get_error();

    {
        CU_pSuite prctlSuite = CU_add_suite("prctl tests",
                                   &nop_init_suite,
                                   &nop_clean_suite);

        CU_add_test(prctlSuite, "Test of fork() disabling with prctl()", &test_prctl_stop_fork);
	CU_add_test(prctlSuite, "Test of clone() disabling with prctl()", &test_prctl_stop_clone);
	CU_add_test(prctlSuite, "Test of fork() reenabling after disabling", &test_prctl_reenable);
    }

    /* Actually run your tests here. */
    CU_basic_set_mode(CU_BRM_VERBOSE);
    CU_basic_run_tests();
    CU_cleanup_registry();
    return CU_get_error();
}
#endif
