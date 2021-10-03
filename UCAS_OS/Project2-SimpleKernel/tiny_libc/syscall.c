#include <sys/syscall.h>
#include <screen.h>
#include <stdint.h>

void sys_sleep(uint32_t time)
{
    // TODO:
}

void sys_write(char *buff)
{
    port_write(buff);
}

void sys_reflush()
{
    // TODO:
}

void sys_move_cursor(int x, int y)
{
    vt100_move_cursor(x, y);
}

long sys_get_timebase()
{
    // TODO:
}

long sys_get_tick()
{
    // TODO:
}

void sys_yield()
{
    // TODO:
    // invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE);
    do_scheduler();
}
