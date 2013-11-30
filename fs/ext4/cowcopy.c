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

    //get the kstat info of the src
    if ( vfs_stat(src, &kstat) != 0) {
        printk ("### %s doesn't exist.\n", src);
        return -EPERM;
    }
    //check whether src is a regular file and isn't a dir 
    if (!S_ISREG (kstat.mode) || S_ISDIR (kstat.mode)) {
        printk ("### %s is not a regular file or it's a directory.\n", src);
        return -EPERM;
    }
    
    printk("### stat.ino: %llu \n", kstat.ino);
    printk("### stat.dev: %llu \n", (unsigned long long)kstat.dev);
    printk("### stat.mode: %llu \n", (unsigned long long)kstat.mode);

    

   

	return ext4_cowcopy(src, dest);
}

int  ext4_cowcopy(const char __user *src, const char __user *dest){
	return 0;
}
