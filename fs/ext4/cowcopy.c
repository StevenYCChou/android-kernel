#include <linux/linkage.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <asm/stat.h>
#include <linux/stat.h>
#include <linux/unistd.h>

//#include <linux/limits.h>
//#include <linux/ioctl.h>
//#include <linux/blk_types.h>
//#include <linux/types.h>

int ext4_cowcopy(const char __user *src, const char __user *dest);
asmlinkage int sys_ext4_cowcopy(const char __user *src, const char __user *dest);
extern int vfs_stat(const char __user *, struct kstat *);

asmlinkage int sys_ext4_cowcopy(const char __user *src, const char __user *dest){
	//struct __old_kernel_stat st_buf;
	struct kstat kstat;

    printk ("### sys_ext4_cowcopy is called\n");

    if (0 != vfs_stat(src, &kstat)) {
        printk ("Error in stat\n");
        return -1;
    }

    printk("### stat.ino: %llu \n", kstat.ino);
    printk("### stat.dev: %llu \n", (unsigned long long)kstat.dev);
    printk("### stat.mode: %llu \n", (unsigned long long)kstat.mode);


    // if (!S_ISREG (st_buf.st_mode)) {
    //     printk ("%s is not a regular file.\n", src);
    //     return -EPERM;
    // }
    // if (S_ISDIR (st_buf.st_mode)) {
    //     printk ("%s is a directory.\n", src);
    //     return -EPERM;
    // }

	return ext4_cowcopy(src, dest);
}

int  ext4_cowcopy(const char __user *src, const char __user *dest){
	return 0;
}
