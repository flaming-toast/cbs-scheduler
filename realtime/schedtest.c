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
unsigned long test1[t1_procs][3] = {
	{0, 1, 50000000000}, 
	{1, 5, 45000},
	{1, 1, 60000}
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
	while((int *)keep_running);
	return 1;
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

	printf("Testing run queue deadlines...\n");
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

		printf("Creating new CBS process...\n");
		if((ret = cbs_create(&cbs_ts[i], mode, cpu, &period, &entry, &keep_running)) != 0)
		{
			printf("Failed to create process %d with ret val %d\n", i, ret);
			keep_running = 0;
		} else
		{
			ct1[i].pid = ((struct cbs_task *)cbs_ts+i)->pid;
			ct1[i].utilization = cpu/period.tv_usec;
			ct1[i].cpu = cpu;
			ct1[i].period_usec = period.tv_usec;
			printf("Spawned process with PID: %d\n", ct1[i].pid);
		}

	}

	signal(SIGINT, handle_interrupt);
	while(keep_running)
	{
		//TODO insert snapshot syscall here, calculate snap_id
		enum snap_event *ev = malloc(sizeof(enum snap_event) * 8);
		enum snap_trig *tr = malloc(sizeof(enum snap_trig) * 8);
		for(i = 0; i < 8; i++)
		{
		        ev[i] = SNAP_EVENT_CBS_SCHED;
		        tr[i] = SNAP_TRIG_AEDGE;
		}


		syscall(314, ev, tr, 8);
		snap_id = 0;

		test_rq(snap_id);
		test_utilization(snap_id);
		sleep(5);
		keep_running = 0;
	}

/*
	for(i = 0; i < t1_procs; i++)
	{
	       cbs_join(cbs_ts[i], NULL);
	}

	free(cbs_ts);
	free(ct1);
	*/
}
