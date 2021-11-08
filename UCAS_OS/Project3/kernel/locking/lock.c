#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>

mutex_lock_t lock[40];
int hash[40];
static int id = 0;

void do_mutex_lock_init(mutex_lock_t *lock)
{
    __asm__ __volatile__("csrr x0, sscratch\n");
    list_init(&(lock -> block_queue));
    lock -> status = UNLOCKED;
}

void do_mutex_lock_acquire(mutex_lock_t *lock)
{
    if(lock -> status == LOCKED){
        prints("\n\n\n\n\n> [WARNING] Lock is occupied by other process!\n");
        do_block(&(current_running -> list), &(lock -> block_queue));
    }else{
        lock -> status = LOCKED;
        current_running -> locks[current_running -> num_lock++] = lock;
        prints("\n\n\n\n\n\n> [SUCCESS] Lock acquired!\n");
    }
}

void do_mutex_lock_release(mutex_lock_t *lock)
{
    int i;
    for (i = 0; i < current_running -> num_lock; i++) {
        if (current_running -> locks[i] = lock) {
            for (int j = i; j < current_running -> num_lock; j++)
                current_running -> locks[j] = current_running -> locks[j + 1];
            current_running -> num_lock--;
            break;
        }
        prints("\n\n\n\n\n\n\n> [ERROR] This lock is not acquired by current running process!\n");
        return;
    }

    if(is_list_empty(&(lock -> block_queue))){
        lock -> status = UNLOCKED;
        prints("\n\n\n\n\n\n\n\n> [SUCCESS] Lock released!\n");
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