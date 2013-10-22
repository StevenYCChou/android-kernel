/*
 *  linux/arch/arm/kernel/netlock.c
 */
#include <linux/linkage.h>
#include <linux/wait.h>
#include <linux/spinlock.h>
#include <linux/sched.h>
#include <linux/netlock.h>

struct myStruct {
    int nreader;
    spinlock_t key_lock;
    netlock_t exc_key;
};

static struct myStruct rw_netlock = {
    .nreader = 0,
    .key_lock = __SPIN_LOCK_UNLOCKED(key_lock),
    .exc_key = NET_LOCK_N,
};

static DECLARE_WAIT_QUEUE_HEAD(r_queue); //reader wait queue
static DECLARE_WAIT_QUEUE_HEAD(w_queue); //writer wait queue

static int key_avail(netlock_t type);
int netlock_acquire(netlock_t type);
int netlock_release(void);
asmlinkage int sys_netlock_acquire(netlock_t type);
asmlinkage int sys_netlock_release(void);

// return 1 if key is available 
// return 0 if key is not available
static int key_avail(netlock_t type){
    
    // if exc_key is availble
    if(rw_netlock.exc_key == NET_LOCK_N){
        return 1;
    }
    // exc_key is not availble
    else if (rw_netlock.exc_key == NET_LOCK_R && type == NET_LOCK_R) {
        spin_lock((&w_queue.lock));
        if(waitqueue_active(&w_queue)){
            spin_unlock((&w_queue.lock));
            return 0;
        }
        else{
            spin_unlock((&w_queue.lock));
            return 1;
        }
    }
    else {
        return 0;
    }    
}



asmlinkage int sys_netlock_acquire(netlock_t type) {
    return netlock_acquire(type);
}

asmlinkage int sys_netlock_release(void) {
    return netlock_release();
}


/* Syscall 378. Acquire netlock. type indicates whether a regular 
or exclusive lock is needed. Returns 0 on success and -1 on failure. */
int netlock_acquire(netlock_t type){
    DEFINE_WAIT(wait);

    if(get_current()->netlock != NET_LOCK_N)
        return -1;

    if(type == NET_LOCK_R){ //requiring reader lock
        while(1){
            //interruptible_sleep_on(&r_queue);
            prepare_to_wait(&r_queue, &wait, TASK_INTERRUPTIBLE);
            if (signal_pending(current)){
                break;
            }
            spin_lock(&(rw_netlock.key_lock));
            if (key_avail(type)) {
                get_current()->netlock = NET_LOCK_R;
                rw_netlock.nreader ++;
                rw_netlock.exc_key = NET_LOCK_R;
                spin_unlock(&(rw_netlock.key_lock));
                break;
            }
            spin_unlock(&(rw_netlock.key_lock));
            schedule();
        }
        finish_wait(&r_queue, &wait);
        // wake up here
        return 0;
    }
    else if(type == NET_LOCK_E) { //acquring writer lock
        while(1){
            prepare_to_wait_exclusive(&w_queue, &wait, TASK_INTERRUPTIBLE);
            if (signal_pending(current)){
                break;
            }
            spin_lock(&(rw_netlock.key_lock));
            if (key_avail(type)) {
                // wake up here, takes lock
                get_current()->netlock = NET_LOCK_E;
                rw_netlock.exc_key = NET_LOCK_E;
                spin_unlock(&(rw_netlock.key_lock));            
                break;
            }
            spin_unlock(&(rw_netlock.key_lock));            
            schedule();
        }
        finish_wait(&r_queue, &wait);
        return 0;
     }
     else{
        return -1; //received invalid argument
     }
     
}

/* Syscall 379. Release netlock. Return 0 on success and -1 on failure.*/
int netlock_release(void){
    if (get_current()->netlock == NET_LOCK_R){
        spin_lock(&(rw_netlock.key_lock));
        --rw_netlock.nreader;
        get_current()->netlock = NET_LOCK_N;
        // if there are still readers left, do nothing and exit
        if (rw_netlock.nreader > 0) {
            spin_unlock(&(rw_netlock.key_lock));
            return 0;
        }
        // if no reader left, give lock to w_Queue if there is anyone
        else {
            rw_netlock.exc_key = NET_LOCK_N; // return lock
            // if w_queue is active, do nothing
            if (waitqueue_active(&w_queue)){
                spin_unlock(&(rw_netlock.key_lock));
                // wake up first one and return 0
                wake_up_interruptible(&w_queue);
                return 0;
            }
            // if w_queue is not active, do nothing
            else {
                spin_unlock(&(rw_netlock.key_lock));
                return 0;
            }
        }
        return 0;
    }
    else if(get_current()->netlock == NET_LOCK_E){ 
        spin_lock(&(rw_netlock.key_lock));
        get_current()->netlock = NET_LOCK_N;
        rw_netlock.exc_key = NET_LOCK_N;
        // if w_queue is not active, wake all reader
        if (!waitqueue_active(&w_queue)){
            // if reader queue active, wake all
            if (waitqueue_active(&r_queue)){
                spin_unlock(&(rw_netlock.key_lock)); 
                wake_up_interruptible_all(&r_queue);
            }
            else {
                spin_unlock(&(rw_netlock.key_lock)); 
                // both queues are not active, do nothing
            }
        }
        else{
            spin_unlock(&(rw_netlock.key_lock));            
            wake_up_interruptible(&w_queue);
        }
        return 0;
    }
    else{
        return -1; // has no lock, do nothing.
    }   
}

