#include <os/time.h>
#include <os/mm.h>
#include <os/irq.h>
#include <os/sched.h>
#include <type.h>

uint64_t time_elapsed = 0;
uint32_t time_base = 0;

uint64_t get_ticks()
{
    __asm__ __volatile__(
        "rdtime %0"
        : "=r"(time_elapsed));
    return time_elapsed;
}

uint32_t get_wall_time(uint32_t *time_elapsed)
{
    *time_elapsed = get_ticks();
    return get_time_base();
}

// Get current time
uint64_t get_timer()
{
    return get_ticks() / time_base;
}

uint64_t get_time_base()
{
    return time_base;
}

void latency(uint64_t time)
{
    uint64_t begin_time = get_timer();

    while (get_timer() - begin_time < time);
    return;
}

// Create a timer for current_running process
void create_timer(uint64_t ticks)
{
    //disable_preempt();
    current_running -> timeout_ticks = get_ticks() + ticks;
    enqueue(&sleep_queue, &(current_running -> list));
    //enable_preempt();
}

// Check through the sleep queue 
// to see if any of them should be waked up
void check_timer()
{
    //disable_preempt();
    pcb_t *tmp;
    list_node_t *p = sleep_queue.next;
    while(!is_list_empty(&sleep_queue) && (p != &sleep_queue)){
        tmp = container_of(p, pcb_t, list);
        p = p -> next;
        if(get_ticks() >= tmp -> timeout_ticks)
            do_unblock(&(tmp -> list));  // Wake up
    }
    //enable_preempt();
}