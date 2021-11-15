#include <os/sync.h>
#include <os/lock.h>
#include <os/sched.h>
#include <atomic.h>

#define MAX_LOCK_NUM 16
#define MAX_SEMAPHORE_NUM 16
#define MAX_BARRIER_NUM 16

static int lock_id = 0;
static int semaphore_id = 0;
static int barrier_id = 0;

int lock_hash[MAX_LOCK_NUM];
int semaphore_hash[MAX_SEMAPHORE_NUM];
int barrier_hash[MAX_BARRIER_NUM];

mutex_lock_t lock[MAX_LOCK_NUM];
semaphore_t semaphore[MAX_SEMAPHORE_NUM];
barrier_t barrier[MAX_BARRIER_NUM];


void do_mutex_lock_init(mutex_lock_t *lock)
{
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
    if (lock_fetchhash(key) == -1){
        lock_hash[lock_id] = key;
        do_mutex_lock_init(&lock[lock_id]);
        return lock_id++;
    }else{
        return lock_fetchhash(key);
    }
}

void mutex_op(int handle, int op){
    if (op == LOCK)
        do_mutex_lock_acquire(&lock[handle]);
    else
        do_mutex_lock_release(&lock[handle]);
}

int lock_fetchhash(int key){
    for (int i = 0; i < MAX_LOCK_NUM; i++){
        if (lock_hash[i] == key) return i;
        else return -1;
    }
}

int semaphore_fetchhash(int key){
    for (int i = 0; i < MAX_SEMAPHORE_NUM; i++){
        if (semaphore_hash[i] == key) return i;
        else return -1;
    }
}

int barrier_fetchhash(int key){
    for (int i = 0; i < MAX_BARRIER_NUM; i++){
        if (barrier_hash[i] == key) return i;
        else return -1;
    }
}

void do_semaphore_init(semaphore_t *sem, int val){
    sem -> val = val;
    sem -> destoryed = 0;
    list_init(&(sem -> block_queue));
}

void do_semaphore_up(semaphore_t *sem){
    sem -> val++;
    if (!is_list_empty(&(sem -> block_queue)))
        do_unblock(dequeue(&(sem -> block_queue)));
}

void do_semaphore_down(semaphore_t *sem){
    sem -> val--;
    if (sem -> val < 0)
        do_block(&(current_running -> list), &(sem -> block_queue));
}

void do_semaphore_destroy(semaphore_t *sem){
    sem -> val = 0;
    sem -> destoryed = 1;
    while (!is_list_empty(&(sem -> block_queue)))
        do_unblock(dequeue(&(sem -> block_queue)));
}

void semaphore_init(int handle, int val){
    do_semaphore_init(&semaphore[handle], val);
}

int semaphore_get(int key){
    if (semaphore_fetchhash(key) == -1){
        semaphore_hash[semaphore_id] = key;
        return semaphore_id++;
    }else{
        return semaphore_fetchhash(key);
    }
}

void semaphore_op(int handle, int op){
    if (op == DOWN)
        do_semaphore_down(&barrier[handle]);
    else if (op == UP)
        do_semaphore_up(&barrier[handle]);
    else if (op == DESTROY)
        do_semaphore_destroy(&barrier[handle]);
}

void do_barrier_init(barrier_t *bar, int count){
    bar -> total_num = count;
    bar -> wait_num = 0;
    bar -> destoryed = 0;
    list_init(&(bar -> block_queue));
}

void do_barrier_wait(barrier_t *bar){
    bar -> wait_num++;
    if (bar -> wait_num == bar -> total_num){
        while (!is_list_empty(&(bar -> block_queue)))
            do_unblock(dequeue(&(bar -> block_queue)));
        bar -> wait_num = 0;
    } else {
        do_block(&(current_running -> list), &(bar -> block_queue));
    }
}

void do_barrier_destroy(barrier_t *bar){
    bar -> total_num = 0;
    bar -> wait_num = 0;
    bar -> destoryed = 1;
    while (!is_list_empty(&(bar -> block_queue)))
        do_unblock(dequeue(&(bar -> block_queue)));
}

void barrier_init(int handle, int count){
    do_barrier_init(&barrier[handle], count);
}

int barrier_get(int key){
    if (barrier_fetchhash(key) == -1){
        barrier_hash[barrier_id] = key;
        return barrier_id++;
    }else{
        return barrier_fetchhash(key);
    }
}

void barrier_op(int handle, int op){
    if (op == WAIT)
        do_barrier_wait(&barrier[handle]);
    else
        do_barrier_destroy(&barrier[handle]);
}