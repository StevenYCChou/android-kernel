#include <linux/export.h>
#include <linux/errno.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/sem.h>
#include <linux/msg.h>
#include <linux/shm.h>
#include <linux/stat.h>
#include <linux/syscalls.h>
#include <linux/mman.h>
#include <linux/fs.h>
#include <linux/file.h>
#include <linux/ipc.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

long should_fail(void);
long fail_syscall(void);

/* 
 
 */
asmlinkage int sys_fail(int Nth_to_fail)
{
    get_current()->remaining_times_to_fail = Nth_to_fail;

    return 0;
}

long should_fail(void){
   if (get_current()->remaining_times_to_fail == 0){
        return 1;
   }
   else {
        get_current()->remaining_times_to_fail--;
        return 0;
   }
    
}

long fail_syscall(void){
    return 1;

}
