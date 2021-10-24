#include <os/syscall_number.h>
#include <screen.h>
#include <stdint.h>

void sys_sleep(uint32_t time)
{
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, buff, IGNORE, IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE);
}

long sys_get_timebase()
{
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick()
{
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE);
}

void sys_yield()
{
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE);
}

void sys_lock_init(uint64_t lock)
{
    invoke_syscall(SYSCALL_LOCK_INIT, lock, IGNORE, IGNORE);
}

void sys_lock_acqure(uint64_t lock)
{
    invoke_syscall(SYSCALL_LOCK_ACQUIRE, lock, IGNORE, IGNORE);
}

void sys_lock_release(uint64_t lock)
{
    invoke_syscall(SYSCALL_LOCK_RELEASE, lock, IGNORE, IGNORE);
}
