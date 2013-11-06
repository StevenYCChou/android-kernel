/*
 *  kernel/sched/mycfs.c
 */
#include <linux/sched.h>
#include "sched.h"

/*
* Minimal preemption granularity for CPU-bound tasks:
* (default: 0.75 msec * (1 + ilog(ncpus)), units: nanoseconds)
*/
unsigned int mycfs_sysctl_sched_min_granularity = 750000ULL;

// default sched latency of a process: 6ms
unsigned int mycfs_sysctl_sched_latency = 6000000ULL;

// if curr->vruntime > se->vruntime for this amount, then this se can preempt curr
unsigned int mycfs_sysctl_sched_wakeup_granularity = 1000000UL;

// if more than sched_nr_latency process is in mycfs scheduler, replace default __sched_period
// static unsigned int sched_nr_latency = 8;

typedef int sd_flag_t;
typedef int flag_t;
typedef int prio_t;

typedef struct mycfs_rq mycfs_rq_t;
typedef struct sched_mycfs_entity sched_mycfs_entity_t;
typedef struct rq rq_t;
typedef struct task_struct task_struct_t;
typedef struct sched_class sched_class_t;

void init_mycfs_rq(mycfs_rq_t *);

static void check_preempt_wakeup_mycfs(rq_t*, task_struct_t*, int);

static void yield_task_mycfs(rq_t*);
static bool yield_to_task_mycfs(rq_t*, task_struct_t*, bool);

static void task_fork_mycfs(task_struct_t*);

static void switched_to_mycfs(rq_t*, task_struct_t*);
static void switched_from_mycfs(rq_t*, task_struct_t*);

/* ==================================================================  */

/* runqueue on which this entity is (to be) queued */
static inline mycfs_rq_t *mycfs_rq_of(sched_mycfs_entity_t *my_se)
{
	return my_se->mycfs_rq;
}

static inline task_struct_t *task_of(sched_mycfs_entity_t *my_se)
{
	return container_of(my_se, task_struct_t, my_se);
}

static inline rq_t *rq_of(mycfs_rq_t *mycfs_rq)
{
	return mycfs_rq->rq;
}

static inline mycfs_rq_t *task_mycfs_rq(task_struct_t *p)
{
	return p->my_se.mycfs_rq;
}



static void check_preempt_wakeup_mycfs(rq_t* rq, task_struct_t* p, int wake_flags){
	printk("*** check_preempt_wakeup_mycfs is called \n");
}

static void yield_task_mycfs(rq_t* rq){
	printk("*** yield_task_mycfs is called \n");
}

static bool yield_to_task_mycfs(rq_t* rq, task_struct_t* p, bool preempt){
	printk("*** yield_to_task_mycfs is called \n");
	return 1;
}

static void task_fork_mycfs(task_struct_t* p){
	printk("*** task_fork_mycfs is called \n");

}

static void switched_to_mycfs(rq_t* rq, task_struct_t* p){
	printk("*** switched_to_mycfs is called \n");

}

static void switched_from_mycfs(rq_t* rq, task_struct_t* p){
	printk("*** switched_from_mycfs is called \n");

}

/*
static inline void
__update_curr(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *curr, unsigned long delta_exec)
{
	
	// Note: Since we use equal weight, we can simply use 
	// original excuted time as virtual runtime.
	
	curr->sum_exec_runtime += delta_exec;

	curr->vruntime += delta_exec; 
	update_min_vruntime(mycfs_rq);

}
*/
/*
static void update_curr(mycfs_rq_t *mycfs_rq)
{
	sched_mycfs_entity_t *curr = mycfs_rq->curr;
	u64 now = rq_of(mycfs_rq)->clock_task;
	unsigned long delta_exec;

	printk("*** update_curr is called.\n");

	if (unlikely(!curr))
		return;

	delta_exec = (unsigned long)(now - curr->exec_start);
	if (!delta_exec)
		return;

	//__update_curr(mycfs_rq, curr, delta_exec);
	curr->exec_start = now;

	printk("*** update_curr is end.\n");

}
*/

/*
 * mycfs-task scheduling class.
 *
 * (NOTE: these are not related to SCHED_IDLE tasks which are
 *  handled in sched_fair.c)
 */

#ifdef CONFIG_SMP
// static int select_task_rq_mycfs(struct task_struct *p, int sd_flag, int flags)
// {
// 	return task_cpu(p); /* mycfs tasks as never migrated */
// }
#endif /* CONFIG_SMP */



static struct task_struct *pick_next_task_mycfs(struct rq *rq)
{
	
	return NULL;
}



static void 
enqueue_mycfs_entity(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *my_se, int flags){
	if (!(flags & ENQUEUE_WAKEUP) || (flags & ENQUEUE_WAKING))
		my_se->vruntime += mycfs_rq->min_vruntime;
		//update_curr(mycfs_rq);



	// if(flags & ENQUEUE_WAKEUP){
	// 	place_entity(mycfs_rq, my_se, 0);
	// }

	// if (my_se != mycfs_rq->curr){
	// 	__enqueue_entity(mycfs_rq, my_se);
	// }
	
	my_se->on_rq = 1;
	printk("*** enqueue_mycfs_entity is called, after update_curr, vruntime: %llu, on_rq: %d \n", 
		my_se->vruntime, (int) my_se->on_rq);
}


