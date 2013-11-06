/*
 *  kernel/sched/mycfs.c
 */

#include <linux/sched.h>
#include "sched.h"
#include <linux/rbtree.h>

/*
 * mycfs-task scheduling class.
 */

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
static unsigned int sched_nr_latency = 8;

typedef int sd_flag_t;
typedef int flag_t;
typedef int prio_t;

typedef struct mycfs_rq mycfs_rq_t;
typedef struct sched_mycfs_entity sched_mycfs_entity_t;
typedef struct rq rq_t;
typedef struct task_struct task_struct_t;
typedef struct sched_class sched_class_t;
/*
 function declaration
*/

/*Initialization */
void init_mycfs_rq(mycfs_rq_t*, rq_t*);

/*task updating related function*/
static void update_curr(mycfs_rq_t*);
static inline void __update_curr(mycfs_rq_t*, sched_mycfs_entity_t*, unsigned long);

/*tick related function*/
static void task_tick_mycfs(rq_t*, task_struct_t*, int);
static void entity_tick(mycfs_rq_t*, sched_mycfs_entity_t*, int);
static void check_preempt_tick(mycfs_rq_t*, sched_mycfs_entity_t*);

/*dequeue related function*/
static void dequeue_task_mycfs(rq_t*, task_struct_t*, flag_t);
static void dequeue_mycfs_entity(mycfs_rq_t*, sched_mycfs_entity_t*, flag_t);
static void __dequeue_entity(mycfs_rq_t*, sched_mycfs_entity_t*);

static void put_prev_task_mycfs(rq_t*, task_struct_t*);
static void put_prev_entity(mycfs_rq_t*, sched_mycfs_entity_t*);

static task_struct_t* pick_next_task_mycfs(rq_t*);
static inline sched_mycfs_entity_t* pick_next_mycfs_entity(mycfs_rq_t*);
static void set_next_entity(mycfs_rq_t*, sched_mycfs_entity_t*);

static void task_fork_mycfs(task_struct_t*);
//called when (1) be forked out (2) wake up by someone
static void place_entity(mycfs_rq_t*, sched_mycfs_entity_t*, int);

static void enqueue_task_mycfs(rq_t*, task_struct_t*, flag_t);
static void enqueue_mycfs_entity(mycfs_rq_t*, sched_mycfs_entity_t*, flag_t);
static void __enqueue_entity(mycfs_rq_t*, sched_mycfs_entity_t*);

static void check_preempt_wakeup_mycfs(rq_t*, task_struct_t*, flag_t);
static int wakeup_preempt_entity(sched_mycfs_entity_t*, sched_mycfs_entity_t*);

/*other function*/
static void set_curr_task_mycfs(rq_t*);
static void switched_to_mycfs(rq_t*, task_struct_t*);
static void yield_task_mycfs(rq_t*);
static bool yield_to_task_mycfs(rq_t*, task_struct_t*, bool);

/*empty function*/
static unsigned int get_rr_interval_mycfs(rq_t*, task_struct_t*);
static void prio_changed_mycfs(rq_t*, task_struct_t*, prio_t);
static void switched_from_mycfs(rq_t*, task_struct_t*);

static inline void update_stats_curr_start(mycfs_rq_t*, sched_mycfs_entity_t*);

/*helper function*/
static inline task_struct_t* task_of(sched_mycfs_entity_t*);
static inline mycfs_rq_t* task_mycfs_rq(task_struct_t*);
static inline rq_t* rq_of(mycfs_rq_t*);


static u64 __sched_period(unsigned long);
static u64 sched_slice(mycfs_rq_t*, sched_mycfs_entity_t*);

//static unsigned long calc_delta(unsigned long delta_exec, unsigned long weight, struct load_weight *lw);

static inline int entity_before(sched_mycfs_entity_t*, sched_mycfs_entity_t*);

static inline u64 max_vruntime(u64 min_vruntime, u64 vruntime);
static inline u64 min_vruntime(u64 min_vruntime, u64 vruntime);

