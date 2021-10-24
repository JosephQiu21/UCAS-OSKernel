#include <stdatomic.h>
#include <mthread.h>
#include <sys/syscall.h>
#include <os/lock.h>

int mutex_get(int key);

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
