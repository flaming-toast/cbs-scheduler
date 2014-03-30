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

#include "sched.h"

#define SNAP_MAX_ENTRIES 5

const struct sched_class cbs_sched_class;
//static const struct file_operations cbs_snapshot_fops;

static void enqueue_task_cbs(struct rq *rq, struct task_struct *p, int flags)
{
}

static void dequeue_task_cbs (struct rq *rq, struct task_struct *p, int flags)
{
}

static void yield_task_cbs(struct rq *rq){
}

static bool yield_to_task_cbs(struct rq *rq, struct task_struct *p, bool preempt){
	return false;
}
                      
static void check_preempt_wakeup(struct rq *rq, struct task_struct *p, int wake_flags){
}
                      
static struct task_struct *pick_next_task_cbs(struct rq *rq){
	struct task_struct *p = NULL;
	return p;
}
static void put_prev_task_cbs(struct rq *rq, struct task_struct *prev)
{

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
}
static void task_tick_cbs(struct rq *rq, struct task_struct *curr, int queued)
{
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

__init void init_sched_cbs_class(void)
{
	printk("init_sched_cbs_class");
	int i;
	char entry[1];
	/* populate proc directory */
	struct proc_dir_entry *parent = proc_mkdir("snapshot", NULL);
	for (i = 0; i < SNAP_MAX_ENTRIES; i++) {
		sprintf(entry, "%d", i);
//		proc_create(entry, S_IRUGO|S_IWUGO, parent, &cbs_snapshot_fops);
		proc_create(entry, S_IRUGO|S_IWUGO, parent, NULL);
	}
	
}
int read_proc(struct file *f, char __user *u, size_t i, loff_t *t)
{
	return 0;
}

/*
static const struct file_operations cbs_snapshot_fops = {
	.read = read_proc,
	//.write = write_proc
};
*/
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

	.check_preempt_curr	= check_preempt_wakeup,

	.pick_next_task		= pick_next_task_cbs,
	.put_prev_task		= put_prev_task_cbs,
/*
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

