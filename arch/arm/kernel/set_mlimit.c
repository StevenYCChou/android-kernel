
#include <linux/linkage.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>

int set_mlimit(uid_t uid, long mem_max){
	struct user_struct *user;

	if (mem_max <= 0)
		return -EINVAL;

	user = find_user(uid);
	if(user == NULL)
		return -EINVAL;

	user->mem_quota = mem_max;

	printk("Set_mlimit succeed, the mem_max of the user %lu is %lu.\n", 
		(unsigned long)uid, (unsigned long)user->mem_quota);

	free_uid(user);

	return 0;	
}

asmlinkage int sys_set_mlimit(uid_t uid, long mem_max){
	return set_mlimit(uid, mem_max);
}