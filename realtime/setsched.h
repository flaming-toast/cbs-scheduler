#define SCHED_CBS_BW 6
#define SCHED_CBS_RT 7

struct cbs_sched_param {
	int sched_priority;
	int cpu_budget;
	int period;
};
