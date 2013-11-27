#include <linux/linkage.h>
#include <linux/types.h>
#include <linux/errno.h>

int ext4_cowcopy(const char __user *src, const char __user *dest);
asmlinkage int sys_ext4_cowcopy(const char __user *src, const char __user *dest);


asmlinkage int sys_ext4_cowcopy(const char __user *src, const char __user *dest){
	return ext4_cowcopy(src, dest);
}

int  ext4_cowcopy(const char __user *src, const char __user *dest){
	return 0;
}
