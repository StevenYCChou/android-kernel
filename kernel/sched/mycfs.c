/*
 *  kernel/sched/mycfs.c
 */

#include "sched.h"
#include <linux/rbtree.h>

/*
 * mycfs-task scheduling class.
 *
 * (NOTE: these are not related to SCHED_IDLE tasks which are
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

static inline struct rq *rq_of(struct mycfs_rq *mycfs_rq)
{
	return mycfs_rq->rq;
}

static inline void
__update_curr(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *curr,
	      unsigned long delta_exec)
{
	unsigned long delta_exec_weighted;

	schedstat_set(curr->statistics.exec_max,
		      max((u64)delta_exec, curr->statistics.exec_max));

	curr->sum_exec_runtime += delta_exec;
	schedstat_add(cfs_rq, exec_clock, delta_exec);
	delta_exec_weighted = calc_delta_fair(delta_exec, curr);

	curr->vruntime += delta_exec_weighted;
	update_min_vruntime(cfs_rq);

#if defined CONFIG_SMP && defined CONFIG_FAIR_GROUP_SCHED
	cfs_rq->load_unacc_exec_time += delta_exec;
#endif
}

static void update_curr(struct mycfs_rq *mycfs_rq)
{
	struct sched_mycfs_entity *curr = mycfs_rq->curr;
	u64 now = rq_of(mycfs_rq)->clock_task;
	unsigned long delta_exec;

	if (unlikely(!curr))
		return;

	delta_exec = (unsigned long)(now - curr->exec_start);
	if (!delta_exec)
		return;

	__update_curr(mycfs_rq, curr, delta_exec);
	curr->exec_start = now;

}

static inline u64 max_vruntime(u64 min_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - min_vruntime);
	if (delta > 0)
		min_vruntime = vruntime;

	return min_vruntime;
}

static inline u64 min_vruntime(u64 min_vruntime, u64 vruntime)
{
	s64 delta = (s64)(vruntime - min_vruntime);
	if (delta < 0)
		min_vruntime = vruntime;

	return min_vruntime;
}

static void update_min_vruntime(struct mycfs_rq *mycfs_rq)
{
	u64 vruntime = mycfs_rq->min_vruntime;

	if (mycfs_rq->curr)
		vruntime = mycfs_rq->curr->vruntime;

	if (mycfs_rq->rb_leftmost) {
		struct sched_mycfs_entity *my_se = rb_entry(mycfs_rq->rb_leftmost, 
											struct sched_mycfs_entity,run_node);

		if (!mycfs_rq->curr)
			vruntime = my_se->vruntime;
		else
			vruntime = min_vruntime(vruntime, my_se->vruntime);
	}

	mycfs_rq->min_vruntime = max_vruntime(mycfs_rq->min_vruntime, vruntime);

}



static inline int entity_before(struct sched_mycfs_entity *a, struct sched_mycfs_entity *b){
	return (s64)(a->vruntime - b->vruntime) < 0;
}

static void __enqueue_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *my_se){
	struct rb_node **link= &mycfs_rq->tasks_timeline.rb_node;
	struct rb_node *parent = NULL;
	struct sched_mycfs_entity *entry;
	int leftmost = 1;

	while (*link) {
		parent = *link;
		entry = rb_entry(parent, struct sched_mycfs_entity, run_node);
		/*
		 * We dont care about collisions. Nodes with
		 * the same key stay together.
		 */
		if (entity_before(my_se, entry)) {
			link = &parent->rb_left;
		} else {
			link = &parent->rb_right;
			leftmost = 0;
		}
	}

	if (leftmost)
		mycfs_rq->rb_leftmost = &my_se->run_node;

	rb_link_node(&my_se->run_node, parent, link);
	rb_insert_color(&my_se->run_node, &mycfs_rq->tasks_timeline);

}

static void 
enque_mycfs_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *my_se, int flags){
	if (!(flags & ENQUEUE_WAKEUP) || (flags & ENQUEUE_WAKING))
		my_se->vruntime += mycfs_rq->min_vruntime;

	update_curr(mycfs_rq);

	if(flags & ENQUEUE_WAKEUP){
		place_entity(mycfs_rq, my_se, 0);
	}

	if (my_se != mycfs_rq->curr){
		__enqueue_entity(mycfs_rq, my_se);
	}
	
	my_se->on_rq = 1;
}

static void enqueue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	struct mycfs_rq *mycfs_rq = rq-> mycfs;
	struct sched_mycfs_entity *my_se = &p -> my_se;

	enqueue_mycfs_entity(mycfs_rq, my_se, flags);

	mycfs_rq->nr_running++;

}

static void __dequeue_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *my_se)
{
	if (mycfs_rq->rb_leftmost == &my_se->run_node) {
		struct rb_node *next_node;

		next_node = rb_next(&my_se->run_node);
		mycfs_rq->rb_leftmost = next_node;
	}

	rb_erase(&my_se->run_node, &mycfs_rq->tasks_timeline);
}

static void
dequeue_mycfs_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *se, int flags)
{
	
	update_curr(cfs_rq);

	if (my_se != mycfs_rq->curr)
		__dequeue_entity(mycfs_rq, my_se);
	my_se->on_rq = 0;
	
	if (!(flags & DEQUEUE_SLEEP))
		my_se->vruntime -= mycfs_rq->min_vruntime;

	update_min_vruntime(cfs_rq);
}

static void dequeue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	struct mycfs_rq *mycfs_rq;
	struct sched_mycfs_entity *my_se = &p->my_se;

	dequeue_entity(mycfs_rq, my_se, flags);
	mycfs_rq->nr_running--;

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

void init_mycfs_rq(struct mycfs_rq *mycfs_rq)
{
	mycfs_rq->tasks_timeline = RB_ROOT;
	mycfs_rq->nr_running = 0;
	mycfs_rq->min_vruntime = (u64)(-(1LL << 20));
}

const struct sched_class mycfs_sched_class = {
	.next				= &idle_sched_class,
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
