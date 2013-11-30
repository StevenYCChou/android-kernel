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
#include <linux/namei.h>

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
	struct nameidata nd;
	dev_t s_dev_src, s_dev_dest;

    //printk ("### sys_ext4_cowcopy is called, src: %s, dest: %s\n", src, dest);
    printk ("### sys_ext4_cowcopy is called\n");

    //get the kstat info of the src
    if (vfs_stat(src, &kstat) != 0) {
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


    //check if src is in ext4 fs
	if (kern_path_parent(src, &nd)!=0){
		//this err shouldn't occur since src has been verified by vfs_stat, or
		//we misunderstand the function
		printk("### kern_path_parent failed when checking if src is in ext4 (This err shouldn't occur)\n");
		return -999;
	}
	else{
		if (strcmp(nd.inode->i_sb->s_type->name,"ext4"))
		{
			printk ("### src isn't ext4 fs, it's in: %s\n", nd.inode->i_sb->s_type->name);
			return -EOPNOTSUPP;
		}
		printk ("### src is in the fs of: %s\n", nd.inode->i_sb->s_type->name);
	}

	//check if src and dest are in the same device
	s_dev_src = nd.inode->i_sb->s_dev;
	if( kern_path_parent(dest, &nd) != 0){
		//incorrect dest parameter. what should we return? 
		printk("### kern_path_parent failed when checking dest, dest: %s\n", dest);
		return -EINVAL;
	}else{
		//check if dest is a file. Again, make sure what should we return.
		printk("### The last char of dest is: %c\n", dest[strlen(dest)-1]);
		if(dest[strlen(dest)-1] == '/'){
			printk("### The dest is a directory.\n");
			return -EINVAL;
		}

		s_dev_dest = nd.inode->i_sb->s_dev;
		printk("### s_dev_src: %lu, s_dev_dest: %lu\n", 
			(unsigned long)s_dev_src, (unsigned long)s_dev_dest);
		if(s_dev_src != s_dev_dest){
			printk("### src and dest are in different device\n");
			return -EXDEV;
		}
	}

	return ext4_cowcopy(src, dest);
}

int  ext4_cowcopy(const char __user *src, const char __user *dest){
    struct path src_path;
    struct nameidata src_parent_nd;
    struct path dest_path;
    struct dentry* dest_dentry;
    
    int res;
    
    res = kern_path(src, LOOKUP_FOLLOW, &src_path);
    if(res)
        printk("### some error when lookup src path: %s\n", src);

    res = kern_path_parent(src, &src_parent_nd);
    if(res)
        printk("### some error when lookup parent of src path: %s\n", src);    

    dest_dentry = user_path_create(AT_FDCWD, dest, &dest_path, 0);
    if(res)
        printk("### some error when lookup dest path: %s\n", dest);

    printk("### begin of hardlink between src: %s and dest: %s\n", src, dest);
    src_parent_nd.inode->i_op->link(src_path.dentry, src_parent_nd.inode, dest_path.dentry);
    printk("### end of hardlink between src: %s and dest: %s\n", src, dest);
    
	return 0;
}
