#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <os/lock.h>
#include <os/sync.h>

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

void mthread_barrier_init(void* handle, unsigned count)
{
    int *id = (int *)handle;
    *id = sys_barrier_get((int)handle);
    sys_barrier_init(*id, count);
}

void mthread_barrier_wait(void* handle)
{
    int *id = (int *)handle;
    sys_barrier_op(*id, WAIT);
}

void mthread_barrier_destroy(void* handle)
{
    int *id = (int *)handle;
    sys_barrier_op(*id, DESTROY);
}

void mthread_semaphore_init(void* handle, int val)
{
    int *id = (int *)handle;
    *id = sys_semaphore_get((int)handle);
    sys_semaphore_init(*id, val);
}

void mthread_semaphore_up(void* handle)
{
    int *id = (int *)handle;
    sys_semaphore_op(*id, UP);
}

void mthread_semaphore_down(void* handle)
{
    int *id = (int *)handle;
    sys_semaphore_op(*id, DOWN);
}

void mthread_semaphore_destroy(void* handle)
{
    int *id = (int *)handle;
    sys_semaphore_op(*id, DESTROY);
}
