#include <linux/errno.h>
#include <linux/kernel.h>
#include <linux/sched.h>

long should_fail(void);
long fail_syscall(void);
asmlinkage int sys_fail(int);

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
        return -EINVAL;
      }
      else {
        get_current()->syscall_fail = 0;
      }

    }
    else {
      // n_to_fail < 0, return err code
      // shouldn't get here, n_to_fail should be > 0
      return -EINVAL;
    }

    return 0;
}

long should_fail(void) {

  if (get_current()->syscall_fail == 1){
    // should fail!
    (get_current()->syscall_fail)--;
    return 1;
  }
  else if(get_current()->syscall_fail == 0){
    // continue execution, no fail
    return 0;
  }
  else{
    // continue execution, no fail, but subtract one from bookkeeping
    (get_current()->syscall_fail)--;
    return 0;
  }
    
}

long fail_syscall(void) {
    return -EINVAL;
}
