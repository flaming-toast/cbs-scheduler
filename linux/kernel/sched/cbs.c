/* CBS Scheduler */

#include <linux/latencytop.h>
#include <linux/sched.h>
#include <linux/cpumask.h>
#include <linux/slab.h>
#include <linux/profile.h>
#include <linux/interrupt.h>
#include <linux/mempolicy.h>
#include <linux/migrate.h>
#include <linux/task_work.h>
#include <linux/proc_fs.h>

#include <trace/events/sched.h>

#include <linux/gcd.h>

#include "sched.h"
#include "cbs_snapshot.h"

/* 24.8 fixed point arithmetic for utilization calculations */
#define div_fp(x,y) ( ((u64)(x) << 8) / (y) )
#define mul_fp(x,y) ( ((u64)(x) * (u64)(y)) >> 8 )
#define int_to_fp(x) ( (u32)(x) << 8 )
#define fp_to_int(x) ((x) >> 8 )


static inline struct task_struct *task_of(struct sched_cbs_entity *se)
{
	return container_of(se, struct task_struct, cbs_se);
}

static inline int entity_before(struct sched_cbs_entity *a,
				struct sched_cbs_entity *b)
{
	return (s64)(a->deadline - b->deadline) < 0;
}

static inline int entity_after(struct sched_cbs_entity *a,
				struct sched_cbs_entity *b)
{
	return (s64)(a->deadline - b->deadline) > 0;
}

/* Total utilization; sum of bandwidth ratios Q_s/T_s = U_s of all CBS servers
 * and hard RT tasks on the cbs runqueue. If U_1 + U_2 + ... + U_i <= 1, 
 * the set is schedulable by EDF.
 *
 * This should be updated when tasks are enqueued or dequeued, and checked when 
 * a new task is being enqueued. If the schedulability test fails, we will 
 * not enqueue the task.
 *
 * Since we cannot perform floating point arithmetic, a work-around would be 
 * to keep track of the running total of cpu_budget requested and period, 
 * and make sure that total_budget <= total_period 
 */

/* Update current task's runtime stats 
 */
static void update_curr(struct cbs_rq *cbs_rq) 
{
	struct sched_cbs_entity *curr = cbs_rq->curr;
	/* update budget and deadline etc */
	/* No need for a deadline counter, just check the value of jiffies against the deadline */
	curr->current_budget--;
	/* Do we need to erase and reinsert into the tree to rebalance? */
}


const struct sched_class cbs_sched_class;
//static const struct file_operations cbs_snapshot_fops;

static void enqueue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
	/* What to do about flags? see fair.c */
	struct cbs_rq *cbs_rq;
	struct sched_cbs_entity *cbs_se = &p->cbs_se;
	cbs_rq = &rq->cbs;

	/* bandwidth represented as a 24.8 fp value */
	unsigned long utilization = div_fp(int_to_fp(cbs_se->cpu_budget), int_to_fp(cbs_se->period));


	/* Schedulability test, check if sum of ratios >= 1 */
	if (utilization + cbs_rq->total_sched_cbs_utilization > int_to_fp(1))  //0x100
		return;
	else
		cbs_rq->total_sched_cbs_utilization += utilization;
	
	/* Recalculate slack task bandwidth (1 - total_sched_cbs_utilization)*/
	/* Instead of dynamically recalculating this each time...might just be 
	 * a good idea to set aside a percentage -_-
	 */
	cbs_rq->total_sched_cbs_periods += cbs_se->period;
	cbs_rq->slack_se->cpu_budget = mul_fp((int_to_fp(1) - cbs_rq->total_sched_cbs_utilization), cbs_rq->total_sched_cbs_periods);
	cbs_rq->slack_se->period = cbs_rq->total_sched_cbs_periods;

	/* if c > (d - r)U
	 * recharge deadline to period,
	 * recharge budget to max budget
	 * else use current values
	 * jiffies represents time since system booted in ticks
	 */
	signed long temp = (cbs_se->deadline - jiffies)*cbs_se->cpu_budget;
	signed long temp2 = cbs_se->current_budget*cbs_se->period;
	int temp3 = (temp2 >= temp);