static void update_min_vruntime(mycfs_rq_t*);

//pick up leftmost node in rb-tree
sched_mycfs_entity_t* __pick_first_mycfs_entity(mycfs_rq_t*);


/*function for SMP*/
static int select_task_rq_mycfs(task_struct_t *p, sd_flag_t, flag_t);

void init_mycfs_rq(mycfs_rq_t *mycfs_rq, rq_t* rq)
{
	//printk("*** init_mycfs is call, this is @@@1 try\n");
	printk("***The address of rq in init_mycfs_rq:  %d, %pa\n", (int)rq, rq);
	mycfs_rq->rq = rq;

	printk("***The address mycfs_rq->rq in init_mycfs_rq:  %d, %pa\n", (int)mycfs_rq->rq, mycfs_rq->rq);
	mycfs_rq->tasks_timeline = RB_ROOT;
	mycfs_rq->nr_running = 0;
	mycfs_rq->min_vruntime = (u64)(-(1LL << 20));
}

static inline task_struct_t *task_of(sched_mycfs_entity_t *my_se)
{
	return container_of(my_se, task_struct_t, my_se);
}

static inline rq_t *rq_of(mycfs_rq_t *mycfs_rq)
{
	//printk("***rq_of_is called.\n");
	
	
	//printk("***The address of rq in rq_of:  %d, %pa\n", (int)container_of(mycfs_rq, rq_t, mycfs), container_of(mycfs_rq, rq_t, mycfs));
	//printk("***The address of rq in rq_of(mycfs):  %d, %pa\n", (int)mycfs_rq->rq, mycfs_rq->rq);
	return mycfs_rq->rq;
	//return container_of(mycfs_rq, rq_t, mycfs);
}

static inline mycfs_rq_t *task_mycfs_rq(task_struct_t *p)
{
	return p->my_se.mycfs_rq;
}



/*
static inline sched_mycfs_entity_t* pick_next_entity(mycfs_rq_t* mycfs_rq){
	return __pick_first_mycfs_entity(mycfs_rq);
}

static task_struct_t *pick_next_task_mycfs(rq_t *rq)
{   
	
	struct rb_node *left = rq->mycfs.rb_leftmost;
	mycfs_rq_t* mycfs_rq = &(rq->mycfs);
	sched_mycfs_entity_t* my_se;

	if (!mycfs_rq->nr_running)
		return NULL;

    
    if (!left) {
        return NULL;
	}

    my_se = pick_next_entity(mycfs_rq);
    set_next_entity(mycfs_rq, my_se);

    return task_of(my_se);
}
*/
/*
static sched_mycfs_entity_t *__pick_next_entity(sched_mycfs_entity_t *my_se)
{
	struct rb_node *next = rb_next(&my_se->run_node);

	if (!next)
		return NULL;

	return rb_entry(next, sched_mycfs_entity_t, run_node);
}
*/

sched_mycfs_entity_t *__pick_first_mycfs_entity(struct mycfs_rq *mycfs_rq)
{
	struct rb_node *left = mycfs_rq->rb_leftmost;

	if (!left)
		return NULL;

	return rb_entry(left, sched_mycfs_entity_t, run_node);
}

static sched_mycfs_entity_t *pick_next_mycfs_entity(struct mycfs_rq *mycfs_rq)
{
	sched_mycfs_entity_t *my_se = __pick_first_mycfs_entity(mycfs_rq);

	return my_se;
}

static struct task_struct *pick_next_task_mycfs(struct rq *rq)
{
	struct task_struct *p;
	struct mycfs_rq *mycfs_rq = &rq->mycfs;
	sched_mycfs_entity_t *my_se;

	if (!mycfs_rq->nr_running)
		return NULL;

	my_se = pick_next_mycfs_entity(mycfs_rq);
	set_next_entity(mycfs_rq, my_se);

	p = task_of(my_se);
	

