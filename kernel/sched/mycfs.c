/*
 *  kernel/sched/mycfs.c
 */

#include "sched.h"
#include <include/linux/rb-tree.h>

/*
 * mycfs-task scheduling class.
 *
 * (NOTE: these are not related to SCHED_mycfs tasks which are
 *  handled in sched_fair.c)
 */

#ifdef CONFIG_SMP
static int select_task_rq_mycfs(struct task_struct *p, int sd_flag, int flags)
{
	return task_cpu(p); /* mycfs tasks as never migrated */
}
#endif /* CONFIG_SMP */
/*
 * mycfs tasks are unconditionally rescheduled:
 */
static void check_preempt_curr_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	resched_task(rq->mycfs);
}

static struct sched_entity *pick_next_task_mycfs(struct rq *rq)
{   
    struct rb_node *left = rq->leftmost;
    if (!left) {
        return 0:
    }

    return rb_entry(left, struct  sched_entity, run_node);

    /*
	schedstat_inc(rq, sched_gomycfs);
	calc_load_account_mycfs(rq);
	return rq->mycfs;
    */
}

static void enqueue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{


}

/*
 * It is not legal to sleep in the mycfs task - print a warning
 * message if some code attempts to do it:
 */
static void dequeue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	// raw_spin_unlock_irq(&rq->lock);
	// printk(KERN_ERR "bad: scheduling from the mycfs thread!\n");
	// dump_stack();
	// raw_spin_lock_irq(&rq->lock);
}

static void put_prev_task_mycfs(struct rq *rq, struct task_struct *prev)
{
}

static void task_tick_mycfs(struct rq *rq, struct task_struct *curr, int queued)
{
}

static void set_curr_task_mycfs(struct rq *rq)
{
}

static void switched_to_mycfs(struct rq *rq, struct task_struct *p)
{
	
}

static void prio_changed_mycfs(struct rq *rq, struct task_struct *p, int oldprio)
{
	
}

static unsigned int get_rr_interval_mycfs(struct rq *rq, struct task_struct *task)
{
	return 0;
}

const struct sched_class mycfs_sched_class = {
	.enqueue_task		= enqueue_task_mycfs,
	.dequeue_task 		= dequeue_task_mycfs,
	.check_preempt_curr = check_preempt_curr_mycfs,
	.pick_next_task		= pick_next_task_mycfs,
	.put_prev_task		= put_prev_task_mycfs,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_mycfs,
#endif

	.set_curr_task      = set_curr_task_mycfs,
	.task_tick			= task_tick_mycfs,

	.get_rr_interval	= get_rr_interval_mycfs,

	.prio_changed		= prio_changed_mycfs,
	.switched_to		= switched_to_mycfs,

};
