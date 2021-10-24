#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>

mutex_lock_t lock[40];
int hash[40];
static int id = 0;

void do_mutex_lock_init(mutex_lock_t *lock)
{
    list_init(&(lock -> block_queue));
    lock -> status = UNLOCKED;
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    if(lock -> status == LOCKED){
        do_block(&(current_running -> list), &(lock -> block_queue));
    }else{
        lock -> status = LOCKED;
    }
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    if(is_list_empty(&(lock -> block_queue))){
        lock -> status = UNLOCKED;
    }else{
        // Pop a task from block queue and add it to ready queue
        do_unblock(dequeue(&(lock -> block_queue)));
    }
}

int mutex_get(int key){
    if (fetchhash(key) == -1){
        hash[id] = key;
        do_mutex_lock_init(&lock[id]);
        return id++;
    }else{
        return fetchhash(key);
    }
}

int mutex_op(int handle, int op){
    if (op == LOCK)
        do_mutex_lock_acquire(&lock[handle]);
    else
        do_mutex_lock_release(&lock[handle]);
    return 0;
}

int fetchhash(int key){
    for (int i = 0; i < 40; i++){
        if (hash[i] == key) return i;
        else return -1;
    }
}