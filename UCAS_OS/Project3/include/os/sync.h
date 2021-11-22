/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Copyright (C) 2018 Institute of Computing Technology, CAS
 *               Author : Han Shukai (email : hanshukai@ict.ac.cn)
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *            Synchronous primitive related content implementation,
 *                 such as: locks, barriers, semaphores, etc.
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *  * * * * * * * * * * *
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#ifndef INCLUDE_SYNC_H_
#define INCLUDE_SYNC_H_

#include <os/lock.h>
#include <os/list.h>

enum semaphore_op{
    UP,
    DOWN,
    SEMAPHORE_DESTROY
};

enum barrier_op{
    WAIT,
    BARRIER_DESTROY
};

typedef struct semaphore
{
    int val;
    list_head block_queue;
    int destoryed;
} semaphore_t;

typedef struct barrier
{
    int total_num;
    int wait_num;
    list_head block_queue;
    int destoryed;
} barrier_t;

void do_semaphore_init(semaphore_t *sem, int val);
void do_semaphore_up(semaphore_t *sem);
void do_semaphore_down(semaphore_t *sem);
void do_semaphore_destroy(semaphore_t *sem);

int semaphore_get(int key);
void semaphore_op(int handle, int op);
void semaphore_init(int handle, int val);

int semaphore_fetchhash(int key);

void do_barrier_init(barrier_t *bar, int count);
void do_barrier_wait(barrier_t *bar);
void do_barrier_destroy(barrier_t *bar);

void barrier_init(int handle, int count);
int barrier_get(int key);
void barrier_op(int handle, int op);

int barrier_fetchhash(int key);


#endif
