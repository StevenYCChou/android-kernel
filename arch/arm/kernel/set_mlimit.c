
#include <linux/linkage.h>
#include <linux/types.h>
#include <linux/sched.h>
#include <linux/errno.h>
#include <asm-generic/atomic-long.h>

int set_mlimit(uid_t uid, long mem_max){
	struct user_struct *user;

	printk("set_mlimit is called 1\n");

	if (mem_max <= 0)
		return -EINVAL;

	user = find_user(uid);
	if(user == NULL)
		return -EINVAL;

	printk("set_mlimit is called 2\n");

	atomic_long_set(&user->mem_quota, mem_max);

	printk("Set_mlimit succeed, the mem_max of the user %lu is %lu.\n", 
		(unsigned long)uid, atomic_long_read(&user->mem_quota));

	free_uid(user);

	return 0;	
}

asmlinkage int sys_set_mlimit(uid_t uid, long mem_max){
	return set_mlimit(uid, mem_max);
}