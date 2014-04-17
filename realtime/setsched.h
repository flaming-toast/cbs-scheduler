#define SCHED_CBS_BW 6
#define SCHED_CBS_RT 7

struct cbs_sched_param {
	int sched_priority;
	unsigned int cpu_budget;
	unsigned long period_ns;
};
