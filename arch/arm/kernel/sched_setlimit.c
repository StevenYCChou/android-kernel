/*
 *  linux/arch/arm/kernel/sched_setlimit.c
 */
int sched_setlimit(pid_t pid, int limit){
	struct task_struct* p;

	if(pid < 0 || limit < 0)
		return -EINVAL;

	p = find_process_by_pid(pid);
	if (!p){
		return -ESRCH;
	}

	(p->my_se).time_limit_per_period = (limit ? limit : 100);
	return 0;
};