//	if ((cbs_se->current_budget*cbs_se->period) >= (signed long)((cbs_se->deadline - jiffies)*cbs_se->cpu_budget)) {
	if (temp3) {
		/* Refresh deadline = period */
		/* Initially this'd be (0+period), then every other time it'd be prev_deadline + period */
		cbs_se->deadline = jiffies + cbs_se->period; // jiffies is arrival time
		/* Recharge budget  */
		cbs_se->current_budget = cbs_se->cpu_budget;
	}

	insert_cbs_rq(cbs_rq, cbs_se, 0);

	cbs_rq->nr_running++;
}

/* Take off runqueue because task is blocking/sleeping/terminating etc */
static void dequeue_task_cbs (struct rq *rq, struct task_struct *p, int flags)
{
	/* cfs calls update_curr here, not sure if we should */
	struct cbs_rq *cbs_rq;
	struct sched_cbs_entity *cbs_se = &p->cbs_se;
	cbs_rq = &rq->cbs;

	if (cbs_rq->leftmost == &cbs_se->run_node) {
		struct rb_node *next_node;

		next_node = rb_next(&cbs_se->run_node);
		cbs_rq->leftmost = next_node;
	}

	rb_erase(&cbs_se->run_node, &cbs_rq->deadlines);
	cbs_se->on_rq = 0;

	cbs_rq->total_sched_cbs_utilization -= div_fp(int_to_fp(cbs_se->cpu_budget), int_to_fp(cbs_se->period));
	/* should update slack cbs budget to reflect bandwidth ratio */

	cbs_rq->nr_running--;
}

static void yield_task_cbs(struct rq *rq)
{
}

