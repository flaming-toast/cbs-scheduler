/*
 *
 * Spawns CBS tasks and uses snapshot() and the snapshot
 * interface to ensure the scheduler is picking tasks
 * properly
 *
 */

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <syscall.h>
#include "cbs.h"
#include "cbs_proc.h"

// note: slack task has >= 10% utilization

// type:{0/hard,1/soft} | cpu budget (MIs)| period (usec)
#define t1_procs 3
unsigned long test1[t1_procs][4] = {
	{0, 10000, 1000000,  11111},
	{0, 10000, 2000000,  111111111},
	{1, 10000, 6000000,  11111111111111},
//	{1, 10000, 5000000000,111111111111111111},
//	{1, 10000, 5000000,  111111111},
};

struct cbs_tester
{
        pid_t pid;
	double utilization;
	int cpu;
	int period_usec;
};

static int keep_running = 1;

void handle_interrupt()
{
        keep_running = 0;
	exit(1);
}

static int entry(void *keep_running)
{
	unsigned long i = 0;
	for(; i < 100000000000000000; i++)
	{
		if(i % 1000000 == 0)
		{
		        printf("%d\t%lu\n", getpid(), *((unsigned long *)keep_running));
		}
	}
//	return 1;
	exit(0);
}

int proc_counter;
struct cbs_proc_t *procs;
void fill_buffer(cbs_proc_t cbs, void *args)
{
	((struct cbs_snapshot_task*)procs)[proc_counter] = *((struct cbs_snapshot_task*)cbs);
	proc_counter++;
}

void test_rq(int snap_id)
{
	unsigned long prev = ULONG_MAX;
	unsigned long cur;
	int i;
	procs = malloc(sizeof(cbs_proc_t) * CBS_MAX_HISTORY);
	proc_counter = 0;

	// assumes func gets called from end of queue to beginning

	cbs_list_rq(snap_id, fill_buffer, NULL);

	//printf("Testing run queue deadlines...\n");
	for(i = 0; i < proc_counter; i++)
	{
		cur = ((struct cbs_snapshot_task *)procs)[i].time_len;
		if(cur > prev)
		{
		        printf("PID %lu with deadline %lu incorrectly queued earlier than PID %lu with deadline %lu\n", ((struct cbs_snapshot_task *)procs)[i].pid, cur, ((struct cbs_snapshot_task *)procs)[i-1].pid, prev);
		}
		prev = cur;
	}

	free(procs);
}

void test_utilization(int snap_id)
{
}

int main()
{
	int snap_id;
	int ret;
	int cpu;
	int i;
	enum cbs_type mode;
	struct timeval period;
	cbs_t *cbs_ts = malloc(sizeof(cbs_t) * t1_procs);
	struct cbs_tester *ct1 = malloc(sizeof(struct cbs_tester) * t1_procs);

	for(i = 0; i < t1_procs; i++)
	{
		if(test1[i][0] == 0)
		{
			mode = CBS_RT;
		} else
		{
			mode = CBS_BW;
		}
		cpu = test1[i][1];
		period.tv_usec = test1[i][2];

		//printf("Creating new CBS process...\n");
		if((ret = cbs_create(&cbs_ts[i], mode, cpu, &period, &entry,  (void *)(&test1[i][3]))) != 0)
		{
			printf("Failed to create process %d with ret val %d\n", i, ret);
		} else
		{
			ct1[i].utilization = cpu/period.tv_usec;
			ct1[i].cpu = cpu;
			ct1[i].period_usec = period.tv_usec;
			ct1[i].pid = ((struct cbs_task *)cbs_ts+i)->pid;
			printf("Spawned process with PID: %d\n", ct1[i].pid);
		}

	}

	signal(SIGINT, handle_interrupt);
	snap_event *ev = malloc(sizeof(snap_event) * 8);
	snap_trig *tr = malloc(sizeof(snap_trig) * 8);
	for(i = 0; i < 8; i++)
	{
		ev[i] = SNAP_EVENT_CBS_SCHED;
		tr[i] = 0;
	}

	for(i = 0; i < 4; i++)
	{
		syscall(314, ev, tr, 8);
		snap_id = 0;

		test_rq(snap_id);
		test_utilization(snap_id);
		sleep(2);
	}

	free(ev);
	free(tr);
	free(cbs_ts);
	free(ct1);
}
