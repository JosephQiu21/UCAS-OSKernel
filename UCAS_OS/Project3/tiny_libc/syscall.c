#include <os/syscall_number.h>
#include <screen.h>
#include <stdint.h>
#include <os/sched.h>

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

int sys_mutex_get(int key)
{
    return invoke_syscall(SYSCALL_MUTEX_GET, key, IGNORE, IGNORE);
}

void sys_mutex_op(int id, int op)
{
    invoke_syscall(SYSCALL_MUTEX_OP, id, op, IGNORE);
}

uint32_t sys_get_wall_time(uint32_t *time_elapsed)
{
    return invoke_syscall(SYSCALL_GET_WALL, time_elapsed, IGNORE, IGNORE);
}

pid_t sys_spawn(task_info_t *info, void *arg, spawn_mode_t mode)
{
    return invoke_syscall(SYSCALL_SPAWN, info, arg, mode);
}

int sys_kill(pid_t pid)
{
    return invoke_syscall(SYSCALL_KILL, pid, IGNORE, IGNORE);
}

void sys_exit(void)
{
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid)
{
    invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE);
}

void sys_process_show(void)
{
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(void)
{
    invoke_syscall(SYSCALL_SCREEN_CLEAR, IGNORE, IGNORE, IGNORE);
}

pid_t sys_getpid(){
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE);
}

void sys_serial_write(char c)
{
    invoke_syscall(SYSCALL_SERIAL_WRITE, c, IGNORE, IGNORE);
}

int sys_get_char(void)
{
    return invoke_syscall(SYSCALL_SERIAL_READ, IGNORE, IGNORE, IGNORE);
}

char sys_read(void)
{
    return invoke_syscall(SYSCALL_READ, IGNORE, IGNORE, IGNORE);
}

int sys_semaphore_get(int key){
    return invoke_syscall(SYSCALL_SEMAPHORE_GET, key, IGNORE, IGNORE);
}

void sys_semaphore_init(int id, int val){
    invoke_syscall(SYSCALL_SEMAPHORE_INIT, id, val, IGNORE);
}

void sys_semaphore_op(int id, int op){
    invoke_syscall(SYSCALL_SEMAPHORE_OP, id, op, IGNORE);
}

int sys_barrier_get(int key){
    return invoke_syscall(SYSCALL_BARRIER_GET, key, IGNORE, IGNORE);
}

void sys_barrier_init(int id, int count){
    invoke_syscall(SYSCALL_BARRIER_INIT, id, count, IGNORE);
}

void sys_barrier_op(int id, int op){
    invoke_syscall(SYSCALL_BARRIER_OP, id, op, IGNORE);
}

int sys_mbox_open(char *name){
    return invoke_syscall(SYSCALL_MBOX_OPEN, name, IGNORE, IGNORE);
}

void sys_mbox_close(int id){
    invoke_syscall(SYSCALL_MBOX_CLOSE, id, IGNORE, IGNORE);
}

int sys_mbox_send(int id, void *msg, int msg_length){
    return invoke_syscall(SYSCALL_MBOX_SEND, id, msg, msg_length);
}

int sys_mbox_recv(int id, void *msg, int msg_length){
    return invoke_syscall(SYSCALL_MBOX_RECV, id, msg, msg_length);
}
