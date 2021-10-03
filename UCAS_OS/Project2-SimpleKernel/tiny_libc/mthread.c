#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <os/lock.h>

int mthread_mutex_init(void* handle)
{
    do_mutex_lock_init(&((mthread_mutex_t *)handle) -> mlock);
    return 0;
}
int mthread_mutex_lock(void* handle) 
{
    do_mutex_lock_acquire(&((mthread_mutex_t *)handle) -> mlock);
    return 0;
}
int mthread_mutex_unlock(void* handle)
{
    do_mutex_lock_release(&((mthread_mutex_t *)handle) -> mlock);
    return 0;
}
