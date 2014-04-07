/* cs194-24 Lab 2 */
#define _XOPEN_SOURCE
#define _BSD_SOURCE
#define _POSIX_C_SOURCE 199309L

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <syscall.h>

#include <sched.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "cbs.h"

#ifndef BOGO_MIPS
#warning "BOGO_MIPS isn't defined, it should be by the build system"
#endif

#define NANO (1000 * 1000 * 1000)
#define MICRO (1000 * 1000)

#define SCHED_CBS_BW 6
#define SCHED_CBS_RT 7

/* FIXME: This is defined in <bits/sigevent.h>, but it doesn't seem to
 * actually be getting used... does anyone know why? */
#define sigev_notify_thread_id _sigev_un._tid

enum cbs_state
{
    CBS_STATE_SLEEP,
    CBS_STATE_RUNNING,
    CBS_STATE_READY,
    CBS_STATE_DONE,
};


int cbs_create(cbs_t *thread, enum cbs_type type,
	       size_t cpu, struct timeval *period,
               int (*entry)(void *), void *arg) 
{
	if (type != CBS_RT && type != CBS_BW) {
		fprintf(stderr, "Invalid cbs_type");
		exit(1);
	}
	/* Check of period and cpu values are sane, else return EAGAIN */

	pid_t cbs_task;
	if ((cbs_task = fork()) == 0) {
		// do work
		// how to exec entry o.O
		// should we use clone instead so we can pass fn pointer?
		// while return value is CBS_CONTINUE,
		// call the entry function? otherwise, exit
	} else {
		/* Do stuff with the timeval argument period 
		 * and convert to ticks
		 */
		struct sched_param param = {
			.sched_priority = 2, // do numeric priorities even matter?
			// Note: the scheduling policy of a task available in task->policy
			.cpu_budget = cpu, // aka Q_s in the paper; convert to ticks 
			.period = period_ticks // convert to ticks
		};

		switch(type){
			case CBS_RT:
				sched_setscheduler(cbs_task, SCHED_CBS_RT, &param);
				break;
			case CBS_BW:
				sched_setscheduler(cbs_task, SCHED_CBS_BW, &param);
				break;
			default:
				// shouldn't be here..
				break;
		}

	}
}

int cbs_join(cbs_t *thread, int *code)
{
	/* Should just call wait */
	struct cbs_task *t = (struct cbs_task *)thread;
	int status;
	int pid;
	pid = waitpid(t->pid, &status, 0); // we could optionally look at status if we want to...
	return pid; // -1 on error, pid on success
}
