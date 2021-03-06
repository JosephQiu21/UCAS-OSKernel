/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *                  The shell acts as a task running in user mode.
 *       The main function is to make system calls through the user's output.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this
 * software and associated documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * */

#include <test.h>
#include <string.h>
#include <os.h>
#include <sys/syscall.h>
#include <stdio.h>
#include <stdint.h>

struct task_info task_test_waitpid = {
    (uintptr_t)&wait_exit_task, USER_PROCESS};
struct task_info task_test_semaphore = {
    (uintptr_t)&semaphore_add_task1, USER_PROCESS};
struct task_info task_test_barrier = {
    (uintptr_t)&test_barrier, USER_PROCESS};
    
struct task_info strserver_task = {(uintptr_t)&strServer, USER_PROCESS};
struct task_info strgenerator_task = {(uintptr_t)&strGenerator, USER_PROCESS};

struct task_info task_test_multicore = {(uintptr_t)&test_multicore, USER_PROCESS};
struct task_info task_test_affinity = {(uintptr_t)&test_affinity, USER_PROCESS};

static struct task_info *test_tasks[16] = {&task_test_waitpid,
                                           &task_test_semaphore,
                                           &task_test_barrier,
                                           &task_test_multicore,
                                           &strserver_task, 
                                           &strgenerator_task
                                           };
static int num_test_tasks = 8;

#define SHELL_BEGIN 25
#define NUM_CMD 7

typedef void (*function)(void *arg0, void *arg1);

void shell_help();
void shell_man(char *cmd);
pid_t shell_exec(char *task_id);
void shell_kill(char *pid);
void shell_clear();
void shell_ps();
void shell_exit();

struct{
    char *name;
    char *description;
    char *usage;
    function func;
    int num_args;
} cmd_table [] = {
    {"man", "Manual for each command", "man [command]", &shell_man, 1},
    {"help", "Show all supported commands", "help [NO ARG]", &shell_help, 0},
    {"exec", "Execute task n", "exec [task id]", &shell_exec, 1},
    {"kill", "Kill process n", "kill [pid]", &shell_kill, 1},
    {"clear", "Clear the screen", "clear [NO ARG]", &shell_clear, 0},
    {"ps", "Show Process Table", "ps [NO ARG]", &shell_ps, 0},
    {"exit", "Exit current process", "exit [NO ARG], &shell_exit, 0"}
};

int atoi(char *c){
    int tid = 0;
    int i = 0;
    while (c[i]){
        tid = c[i] - '0' + tid * 10;
        i++;
    }
    return tid;
}

void shell_help(){
    printf("HERE YOU ARE!\n");
    printf("---------------\n");
    for (int i = 0; i < NUM_CMD; i++) {
        printf("%d. %s: %s\t%s\n", i + 1,  cmd_table[i].name, cmd_table[i].usage, cmd_table[i].description);
    }
}

void shell_man(char *cmd){
    int cmd_id = -1;
    for (int i = 0; i < NUM_CMD; i++) {
        if (strcmp(cmd, cmd_table[i].name) == 0)
            cmd_id = i;
    }
    if (cmd_id == -1) {
        printf("> [ERROR] Command not found! %s\n", cmd);
    }
    else {
        printf("%s: %s\t%s\n", cmd_table[cmd_id].name, cmd_table[cmd_id].usage, cmd_table[cmd_id].description);
    }
}

pid_t shell_exec(char *task_id){
    int id = atoi(task_id);
    int pid = -1;
    if (id >= num_test_tasks){
        printf("> [ERROR] Task not found!\n");
        return 0;
    }
    if (pid = sys_spawn(test_tasks[id], NULL, 1))
        printf("> Task execution succeed! PID: %d, MODE: %s\n", pid, "AUTO_CLEANUP_ON_EXIT");
    return pid;
}

void shell_kill(char *pid){
    pid_t id = atoi(pid);
    if (sys_kill(id))
        printf("> Kill process succeed!\n");
    
}

void shell_clear(){
    sys_screen_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- JOSEPH's SHELL -------------------\n");
}

void shell_ps(){
    sys_process_show();
}

void shell_exit(){
    sys_exit();
}

char read_uart(){
    char c;
    int tmp;
    while ((tmp = sys_get_char()) == -1);
    c = (char)tmp;
    return c;
}

void parse(char *cmd){
    int cmd_id;
    char arg[10][10] = {0};
    char *parse = cmd;
    int i = 0, j = 0, k = 0;
    // k: scan parse
    while (parse[k] != '\0'){
        if (parse[k] != ' ')
            arg[i][j++] = parse[k];
        else {
            arg[i][j] = '\0';
            i++;
            j = 0;
        }
        k++;
    }
    
    cmd_id = -1;
    for (int t = 0; t < NUM_CMD; t++) {
        if (strcmp(arg[0], cmd_table[t].name) == 0)
            cmd_id = t;
    }

    if (cmd_id == -1) {
        printf("> [ERROR] Command not found! %s\n", arg[0]);
    }

    else if (cmd_table[cmd_id].num_args != i) {
        printf("> [ERROR] Wrong number of arguments! Required: %d Input: %d\n", cmd_table[cmd_id].num_args, i);
        printf("> Usage: %s\n", cmd_table[cmd_id].usage);
    }

    else {
        cmd_table[cmd_id].func(arg[1], arg[2]);
    }

    return;
}

void test_shell()
{
    sys_screen_clear();
    sys_move_cursor(1, SHELL_BEGIN);
    printf("------------------- JOSEPH's SHELL -------------------\n");
    printf("> root@UCAS_OS: ");

    char ch;
    int len;
    char cmd[10] = {0};
    while (1)
    {
        while (1){
            // call syscall to read UART port
            ch = read_uart();

            // Get `Enter`: parse command, flush cmd buffer, break
            if (ch == '\r'){
                printf("\n");
                cmd[len] = '\0';
                parse(cmd);
                len = 0;
                break;
            }

            // Get `Backspace`: change last character to 0
            else if (ch == 8 || ch == 127){
                if (len > 0) {
                    cmd[len--] = 0;
                    sys_serial_write(ch);
                }
            }

            else {
                cmd[len++] = ch;
                sys_serial_write(ch);
            }
        }
        printf("> root@UCAS_OS: ");
        sys_reflush();
    }

}

