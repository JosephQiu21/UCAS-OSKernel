#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>

void do_mutex_lock_init(mutex_lock_t *lock)
{
    list_init(&(lock -> block_queue));
    lock -> lock.status = UNLOCKED;
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    if(lock -> lock.status == LOCKED){
        do_block(&(current_running -> list), &(lock -> block_queue));
    }else{
        lock -> lock.status = LOCKED;
    }
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    if(is_list_empty(&(lock -> block_queue))){
        lock -> lock.status = UNLOCKED;
    }else{
        // Pop a task from block queue and add it to ready queue
        do_unblock(dequeue(&(lock -> block_queue)));
    }
}
