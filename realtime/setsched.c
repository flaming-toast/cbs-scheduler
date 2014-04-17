//#include <linux/sched.h>
#include <sched.h>
#include <unistd.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include "setsched.h"

void sig_handler(int signo) {
	if (signo == SIGXCPU) {
		printf("SIGXCPU signal received");
		exit(0);
	}
}
int main(void) {
	struct cbs_sched_param param = {
		.sched_priority = 2,
		.cpu_budget = 1, // bogomips
		.period_ns = 600000000000 // 1 ms = 1 jiffy; this is 10 seconds
	};
	pid_t pid;
	if ((pid = fork()) == 0) // if child
	{
		if (signal(SIGXCPU, sig_handler) == SIG_ERR) {
			printf("Could not catch SIGXCPU");
		}
		while(1) { // Sleeping kills things because sched_setscheduler will see that you're not on a run queue, (which means you're not current task) and won't put you on the cbs runqueue :|
//			printf("a");
		}
	} else {
		printf("lol: %d", pid);
		int err = sched_setscheduler(pid, SCHED_CBS_BW, (void *)&param);
		if (err) {
		perror("sched_setscheduler");
		}
	}
}
