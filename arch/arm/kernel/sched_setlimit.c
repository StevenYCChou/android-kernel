/*
 *  linux/arch/arm/kernel/sched_setlimit.c
 */

#include <linux/types.h>

int sched_setlimit(pid_t pid, int limit){
	struct task_struct* p;

	if(pid <= 0 || limit < 0 || limit > 100)
		return -EINVAL;

	p = find_process_by_pid(pid);
	if (!p){
		return -ESRCH;
	}

	(p->my_se).sched_limit = (limit ? limit : 100);
	return 0;
};