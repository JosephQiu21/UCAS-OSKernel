#include <test2.h>
#include <stdio.h>
#include <sys/syscall.h>
#include <time.h>

void timer_task(void)
{
    __asm__ __volatile__("csrr x0, sscratch\n");
    int print_location = 2;

    while (1)
    {
        /* call get_timer() to get time */
        // uint32_t time_elapsed = clock();
        uint32_t _time_elapsed = 0;
        uint32_t _time_base = sys_get_wall_time(&_time_elapsed);
	    uint32_t time = _time_elapsed / _time_base;
        sys_move_cursor(1, print_location);
        printf("> [TASK] This is a thread to timing! (%u/%u seconds).\n", time, _time_elapsed);
        // sys_yield();
    }
}
