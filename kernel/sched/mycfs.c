/*
 *  kernel/sched/mycfs.c
 */

#include <linux/sched.h>
#include "sched.h"
#include <linux/rbtree.h>

/*
 * mycfs-task scheduling class.
 *
 * (NOTE: these are not related to SCHED_IDLE tasks which are
 *  handled in sched_fair.c)
 */

/*
 * Minimal preemption granularity for CPU-bound tasks:
 * (default: 0.75 msec * (1 + ilog(ncpus)), units: nanoseconds)
 */
unsigned int sysctl_sched_min_granularity = 750000ULL;
/*
 * Targeted preemption latency for CPU-bound tasks:
 * (default: 6ms * (1 + ilog(ncpus)), units: nanoseconds)
 *
 * NOTE: this latency value is not the same as the concept of
 * 'timeslice length' - timeslices in CFS are of variable length
 * and have no persistent notion like in traditional, time-slice
 * based scheduling concepts.
 *
 * (to see the precise effective timeslice length of your workload,
 *  run vmstat and monitor the context-switches (cs) field)
 */
unsigned int sysctl_sched_latency = 6000000ULL;
/*
 * is kept at sysctl_sched_latency / sysctl_sched_min_granularity
 */
static unsigned int sched_nr_latency = 8;


#ifdef CONFIG_SMP
static int select_task_rq_mycfs(struct task_struct *p, int sd_flag, int flags)
{
	return task_cpu(p); /* mycfs tasks as never migrated */
}
#endif /* CONFIG_SMP */


