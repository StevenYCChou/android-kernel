#include <linux/linkage.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/stat.h>
#include <linux/unistd.h>

int ext4_cowcopy(const char __user *src, const char __user *dest);
asmlinkage int sys_ext4_cowcopy(const char __user *src, const char __user *dest);


asmlinkage int sys_ext4_cowcopy(const char __user *src, const char __user *dest){
	struct __old_kernel_stat st_buf;

    printk ("###### sys_ext4_cowcopy is called\n");

    if (0 != sys_stat(src, &st_buf)) {
        printk ("Error in stat\n");
        return -1;
    }

    if (!S_ISREG (st_buf.st_mode)) {
        printk ("%s is not a regular file.\n", src);
        return -EPERM;
    }
    if (S_ISDIR (st_buf.st_mode)) {
        printk ("%s is a directory.\n", src);
        return -EPERM;
    }

	return ext4_cowcopy(src, dest);
}

int  ext4_cowcopy(const char __user *src, const char __user *dest){
	return 0;
}
