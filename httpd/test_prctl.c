#include "Cunit/Basic.h"
#include <sys/prctl.h>

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
  if (fork()) {
    
  }
}