static bool yield_to_task_cbs(struct rq *rq, struct task_struct *p, bool preempt)
{
	return false;
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
/* "To drive preemption between tasks, the scheduler sets the flag in timer 
 * interrupt handler scheduler_tick()" -- core.c
 * if the flag is set, __schedule() runs the needreschedule
 * goto, which calls pick_next_task, clears the flag, 
 * calls context_switch with the newly picked task set.
 */
static void
check_preempt_tick(struct cbs_rq *cbs_rq, struct sched_cbs_entity *curr) 
{
	int rebalance = 0;
	if (jiffies >= curr->deadline) { // > or >=?
		/* Our deadline is here but 
		 * we still have budget, we 
		 * couldn't meet our deadline
		 */
		if (curr->current_budget > 0) { // still have unrun budget
			/* SIGXCPU? */
			struct task_struct *p = task_of(curr);
			sigaddset(&p->pending.signal, SIGXCPU);
			set_tsk_thread_flag(p, TIF_SIGPENDING);

			/* Sets TIF_NEEDS_RESCHED flag,
		 	 * test_tsk_need_resched will return true
		 	 * for this task
		 	 */
			resched_task(task_of(cbs_rq->curr));
			return;
		}
		/* If we are at the deadline and our budget is <=0, fall through
		 * to the next case...
		 */
	}

	/* 
	 * check if we have exhausted our budget, 
	 * do a refresh
	 */
	if (curr->current_budget <= 0) {
		curr->current_budget = curr->cpu_budget;
		curr->deadline = curr->deadline + curr->period; // our previous deadline + period

		/* Deadline changed, re-sort the tree */
		rebalance = 1;

	}

	if (rebalance) {
		insert_cbs_rq(cbs_rq, curr, 1);
	}
	/* After rebalancing, check if there is an earlier 
	 * deadline in the tree, if so
	 * call resched_task 
	 */
	if (container_of(cbs_rq->leftmost, struct sched_cbs_entity, run_node) != curr) {
		/* Fall throught the scheduler classes in pick_next_tasfdsf,
		 * cbs will be first.
		 *
		 * If a task has been newly enqueued/awoken, and it has an earlier 
		 * deadline than the slack task, then it will get to run on the
		 * next pick_next_task call
		 */
		resched_task(task_of(cbs_rq->curr));
	}

	/* If there curr did not exhaust its budget and deadline is still in the future (jiffies < deadline)
	 * then this function does nothing
	 */
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_curr_cbs(struct rq *rq, struct task_struct *p, int wake_flags)
{
	struct task_struct *curr = rq->curr;
	struct sched_cbs_entity *curr_cbs_se = &curr->cbs_se, *pse = &p->cbs_se;

	/* Idle tasks are by definition preempted by non-idle tasks. */
	if (unlikely(curr->policy == SCHED_IDLE) &&
	    likely(p->policy != SCHED_IDLE))
		goto preempt;

	/*
	 * Batch and idle tasks do not preempt non-idle tasks (their preemption
	 * is driven by the tick):
	 */
	if ((unlikely(p->policy != SCHED_CBS_BW) && unlikely(p->policy != SCHED_CBS_RT)) || !sched_feat(WAKEUP_PREEMPTION))
		return;

	/* CFS calls find_matching_se, which walks up the parent tree
	 * does CBS care about child tasks? 
	 */

	if (entity_before(pse, curr_cbs_se))
		goto preempt;
preempt:
	/* resched_task will set the TIF_NEED_RESCHED flag which will be 
	 */
	resched_task(curr);
}
                      
static struct task_struct *pick_next_task_cbs(struct rq *rq){
	/* defer to next scheduler in the chain for now */
	/*
	struct task_struct *p = NULL;
	return p;
	*/

	struct task_struct *p;
	struct cbs_rq *cbs_rq = &rq->cbs;
	struct sched_cbs_entity *cbs_se;
	struct rb_node *left;

	if (!cbs_rq->nr_running)
		return NULL;

	left = cbs_rq->leftmost;
	if (!left) // empty tree
		return NULL;
	cbs_se  = rb_entry(left, struct sched_cbs_entity, run_node);
	cbs_rq->curr = cbs_se;
	if (cbs_se->is_slack) {
//		printk("Slack task, deferring to CFS");
		return NULL;
	} else {
		p = task_of(cbs_se);
		return p;
	}
}

static void put_prev_task_cbs(struct rq *rq, struct task_struct *prev)
{
	struct sched_cbs_entity *cbs_se = &prev->cbs_se;
	struct cbs_rq *cbs_rq = &rq->cbs;
	/* cfs has this extra update_curr call but I don't think
	 * we need it.
	if (prev->on_rq)
		update_curr(cbs_rq);
	*/
	cbs_rq->curr = NULL;
}
 /*                                           
static int
select_task_rq_cbs(struct task_struct *p, int sd_flag, int wake_flags)
{
	return 0;
}

migrate_task_rq_cbs,
                      
rq_online_cbs,
rq_offline_cbs,
                      
task_waking_cbs,
*/                      
                      
static void set_curr_task_cbs(struct rq *rq)
{
	struct sched_cbs_entity *cbs_se = &rq->curr->cbs_se;
	struct cbs_rq *cbs_rq = &rq->cbs;

	/* CFS doesn't keep their current task in the tree, 
	 * it dequeues the task from the rq, not sure if
	 * we should follow similar approach
	 */
	cbs_rq->curr = cbs_se;
}

static void
entity_tick(struct cbs_rq *cbs_rq, struct sched_cbs_entity *curr, int queued)
{

	/* If we do need to re-sort the tree for any reason, it
	 * should be done in update_curr...
	 *
	 * Need to re-sort when we do a deadline / budget refresh 
	 * 
	 */
	update_curr(cbs_rq); //if this hits zero...check_preempt_tick should catch that.
	/* If the tree has been resorted, we check if there is a new task
	 * with an earlier deadline. If so, preempt
	 */
	check_preempt_tick(cbs_rq, curr);
}

static void task_tick_cbs(struct rq *rq, struct task_struct *curr, int queued)
{
	
	struct cbs_rq *cbs_rq = &rq->cbs;
	struct sched_cbs_entity *cbs_se = &curr->cbs_se;

	/* CFS walks up the parent tree and calls entity_tick for each parent se,
	 * how do we handle children tasks? 
	 */
	entity_tick(cbs_rq, cbs_se, queued);
}


static void task_fork_cbs(struct task_struct *p)
{
}

/*
 * Priority of the task has changed. Check to see if we preempt
 * the current task.
 */
static void
prio_changed_cbs(struct rq *rq, struct task_struct *p, int oldprio)
{
}
                      
static void switched_from_cbs(struct rq *rq, struct task_struct *p)
{
}

/*
 * We switched to the sched_fair class.
 */

static void switched_to_cbs(struct rq *rq, struct task_struct *p)
{
}
                      
static unsigned int get_rr_interval_cbs(struct rq *rq, struct task_struct *task)
{
	return 0;
}

void init_cbs_rq(struct cbs_rq *rq) {
	rq->deadlines = RB_ROOT;
}

__init void init_sched_cbs_class(struct rq *rq)
{
	/* Construct a virtual cbs slack task that will always
	 * remain on the cbs runqueue. All this task does
	 * is defer to CFS 
	 */
	struct cbs_rq *cbs_rq = &rq->cbs;

	struct sched_cbs_entity *cbs_slack_se = kzalloc(sizeof(struct sched_cbs_entity), GFP_KERNEL);
	/* Should the period be the longest period in the runqueue? And thus deadline = period? */
	/* bandwidth of slack cbs = 1 - total_sched_utilization */

	/* When we start out slack gets 100% utilization as there are no tasks in the run queue yet */
	/* These values are quite arbitrary, I'm not sure what to start deadline_ticks_left */
	cbs_slack_se->period = 10;
	cbs_slack_se->cpu_budget = cbs_slack_se->current_budget = cbs_slack_se->period;
	cbs_slack_se->deadline = jiffies + cbs_slack_se->period;

	cbs_slack_se->is_slack = 1;
	cbs_rq->slack_se = cbs_slack_se;
	cbs_rq->total_sched_cbs_periods = cbs_slack_se->period;
	cbs_rq->total_sched_cbs_utilization = int_to_fp(0); // make sure to do everything in fp arithmetic
	cbs_rq->nr_running++;

	/* link this se into the rbtree */
	insert_cbs_rq(cbs_rq, cbs_slack_se, 0);

}
int read_proc(struct file *f, char __user *u, size_t i, loff_t *t)
{
	return 0;
}

static const struct file_operations cbs_snapshot_fops = {
	.read = read_proc,
	//.write = write_proc
};

static int __init  init_sched_cbs_procfs(void) {

	struct proc_dir_entry *parent = proc_mkdir("snapshot", NULL);

	int i;
	char entry[1];

	for (i = 0; i < SNAP_MAX_TRIGGERS; i++) {
		sprintf(entry, "%d", i);
		proc_create(entry, S_IRUGO|S_IWUGO, parent, &cbs_snapshot_fops);
	}
	return 0;
}

__initcall(init_sched_cbs_procfs);




/*
 * Pilfered from fair.c and s/fair/cbs/
 * removed GROUP_SCHED ifdef
 */
const struct sched_class cbs_sched_class = {
	.next			= &rt_sched_class, // enforce scheduler class hierarchy
	.enqueue_task		= enqueue_task_cbs,
	.dequeue_task		= dequeue_task_cbs,
	.yield_task		= yield_task_cbs,
	.yield_to_task		= yield_to_task_cbs,

	.check_preempt_curr	= check_preempt_curr_cbs,

	.pick_next_task		= pick_next_task_cbs,
	.put_prev_task		= put_prev_task_cbs,

/* Not responsible for load-balancing migration operations.
#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_cbs,
	.migrate_task_rq	= migrate_task_rq_cbs,

	.rq_online		= rq_online_cbs,
	.rq_offline		= rq_offline_cbs,

	.task_waking		= task_waking_cbs,
#endif
*/

	.set_curr_task          = set_curr_task_cbs,
	.task_tick		= task_tick_cbs,
	.task_fork		= task_fork_cbs,

	.prio_changed		= prio_changed_cbs,
	.switched_from		= switched_from_cbs,
	.switched_to		= switched_to_cbs,

	.get_rr_interval	= get_rr_interval_cbs,

};

void insert_cbs_rq(struct cbs_rq *cbs_rq, struct sched_cbs_entity *insert, int rebalance) 
{
	if (rebalance)  // would rebalance after deadline refresh
		rb_erase(&insert->run_node, &cbs_rq->deadlines);
	
	struct rb_node **link;
	struct rb_node *parent = NULL;
	struct sched_cbs_entity *entry;
	int leftmost = 1;

	link = &cbs_rq->deadlines.rb_node;

	while (*link) {
		parent = *link;
		entry = rb_entry(parent, struct sched_cbs_entity, run_node);
		if (entity_before(insert, entry)) {
			link = &parent->rb_left;
		} else if (entity_after(insert, entry)) {
			link = &parent->rb_right;
			leftmost = 0;
		} else {
			return; // don't change the tree
		}
	}

	rb_link_node(&insert->run_node, parent, link);
	rb_insert_color(&insert->run_node, &cbs_rq->deadlines);

	if (leftmost) {
		cbs_rq->leftmost = &insert->run_node;
	} else {
		struct rb_node *new_left = rb_first(&cbs_rq->deadlines);
		struct rb_node *ptr;
		while (ptr = rb_prev(new_left)) 
			;
		if (ptr != NULL)
			new_left = ptr;
		cbs_rq->leftmost = new_left;
	}
}
