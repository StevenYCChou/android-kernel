
#include <linux/linkage.h>
#include <linux/types.h>

int set_mlimit(uid_t uid, long mem_max){
	return 1;	
}

asmlinkage int sys_set_mlimit(uid_t uid, long mem_max){
	return set_mlimit(uid, mem_max);
}