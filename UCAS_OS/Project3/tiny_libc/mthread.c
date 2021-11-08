#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <os/lock.h>

void mthread_mutex_init(void* handle)
{
    int *id = (int *)handle;
    *id = sys_mutex_get((int)handle);
}

void mthread_mutex_lock(void* handle) 
{
    int *id = (int *)handle;
    *id = sys_mutex_get((int)handle);
    sys_mutex_op(*id, LOCK);
}

void mthread_mutex_unlock(void* handle)
{
    int *id = (int *)handle;
    *id = sys_mutex_get((int)handle);
    sys_mutex_op(*id, UNLOCK);
}

int mthread_barrier_init(void* handle, unsigned count)
{
    // TODO:
}
int mthread_barrier_wait(void* handle)
{
    // TODO:
}
int mthread_barrier_destroy(void* handle)
{
    // TODO:
}

int mthread_semaphore_init(void* handle, int val)
{
    // TODO:
}
int mthread_semaphore_up(void* handle)
{
    // TODO:
}
int mthread_semaphore_down(void* handle)
{
    // TODO:
}
int mthread_semaphore_destroy(void* handle)
{
    // TODO:
}