static struct sched_entity *pick_next_task_mycfs(struct rq *rq)
{   
    struct rb_node *left = rq->mycfs.rb_leftmost;
    if (!left) {
        return 0;
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

static inline struct mycfs_rq *task_mycfs_rq(struct task_struct *p)
{
	return p->my_se.mycfs_rq;
}

/* runqueue on which this entity is (to be) queued */
static inline struct mycfs_rq *mycfs_rq_of(struct sched_mycfs_entity *my_se)
{
	return my_se->mycfs_rq;
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

static inline void
__update_curr(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *curr, unsigned long delta_exec)
{
	/*
	 * Note: Since we use equal weight, we can simply use 
	 * original excuted time as virtual runtime.
	*/
	curr->vruntime += delta_exec; 
	update_min_vruntime(mycfs_rq);

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
place_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *my_se, int initial)
{
	
}

static void 
enqueue_mycfs_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *my_se, int flags){
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
	struct mycfs_rq *mycfs_rq = rq_of(mycfs_rq);
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


/*
 * The idea is to set a period in which each task runs once.
 *
 * When there are too many tasks (sysctl_sched_nr_latency) we have to stretch
 * this period because otherwise the slices get too small.
 *
 * p = (nr <= nl) ? l : l*nr/nl
 */
static u64 __sched_period(unsigned long nr_running)
{
	u64 period = sysctl_sched_latency;
	unsigned long nr_latency = sched_nr_latency;

	if (unlikely(nr_running > nr_latency)) {
		period = sysctl_sched_min_granularity;
		period *= nr_running;
	}

	return period;
}

/*
 * We calculate the wall-time slice from the period by taking a part
 * proportional to the weight.
 *
 * s = p*P[w/rw]
 */
static u64 sched_slice(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *my_se)
{
	u64 slice = __sched_period(mycfs_rq->nr_running + !my_se->on_rq);

	// for_each_sched_entity(se) {
	// 	struct load_weight *load;
	// 	struct load_weight lw;

	// 	mycfs_rq = mycfs_rq_of(my_se);
	// 	load = &cfs_rq->load;

		// if (unlikely(!se->on_rq)) {
		// 	lw = cfs_rq->load;

		// 	update_load_add(&lw, se->load.weight);
		// 	load = &lw;
		// }

		//slice = calc_delta_mine(slice, se->load.weight, load);
	// }
	return slice;
}

struct sched_entity *__pick_first_entity(struct mycfs_rq *mycfs_rq)
{
	struct rb_node *left = mycfs_rq->rb_leftmost;

	if (!left)
		return NULL;

	return rb_entry(left, struct sched_mycfs_entity, run_node);
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_tick(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *curr)
{
	unsigned long ideal_runtime, delta_exec;
	struct sched_mycfs_entity *my_se;
	u64 now = rq_of(mycfs_rq)->clock_task;
	s64 delta;

	ideal_runtime = sched_slice(mycfs_rq, curr);
	//delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
	delta_exec = (unsigned long)(now - curr->exec_start); //check whether this works
	if (delta_exec > ideal_runtime) {
		resched_task(rq_of(mycfs_rq)->curr);
		return;
	}

	/*
	 * Ensure that a task that missed wakeup preemption by a
	 * narrow margin doesn't have to wait for a full slice.
	 * This also mitigates buddy induced latencies under load.
	 */
	if (delta_exec < sysctl_sched_min_granularity)
		return;

	my_se = __pick_first_entity(mycfs_rq);
	delta = curr->vruntime - my_se->vruntime;

	if (delta < 0)
		return;

	if (delta > ideal_runtime)
		resched_task(rq_of(mycfs_rq)->curr);
}

static void
entity_tick(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *curr, int queued)
{
	update_curr(mycfs_rq);

	if (mycfs_rq->nr_running > 1)
		check_preempt_tick(mycfs_rq, curr);
}

static void task_tick_mycfs(struct rq *rq, struct task_struct *curr, int queued)
{
	struct sched_entity *my_se = &curr->my_se;
	struct mycfs_rq *mycfs_rq = mycfs_rq_of(my_se); //check whether we can directly use rq-> 

	entity_tick(mycfs_rq, my_se, queued);
}


/*
 * We are picking a new current task - update its stats:
 */
static inline 
void update_stats_curr_start(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *my_se)
{
	/*
	 * We are starting a new run period:
	 */
	my_se->exec_start = rq_of(mycfs_rq)->clock_task;
}

static void set_next_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *my_se)
{
	/* 'current' is not kept within the tree. */
	if (my_se->on_rq) {
		/*
		 * Any task has to be enqueued before it get to execute on
		 * a CPU. So account for the time it spent waiting on the
		 * runqueue.
		 */
		//update_stats_wait_end(cfs_rq, se); //check whether really doesn't need 
		__dequeue_entity(mycfs_rq, my_se);
	}

	update_stats_curr_start(mycfs_rq, my_se);
	mycfs_rq->curr = my_se;

	//se->prev_sum_exec_runtime = se->sum_exec_runtime;
}

static void set_curr_task_mycfs(struct rq *rq)
{
	struct sched_mycfs_entity *my_se = &rq->curr->my_se;
	struct mycfs_rq *mycfs_rq = mycfs_rq_of(my_se); //maybe can change to rq->mycfs_rq

	set_next_entity(mycfs_rq, my_se);
}

/*
 * We switched to the sched_fair class.
 */
static void switched_to_mycfs(struct rq *rq, struct task_struct *p)
{
	if (!p->my_se.on_rq)
		return;

	/*
	 * We were most likely switched from sched_rt, so
	 * kick off the schedule if running, otherwise just see
	 * if we can still preempt the current task.
	 */
	if (rq->curr == p)
		resched_task(rq->curr);
	else
		check_preempt_curr(rq, p, 0);
}

/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_wakeup(struct rq *rq, struct task_struct *p, int wake_flags)
{
	struct task_struct *curr = rq->curr;
	struct sched_mycfs_entity *my_se = &curr->my_se, *pse = &p->my_se;
	struct mycfs_rq *mycfs_rq = task_mycfs_rq(curr);
	int scale = mycfs_rq->nr_running >= sched_nr_latency;
	

	if (unlikely(se == pse))
		return;

	
	if (test_tsk_need_resched(curr))
		return;

	//we can use do-while loop to replace original goto structure
	//do{
	/* Idle tasks are by definition preempted by non-idle tasks. */
	if (unlikely(curr->policy == SCHED_IDLE) && likely(p->policy != SCHED_IDLE)) //Should we use this?
		goto preempt;

	/*
	 * Batch and idle tasks do not preempt non-idle tasks (their preemption
	 * is driven by the tick):
	 */
	if (unlikely(p->policy != SCHED_MYCFS))
		return;

	update_curr(mycfs_rq_of(my_se));
	
	return;
	//}while(0);

preempt:
	resched_task(curr);
	
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
	.check_preempt_curr = check_preempt_wakeup,
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