static void enqueue_task_mycfs(rq_t *rq, task_struct_t *p, int flags)
{
	
	mycfs_rq_t *mycfs_rq = &(rq->mycfs);
	sched_mycfs_entity_t *my_se = &p -> my_se;
	
	printk("*** Enqueue_task_mycfs is called, pid: %d, nr_running: %d \n",p->pid, (int)mycfs_rq->nr_running);

	enqueue_mycfs_entity(mycfs_rq, my_se, flags);

	mycfs_rq->nr_running++;

	printk("*** Enqueue_task_mycfs is ended, pid: %d, nr_running: %d \n",p->pid, (int)mycfs_rq->nr_running);

}


/*
 * It is not legal to sleep in the mycfs task - print a warning
 * message if some code attempts to do it:
 */
static void dequeue_task_mycfs(struct rq *rq, struct task_struct *p, int flags)
{
	printk("*** Dequeue_task_mycfs is called, pid: %d \n",p->pid);
	// raw_spin_unlock_irq(&rq->lock);
	// printk(KERN_ERR "bad: scheduling from the mycfs thread!\n");
	// dump_stack();
	// raw_spin_lock_irq(&rq->lock);
}


static void put_prev_entity(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *prev)
{
	/*
	 * If still on the runqueue then deactivate_task()
	 * was not called and update_curr() has to be done:
	 */

	printk("*** put_prev_entity is called.\n");
	if (prev->on_rq) {
		printk("*** put_prev_entity on_rq is true.\n");
		//update_curr(mycfs_rq);
		/* Put 'current' back into the tree. */
		//__enqueue_entity(mycfs_rq, prev);
	}
	mycfs_rq->curr = NULL;
	printk("*** put_prev_entity ended.\n");
}

/*
 * Account for a descheduled task:
 */
static void put_prev_task_mycfs(rq_t *rq, task_struct_t *prev)
{
	

	sched_mycfs_entity_t *my_se = &prev->my_se;
	mycfs_rq_t *mycfs_rq = mycfs_rq_of(my_se);

	printk("*** put_prev_task_mycfs is called, pid: %d \n",prev->pid);

	put_prev_entity(mycfs_rq, my_se);

	printk("*** put_prev_task_mycfs is ended, pid: %d \n",prev->pid);
}


static void task_tick_mycfs(struct rq *rq, struct task_struct *curr, int queued)
{
	printk("*** task_tick_mycfs is called \n");
}

static void
set_next_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *my_se)
{
	printk("*** set_next_entity is called \n");
	/*
	// 'current' is not kept within the tree.
	if (my_se->on_rq) {
		__dequeue_entity(mycfs_rq, my_se);
	}

	update_stats_curr_start(mycfs_rq, my_se);
	mycfs_rq->curr = my_se;

	my_se->prev_sum_exec_runtime = my_se->sum_exec_runtime;
	*/
}

static void set_curr_task_mycfs(struct rq *rq)
{
	sched_mycfs_entity_t *my_se = &rq->curr->my_se;
	mycfs_rq_t *mycfs_rq = mycfs_rq_of(my_se); //maybe can change to rq->mycfs_rq

	
	printk("***In set_curr_task_mycfs bf set_next_entity \n");
	
	set_next_entity(mycfs_rq, my_se);
}

static void prio_changed_mycfs(struct rq *rq, struct task_struct *p, int oldprio)
{
	printk("*** prio_changed_mycfs is called \n");
	
}

static unsigned int get_rr_interval_mycfs(struct rq *rq, struct task_struct *task)
{
	printk("*** get_rr_interval_mycfs is called \n");
	return 0;
}

void init_mycfs_rq(struct mycfs_rq *mycfs_rq)
{
	printk("*** init_mycfs_rq is called \n");

	mycfs_rq->tasks_timeline = RB_ROOT;
	mycfs_rq->nr_running = 0;
	mycfs_rq->min_vruntime = (u64)(-(1LL << 20));
}

const struct sched_class mycfs_sched_class = {
	.next				= &idle_sched_class,
	.enqueue_task		= enqueue_task_mycfs,
	.dequeue_task 		= dequeue_task_mycfs,
	.check_preempt_curr = check_preempt_wakeup_mycfs,
	.pick_next_task		= pick_next_task_mycfs,
	.put_prev_task		= put_prev_task_mycfs,

	.yield_task			= yield_task_mycfs,
	.yield_to_task		= yield_to_task_mycfs,

// #ifdef CONFIG_SMP
// 	.select_task_rq		= select_task_rq_mycfs,
// #endif

	.set_curr_task      = set_curr_task_mycfs,
	.task_tick			= task_tick_mycfs,
	.task_fork			= task_fork_mycfs,

	.get_rr_interval	= get_rr_interval_mycfs,

	.prio_changed		= prio_changed_mycfs,
	.switched_to		= switched_to_mycfs,
	.switched_from		= switched_from_mycfs,
};