	return p;
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

static void update_min_vruntime(mycfs_rq_t *mycfs_rq)
{
	u64 vruntime = mycfs_rq->min_vruntime;

	if (mycfs_rq->curr)
		vruntime = mycfs_rq->curr->vruntime;

	if (mycfs_rq->rb_leftmost) {
		sched_mycfs_entity_t *my_se = rb_entry(mycfs_rq->rb_leftmost, 
											sched_mycfs_entity_t,run_node);

		if (!mycfs_rq->curr)
			vruntime = my_se->vruntime;
		else
			vruntime = min_vruntime(vruntime, my_se->vruntime);
	}

	mycfs_rq->min_vruntime = max_vruntime(mycfs_rq->min_vruntime, vruntime);

}
/*
static inline unsigned long calc_delta_mine(unsigned long delta_exec)
{
	return min(delta_exec, ULONG_MAX);
}
*/

static inline void
__update_curr(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *curr, unsigned long delta_exec)
{
	/*
	 * Note: Since we use equal weight, we can simply use 
	 * original excuted time as virtual runtime.
	*/
	curr->sum_exec_runtime += delta_exec;

	curr->vruntime += delta_exec; 
	update_min_vruntime(mycfs_rq);

}

static void update_curr(mycfs_rq_t *mycfs_rq)
{
	sched_mycfs_entity_t *curr = mycfs_rq->curr;
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

static inline int entity_before(sched_mycfs_entity_t *a, sched_mycfs_entity_t *b){
	return (s64)(a->vruntime - b->vruntime) < 0;
}

static void __enqueue_entity(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *my_se){
	struct rb_node **link= &mycfs_rq->tasks_timeline.rb_node;
	struct rb_node *parent = NULL;
	sched_mycfs_entity_t *entry;
	int leftmost = 1;

	while (*link) {
		parent = *link;
		entry = rb_entry(parent, sched_mycfs_entity_t, run_node);
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
place_entity(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *my_se, int initial)
{
	u64 vruntime = mycfs_rq->min_vruntime;
	printk("*** place_entity() is called.\n");

	/* sleeps up to a single latency don't count. */
	if (!initial) {
		unsigned long thresh = mycfs_sysctl_sched_latency;

		/*
		 * Halve their sleep time's effect, to allow
		 * for a gentler effect of sleepers:
		 */
		if (sched_feat(GENTLE_FAIR_SLEEPERS))
			thresh >>= 1;

		vruntime -= thresh;
	}

	/* ensure we never gain time by being placed backwards. */
	vruntime = max_vruntime(my_se->vruntime, vruntime);

	my_se->vruntime = vruntime;
}

static void 
enqueue_mycfs_entity(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *my_se, int flags){
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

static void enqueue_task_mycfs(rq_t *rq, task_struct_t *p, int flags)
{
	
	mycfs_rq_t *mycfs_rq = &(rq->mycfs);
	sched_mycfs_entity_t *my_se = &p -> my_se;
	printk("***In enqueue_task_mycfs is called. \n");

	enqueue_mycfs_entity(mycfs_rq, my_se, flags);

	printk("***In enqueue_task_mycfs(), before nr++: mycfs_rq = %pa, p = %pa is enqueued, mycfs_rq->nr_running = %lu.\n",mycfs_rq, p, mycfs_rq->nr_running);

	mycfs_rq->nr_running++;

	printk("***In enqueue_task_mycfs(), after nr++: mycfs_rq = %pa, p = %pa is enqueued, mycfs_rq->nr_running = %lu.\n",mycfs_rq, p, mycfs_rq->nr_running);
	printk("***In enqueue_task_mycfs is end. \n");

}

static void __dequeue_entity(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *my_se)
{
	if (mycfs_rq->rb_leftmost == &my_se->run_node) {
		struct rb_node *next_node;

		next_node = rb_next(&my_se->run_node);
		mycfs_rq->rb_leftmost = next_node;
	}
	rb_erase(&my_se->run_node, &mycfs_rq->tasks_timeline);
}

static void
dequeue_mycfs_entity(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *my_se, int flags)
{
	
	update_curr(mycfs_rq);

	if (my_se != mycfs_rq->curr)
		__dequeue_entity(mycfs_rq, my_se);
	my_se->on_rq = 0;
	
	/*
	 * Normalize the entity after updating the min_vruntime because the
	 * update can refer to the ->curr item and we need to reflect this
	 * movement in our normalized position.
	 */
	if (!(flags & DEQUEUE_SLEEP))
		my_se->vruntime -= mycfs_rq->min_vruntime;

	update_min_vruntime(mycfs_rq);
}

static void dequeue_task_mycfs(rq_t *rq, task_struct_t *p, flag_t flags)
{
	mycfs_rq_t *mycfs_rq = &(rq->mycfs);
	sched_mycfs_entity_t *my_se = &p->my_se;

	printk("***In dequeue_task_mycfs is called. \n");

	dequeue_mycfs_entity(mycfs_rq, my_se, flags);
	printk("***In dequeue_task_mycfs(), before nr--: mycfs_rq = %pa, p = %pa is dequeued, mycfs_rq->nr_running = %lu.\n",mycfs_rq, p, mycfs_rq->nr_running);
	--mycfs_rq->nr_running;
	printk("***In dequeue_task_mycfs(), after nr--: mycfs_rq = %pa, p = %pa is dequeued, mycfs_rq->nr_running = %lu.\n",mycfs_rq, p, mycfs_rq->nr_running);
	
	printk("***In dequeue_task_mycfs is end. \n");

}

static void put_prev_entity(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *prev)
{
	/*
	 * If still on the runqueue then deactivate_task()
	 * was not called and update_curr() has to be done:
	 */

	if (prev->on_rq) {
		update_curr(mycfs_rq);
		/* Put 'current' back into the tree. */
		__enqueue_entity(mycfs_rq, prev);
	}
	mycfs_rq->curr = NULL;
}

/*
 * Account for a descheduled task:
 */
static void put_prev_task_mycfs(rq_t *rq, task_struct_t *prev)
{
	sched_mycfs_entity_t *my_se = &prev->my_se;
	mycfs_rq_t *mycfs_rq = &rq->mycfs;

	put_prev_entity(mycfs_rq, my_se);
}

/*
 * The idea is to set a period in which each task runs once.
 *
 * When there are too many tasks (sysctl_sched_nr_latency) we have to stretch
 * this period because otherwise the slices get too small.
 *
 * p = (nr <= nl) ? l : l*nr/nl
 */
static inline u64 __sched_period(unsigned long nr_running)
{
	u64 period = mycfs_sysctl_sched_latency;
	unsigned long nr_latency = sched_nr_latency;

	if (unlikely(nr_running > nr_latency)) {
		period = nr_running * mycfs_sysctl_sched_min_granularity;
	}

	return period;
}

/*
 * We calculate the wall-time slice from the period by taking a part
 * proportional to the weight.
 *
 * s = p*P[w/rw]
 */
static u64 sched_slice(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *my_se)
{
	u64 tmp = 1 / mycfs_rq->nr_running;
	return __sched_period(mycfs_rq->nr_running + !my_se->on_rq) * tmp;
}

/*
sched_mycfs_entity_t *__pick_first_mycfs_entity(mycfs_rq_t *mycfs_rq)
{
	struct rb_node *left = mycfs_rq->rb_leftmost;

	if (!left)
		return NULL;

	return rb_entry(left, sched_mycfs_entity_t, run_node);
}
*/
/*
static void print_my_se_info(sched_mycfs_entity_t *my_se, char* name){
	printk("Values in %s my_se: pid = %d, policy = %d, on_rq = %d, \n\texec_start = %llu, vruntime = %llu, \n\ts_exe_rtime = %llu, pre_s_exe_rtime = %llu\n", 
		name, task_of(my_se)->pid,task_of(my_se)->policy, my_se->on_rq, my_se->exec_start, my_se->vruntime, my_se->sum_exec_runtime, my_se->prev_sum_exec_runtime);

}
*/


static void print_it_all(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *curr, char* callee){
	struct rb_node *node;

	printk("=================print_it_all start===================\n");
	//printk("Caller: %s\n", callee);
	//printk("Mycfs_rq info: nr_running = %lu, min_vrtime = %llu, \n\ttasks_timeline = %pa, rb_leftmost = %pa, \n\tcurr = %pa\n",
	//	mycfs_rq->nr_running, mycfs_rq->min_vruntime, &mycfs_rq->tasks_timeline, mycfs_rq->rb_leftmost, mycfs_rq->curr);
	
	
	//print_my_se_info(curr, "Curr");

	//printk("Start to print rb tree:\n");

    for (node = rb_first(&mycfs_rq->tasks_timeline); node; node = rb_next(node)){
        printk("pid: %d vruntime: %llu\n", 
        	task_of(rb_entry(node, sched_mycfs_entity_t, run_node))->pid, 
        	rb_entry(node, sched_mycfs_entity_t, run_node)->vruntime);
        //print_my_se_info(); //substitude with this if you need more info
    }

    printk("=================print_it_all end===================\n");
}



/*
 * Preempt the current task with a newly woken task if needed:
 */
static void check_preempt_tick(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *curr)
{

	

	unsigned long ideal_runtime, delta_exec;
	sched_mycfs_entity_t *my_se;
	


	s64 delta;

	printk("***In check_preempt_tick is called. \n");

	ideal_runtime = sched_slice(mycfs_rq, curr);
	printk("In check_preempt_tick(): ideal_runtime of curr is %lu\n", ideal_runtime);
	delta_exec = curr->sum_exec_runtime - curr->prev_sum_exec_runtime;
	if (delta_exec > ideal_runtime) {
		resched_task(rq_of(mycfs_rq)->curr);
		return;
	}

	/*
	 * Ensure that a task that missed wakeup preemption by a
	 * narrow margin doesn't have to wait for a full slice.
	 * This also mitigates buddy induced latencies under load.
	 */
	if (delta_exec < mycfs_sysctl_sched_min_granularity)
		return;

	my_se = __pick_first_mycfs_entity(mycfs_rq);
	delta = curr->vruntime - my_se->vruntime;

	if (delta < 0)
		return;

	/*
	 * If current vruntime > lowest vruntime in mycfs_rq + ideal_runtime
	 * then we reschedule the task
	 */
	if (delta > ideal_runtime)
		resched_task(rq_of(mycfs_rq)->curr);

	

	

	printk("***In check_preempt_tick is end. \n");
}

static void
entity_tick(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *curr, int queued)
{
	//printk("***In entity_tick is called. \n");
	update_curr(mycfs_rq);

	if (mycfs_rq->nr_running > 1)
		check_preempt_tick(mycfs_rq, curr);
	//printk("***In entity_tick is end. \n");
}

static void task_tick_mycfs(rq_t* rq, task_struct_t* curr, int queued)
{
	
	sched_mycfs_entity_t *my_se = &curr->my_se;
	mycfs_rq_t *mycfs_rq = &rq->mycfs;

	//printk("***In task_tick_mycfs is called. \n");
	entity_tick(mycfs_rq, my_se, queued);

	print_it_all(&rq->mycfs, &curr->my_se, "task_tick_mycfs");
	//printk("***In task_tick_mycfs is end. \n");

}


/*
 * We are picking a new current task - update its stats:
 */
static inline 
void update_stats_curr_start(mycfs_rq_t *mycfs_rq, sched_mycfs_entity_t *my_se)
{
	/*
	 * We are starting a new run period:
	 */
	rq_t* rq;

	//printk("***In update_stats_curr_start, FUCK MY LIFE %llu \n", my_se->exec_start);


	rq=rq_of(mycfs_rq);

	//printk("*** successful get rq\n");
	my_se->exec_start = rq->clock_task;

	//printk("***update_stats_curr_start end and exec_start: %llu\n", my_se->exec_start);
}

static void
set_next_entity(struct mycfs_rq *mycfs_rq, struct sched_mycfs_entity *my_se)
{
	//printk("***In set_next_entity is called. \n");
	/* 'current' is not kept within the tree. */
	if (my_se->on_rq) {
		__dequeue_entity(mycfs_rq, my_se);
	}

	//printk("***In set_next_entity before update_stats_curr_start(). \n");
	update_stats_curr_start(mycfs_rq, my_se);

	//printk("***In set_next_entity before mycfs_rq->curr . \n");
	mycfs_rq->curr = my_se;
	//printk("***In set_next_entity after mycfs_rq->curr . \n");
	my_se->prev_sum_exec_runtime = my_se->sum_exec_runtime;

	//printk("***In set_next_entity is end. \n");
}

static void set_curr_task_mycfs(rq_t *rq)
{
	sched_mycfs_entity_t *my_se = &rq->curr->my_se;
	mycfs_rq_t *mycfs_rq = &(rq->mycfs); //maybe can change to rq->mycfs_rq

	
	printk("***In set_curr_task_mycfs \n");
	

	set_next_entity(mycfs_rq, my_se);
}

/*
 * We switched to the sched_mycfs class.
 */
static void switched_to_mycfs(rq_t *rq, task_struct_t *p)
{
	//initialize mycfs_rq of my_se
	p->my_se.mycfs_rq = &rq->mycfs;


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
static void check_preempt_wakeup_mycfs(rq_t *rq, task_struct_t *p, int wake_flags)
{
	task_struct_t *curr = rq->curr;
	sched_mycfs_entity_t *curr_my_se = &curr->my_se, *p_my_se = &p->my_se;
	
	if (unlikely(curr_my_se == p_my_se))
		return;

	/*
	 * We can come here with TIF_NEED_RESCHED already set from new task
	 * wake up path.
	 *
	 * Note: this also catches the edge-case of curr being in a throttled
	 * group (e.g. via set_curr_task), since update_curr() (in the
	 * enqueue of curr) will have resulted in resched being set.  This
	 * prevents us from potentially nominating it as a false LAST_BUDDY
	 * below.
	 */
	if (test_tsk_need_resched(curr))
		return;

	/* Idle tasks are by definition preempted by non-idle tasks. */
	if (unlikely(curr->policy == SCHED_IDLE) &&
	    likely(p->policy != SCHED_IDLE))
		goto preempt;

	/*
	 * Batch and idle tasks do not preempt non-idle tasks (their preemption
	 * is driven by the tick):
	 */
	if (unlikely(p->policy != SCHED_MYCFS))
		return;

	//find_matching_se(&se, &pse);
	update_curr(&rq->mycfs);
	//BUG_ON(!pse);
	if (wakeup_preempt_entity(curr_my_se, p_my_se) == 1) {
		/*
		 * Bias pick_next to pick the sched entity that is
		 * triggering this preemption.
		 */
		//if (!next_buddy_marked)
		//	set_next_buddy(pse);
		goto preempt;
	}

	return;

preempt:
	resched_task(curr);
	
	//print_it_all(&rq->mycfs, &curr->my_se, "check_preempt_wakeup_mycfs");
}

static int wakeup_preempt_entity(sched_mycfs_entity_t* curr, sched_mycfs_entity_t* se)
{
	s64 vdiff = curr->vruntime - se->vruntime;

	if (vdiff <= 0)
		return -1;

	if (vdiff > mycfs_sysctl_sched_wakeup_granularity)
		return 1;

	return 0;
}

static void prio_changed_mycfs(rq_t *rq, task_struct_t *p, prio_t oldprio)
{
	
}

static unsigned int get_rr_interval_mycfs(rq_t *rq, task_struct_t *task)
{
	return 0;
}

/*
 * called on fork with the child task as argument from the parent's context
 *  - child not yet on the tasklist
 *  - preemption disabled
 */
static void task_fork_mycfs(task_struct_t *p)
{
	mycfs_rq_t *mycfs_rq;
	sched_mycfs_entity_t *my_se = &p->my_se, *curr;
	int this_cpu = smp_processor_id();
	rq_t *rq = this_rq();
	unsigned long flags;

	printk("***task_fork_mycfs called %pa\n", p);

	raw_spin_lock_irqsave(&rq->lock, flags);

	update_rq_clock(rq);

	mycfs_rq = task_mycfs_rq(current);
	curr = mycfs_rq->curr;

	if (unlikely(task_cpu(p) != this_cpu)) {
		rcu_read_lock();
		__set_task_cpu(p, this_cpu);
		rcu_read_unlock();
	}

	update_curr(mycfs_rq);

	if (curr)
		my_se->vruntime = curr->vruntime;

	place_entity(mycfs_rq, my_se, 1);

	my_se->vruntime -= mycfs_rq->min_vruntime;

	raw_spin_unlock_irqrestore(&rq->lock, flags);
}

static void yield_task_mycfs(rq_t *rq){
	task_struct_t* curr = rq->curr;
	mycfs_rq_t* mycfs_rq = task_mycfs_rq(curr);
	update_rq_clock(rq);
	update_curr(mycfs_rq);
	rq->skip_clock_update = 1;
}

static bool yield_to_task_mycfs(rq_t* rq, task_struct_t *p, bool preempt){
	return true;
}

static void switched_from_mycfs(rq_t *rq, task_struct_t *p){
	sched_mycfs_entity_t *my_se = &p->my_se;
	mycfs_rq_t *mycfs_rq = &rq->mycfs;

	/*
	 * Ensure the task's vruntime is normalized, so that when its
	 * switched back to the fair class the enqueue_entity(.flags=0) will
	 * do the right thing.
	 *
	 * If it was on_rq, then the dequeue_entity(.flags=0) will already
	 * have normalized the vruntime, if it was !on_rq, then only when
	 * the task is sleeping will it still have non-normalized vruntime.
	 */
	if (!my_se->on_rq && p->state != TASK_RUNNING) {
		/*
		 * Fix up our vruntime so that the current sleep doesn't
		 * cause 'unlimited' sleep bonus.
		 */
		place_entity(mycfs_rq, my_se, 0);
		my_se->vruntime -= mycfs_rq->min_vruntime;
	}
}

#ifdef CONFIG_SMP
static int select_task_rq_mycfs(task_struct_t *p, sd_flag_t sd_flag, flag_t flags)
{
	return task_cpu(p); /* mycfs tasks as never migrated */
}
#endif /* CONFIG_SMP */

const sched_class_t mycfs_sched_class = {
	.next				= &idle_sched_class,
	.enqueue_task		= enqueue_task_mycfs,
	.dequeue_task 		= dequeue_task_mycfs,
	.check_preempt_curr = check_preempt_wakeup_mycfs,
	.pick_next_task		= pick_next_task_mycfs,
	.put_prev_task		= put_prev_task_mycfs,

	.yield_task			= yield_task_mycfs,
	.yield_to_task		= yield_to_task_mycfs,

#ifdef CONFIG_SMP
	.select_task_rq		= select_task_rq_mycfs,
#endif
	.set_curr_task      = set_curr_task_mycfs,
	.task_tick			= task_tick_mycfs,
	.task_fork			= task_fork_mycfs,

	.get_rr_interval	= get_rr_interval_mycfs,

	.prio_changed		= prio_changed_mycfs,
	.switched_to		= switched_to_mycfs,
	.switched_from		= switched_from_mycfs,


};
