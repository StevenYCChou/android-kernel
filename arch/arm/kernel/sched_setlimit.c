/*
 *  linux/arch/arm/kernel/sched_setlimit.c
 */

#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <linux/pid.h>

int sched_setlimit(pid_t pid, int limit){
	struct task_struct* p;

	if(pid <= 0 || limit < 0 || limit > 100)
		return -EINVAL;


	p = pid_task(find_vpid(pid), PIDTYPE_PID);
	if (!p){
		return -ESRCH;
	}

	(p->my_se).sched_limit = (unsigned long long) (limit ? limit : 100);
	return 0;
};