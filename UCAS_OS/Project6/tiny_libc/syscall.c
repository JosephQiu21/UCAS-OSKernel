#include <os/syscall_number.h>
#include <stdint.h>
#include <os/sched.h>
#include <mthread.h>

void sys_sleep(uint32_t time)
{
    invoke_syscall(SYSCALL_SLEEP, time, IGNORE, IGNORE, IGNORE);
}

void sys_write(char *buff)
{
    invoke_syscall(SYSCALL_WRITE, buff, IGNORE, IGNORE, IGNORE);
}

void sys_reflush()
{
    invoke_syscall(SYSCALL_REFLUSH, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_move_cursor(int x, int y)
{
    invoke_syscall(SYSCALL_CURSOR, x, y, IGNORE, IGNORE);
}

long sys_get_timebase()
{
    return invoke_syscall(SYSCALL_GET_TIMEBASE, IGNORE, IGNORE, IGNORE, IGNORE);
}

long sys_get_tick()
{
    return invoke_syscall(SYSCALL_GET_TICK, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_yield()
{
    invoke_syscall(SYSCALL_YIELD, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_mutex_get(int key)
{
    return invoke_syscall(SYSCALL_MUTEX_GET, key, IGNORE, IGNORE, IGNORE);
}

void sys_mutex_op(int id, int op)
{
    invoke_syscall(SYSCALL_MUTEX_OP, id, op, IGNORE, IGNORE);
}

uint32_t sys_get_wall_time(uint32_t *time_elapsed)
{
    return invoke_syscall(SYSCALL_GET_WALL, time_elapsed, IGNORE, IGNORE, IGNORE);
}

pid_t sys_spawn(task_info_t *info, void *arg, spawn_mode_t mode)
{
    return invoke_syscall(SYSCALL_SPAWN, info, arg, mode, IGNORE);
}

int sys_kill(pid_t pid)
{
    return invoke_syscall(SYSCALL_KILL, pid, IGNORE, IGNORE, IGNORE);
}

void sys_exit(void)
{
    invoke_syscall(SYSCALL_EXIT, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_waitpid(pid_t pid)
{
    invoke_syscall(SYSCALL_WAITPID, pid, IGNORE, IGNORE, IGNORE);
}

void sys_process_show(void)
{
    invoke_syscall(SYSCALL_PS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_screen_clear(void)
{
    invoke_syscall(SYSCALL_SCREEN_CLEAR, IGNORE, IGNORE, IGNORE, IGNORE);
}

pid_t sys_getpid(){
    return invoke_syscall(SYSCALL_GETPID, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_serial_write(char c)
{
    invoke_syscall(SYSCALL_SERIAL_WRITE, c, IGNORE, IGNORE, IGNORE);
}

int sys_get_char(void)
{
    return invoke_syscall(SYSCALL_SERIAL_READ, IGNORE, IGNORE, IGNORE, IGNORE);
}

char sys_read(void)
{
    return invoke_syscall(SYSCALL_READ, IGNORE, IGNORE, IGNORE, IGNORE);
}

int sys_semaphore_get(int key){
    return invoke_syscall(SYSCALL_SEMAPHORE_GET, key, IGNORE, IGNORE, IGNORE);
}

void sys_semaphore_init(int id, int val){
    invoke_syscall(SYSCALL_SEMAPHORE_INIT, id, val, IGNORE, IGNORE);
}

void sys_semaphore_op(int id, int op){
    invoke_syscall(SYSCALL_SEMAPHORE_OP, id, op, IGNORE, IGNORE);
}

int sys_barrier_get(int key){
    return invoke_syscall(SYSCALL_BARRIER_GET, key, IGNORE, IGNORE, IGNORE);
}

void sys_barrier_init(int id, int count){
    invoke_syscall(SYSCALL_BARRIER_INIT, id, count, IGNORE, IGNORE);
}

void sys_barrier_op(int id, int op){
    invoke_syscall(SYSCALL_BARRIER_OP, id, op, IGNORE, IGNORE);
}

int sys_mbox_open(char *name){
    return invoke_syscall(SYSCALL_MBOX_OPEN, name, IGNORE, IGNORE, IGNORE);
}

void sys_mbox_close(int id){
    invoke_syscall(SYSCALL_MBOX_CLOSE, id, IGNORE, IGNORE, IGNORE);
}

int sys_mbox_send(int id, void *msg, int msg_length){
    return invoke_syscall(SYSCALL_MBOX_SEND, id, msg, msg_length, IGNORE);
}

int sys_mbox_recv(int id, void *msg, int msg_length){
    return invoke_syscall(SYSCALL_MBOX_RECV, id, msg, msg_length, IGNORE);
}

pid_t sys_exec(const char *file_name, int argc, char* argv[], spawn_mode_t mode)
{
        return invoke_syscall(SYSCALL_EXEC, (uintptr_t)file_name, argc, (uintptr_t)argv, mode);
}

pid_t sys_mthread_create(void (*start_routine)(void *), void *arg)
{
    return invoke_syscall(SYSCALL_MTHREAD_CREATE, start_routine, arg, IGNORE, IGNORE);
}

long sys_net_recv(uintptr_t addr, size_t length, int num_packet, size_t* frLength)
{
    return invoke_syscall(SYSCALL_NET_RECV, addr, length, num_packet, frLength);
}

void sys_net_send(uintptr_t addr, size_t length)
{
    invoke_syscall(SYSCALL_NET_SEND, addr, length, IGNORE, IGNORE);
}

void sys_net_irq_mode(int mode)
{
    invoke_syscall(SYSCALL_NET_IRQ_MODE, mode, IGNORE, IGNORE, IGNORE);
}

void sys_mkfs()
{
    invoke_syscall(SYSCALL_MKFS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_statfs()
{
    invoke_syscall(SYSCALL_STATFS, IGNORE, IGNORE, IGNORE, IGNORE);
}

void sys_cd(char *dirname)
{
    invoke_syscall(SYSCALL_CD, (uintptr_t)dirname, IGNORE, IGNORE, IGNORE);
}

void sys_mkdir(char *dirname)
{
    invoke_syscall(SYSCALL_MKDIR, (uintptr_t)dirname, IGNORE, IGNORE, IGNORE);
}

void sys_rmdir(char *dirname)
{
    invoke_syscall(SYSCALL_RMDIR, (uintptr_t)dirname, IGNORE, IGNORE, IGNORE);
}

void sys_ls(char *dirname)
{
    invoke_syscall(SYSCALL_LS, (uintptr_t)dirname, IGNORE, IGNORE, IGNORE);
}

void sys_touch(char *filename)
{
    invoke_syscall(SYSCALL_TOUCH, (uintptr_t)filename, IGNORE, IGNORE, IGNORE);
}

void sys_cat(char *filename)
{
    invoke_syscall(SYSCALL_CAT, (uintptr_t)filename, IGNORE, IGNORE, IGNORE);
}

int sys_fopen(char *name, int access)
{
    return invoke_syscall(SYSCALL_FILE_OPEN, (uintptr_t)name, access, IGNORE, IGNORE);
}

int sys_fread(int fd, char *buff, int size)
{
    return invoke_syscall(SYSCALL_FILE_READ, fd, (uintptr_t)buff, size, IGNORE);
}

void sys_fwrite(int fd, char *buff, int size)
{
    return invoke_syscall(SYSCALL_FILE_WRITE, fd, (uintptr_t)buff, size, IGNORE);
}

void sys_fclose(int fd)
{
    return invoke_syscall(SYSCALL_FILE_CLOSE, fd, IGNORE, IGNORE, IGNORE);
}

void sys_ln(char *source, char *link_name)
{
    invoke_syscall(SYSCALL_LN, (uintptr_t)source, (uintptr_t)link_name, IGNORE, IGNORE);
}

void sys_rm(char *filename)
{
    invoke_syscall(SYSCALL_RM, (uintptr_t)filename, IGNORE, IGNORE, IGNORE);
}

void sys_lseek(int fd, int offset, int whence)
{
    invoke_syscall(SYSCALL_LSEEK, fd, offset, whence, IGNORE);
}