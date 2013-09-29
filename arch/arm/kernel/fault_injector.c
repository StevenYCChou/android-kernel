#include <linux/kernel.h>
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

//define err code
#define NO_RUNNING 777 // no current running session
#define NEG_VAL 888 // recieved negtive parameter

long should_fail(void);
long fail_syscall(void);
asmlinkage int sys_fail(int);

/*Todo: 
  @ initial syscall_fail in task_struct
  @ look up for appropriate err code
*/

/* 
 fail the Nth following system call
 */
asmlinkage int sys_fail(int n_to_fail) {

    if(n_to_fail > 0) {
      //set bookeeping # in task_struct
      get_current()->syscall_fail = n_to_fail;
    
    } 
    else if(n_to_fail == 0) {

      // stop the session if there's a current running session, else return error code
      if(get_current()->syscall_fail == 0) {
        return NO_RUNNING;
      }
      else {
        get_current()->syscall_fail = 0;
      }

    }
    else {
      // n_to_fail < 0, return err code
      // shouldn't get here, n_to_fail should be > 0
      return NEG_VAL;
    }

    return 0;
}


long should_fail(void) {

   if (get_current()->syscall_fail == 1){
        (get_current()->syscall_fail)--;
        return 1;
   }
   else {

        (get_current()->syscall_fail)--;
        return 0;
   }
    
}


long fail_syscall(void) {
    //Todo
    return -EINVAL;
}
