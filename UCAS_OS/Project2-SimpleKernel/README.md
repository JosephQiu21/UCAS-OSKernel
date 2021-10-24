# Project2-SimpleKernel

本实验编写一个具备基础功能的内核。

任务一实现：

* 进程 PCB 初始化
* 任务上下文切换
* 通过引用内核函数实现系统调用
* 运行测试任务，测试调度功能

任务二实现：

* 互斥锁的初始化、申请、释放
* 运行测试任务，测试锁的抢占功能

任务三实现：

* 系统调用
* `sleep` 方法
* 将任务一/二的 `sys_yield`, `printf` 和锁的函数改造为内核函数的用户态系统调用。

任务四实现：

* 时钟中断和抢占式调度


## 文件树

``` bash

Project2-SimpleKernel
├── arch
│ └── riscv
│ ├── boot
│ │ └── bootblock.S # 引导块源代码，需要使⽤p1的⽀持加载⼤核的版本的代码
│ ├── include
│ │ ├── asm # 汇编⽂件中使⽤的头⽂件
│ │ │ ├── regs.h # 定义了⼀些寄存器在栈/结构体中的偏移
│ │ │ ├── sbiasm.h # 汇编调⽤SBI的宏的定义
│ │ │ └── sbidef.h # SBI调⽤号宏定义
│ │ ├── asm.h # ⽤于帮助定义汇编代码中的函数/过程的宏
│ │ ├── atomic.h # 原⼦指令的封装
│ │ ├── common.h # 串⼝输出的相关函数/常量的定义
│ │ ├── csr.h # CSR特权寄存器相关定义
│ │ └── sbi.h # SBI相关功能
│ ├── kernel
│ │ ├── entry.S # 上下⽂切换/例外处理及中断相关的功能
│ │ ├── head.S # 内核的汇编⼊⼝函数，建⽴C语⾔执⾏环境等相关功能
│ │ └── trap.S # 初始化例外处理相关寄存器
│ └── sbi
│ └── common.c # 串⼝输出的实现
├── drivers
│ ├── screen.c # 屏幕缓冲区相关函数的定义
│ └── screen.h # 屏幕缓冲区相关函数的实现
├── include
│ ├── assert.h # 断⾔功能的实现
│ ├── os
│ │ ├── irq.h # 例外处理相关函数和全局变量声明
│ │ ├── list.h # 链表相关函数声明
│ │ ├── lock.h # 锁相关函数和全局变量声明
│ │ ├── mm.h # 内存分配相关函数和全局变量声明
│ │ ├── sched.h # 任务管理/调度相关函数和全局变量声明
│ │ ├── string.h # 字符串处理相关函数和全局变量声明
│ │ ├── sync.h # 同步相关函数和全局变量声明
│ │ ├── syscall.h # 系统调⽤相关函数和全局变量声明
│ │ ├── syscall_number.h # 系统调⽤号宏定义
│ │ └── time.h # 时间相关函数声明
│ ├── stdarg.h # 变⻓参数相关的宏定义
│ ├── stdio.h # 内核中使⽤的stdio.h
│ ├── sys
│ │ └── syscall.h # （⽤⼾使⽤的）系统调⽤相关声明
│ └── type.h # 内核中使⽤的各种类型的定义
├── init
│ └── main.c # 内核C语⾔⼊⼝和各类初始化函数定义
├── kernel
│ ├── irq
│ │ └── irq.c # 例外处理相关函数和全局变量的定义
│ ├── locking
│ │ └── lock.c # 锁相关函数和全局变量的定义
│ ├── mm
│ │ └── mm.c # 内存分配相关函数和全局变量定义
│ ├── sched
│ │ ├── sched.c # 任务管理/调度相关函数和全局变量的定义
│ │ └── time.c # 时间相关函数的定义
│ └── syscall
│ └── syscall.c # 内核中系统调⽤相关处理函数的定义
├── libs
│ ├── printk.c # 内核printk实现
│ └── string.c # 内核中的字符串维护相关函数的实现
├── Makefile # Makefile⽂件
├── riscv.lds # linker script⽂件
├── test # 测试程序
│ ├── test.c # 测试程序⼊⼝/类型的定义
│ ├── test.h
│ └── test_project2 # 各个测试程序的相关声明和实现
│ ├── test2.h
│ ├── test_lock.c
│ ├── test_scheduler.c
│ ├── test_sleep.c
│ └── test_timer.c
├── tiny_libc # 迷你C库，给⽤⼾程序(测试程序)⽤的
│ ├── include
│ │ ├── assert.h
│ │ ├── mthread.h
│ │ ├── stdarg.h
│ │ ├── stdatomic.h
│ │ ├── stdbool.h
│ │ ├── stdint.h
│ │ ├── stdio.h
│ │ ├── string.h
│ │ └── time.h
│ ├── mthread.c # 锁相关的实现
│ ├── printf.c # printf相关的实现
│ ├── string.c
│ ├── syscall.c # 调⽤系统服务的封装
│ ├── syscall.S
│ └── time.c
└── tools
└── createimage.c

```

## 运行方法

1. 进入文件夹目录，在终端输入 `make all` 进行编译，检查是否正确生成 `image` 文件，且正确打印扇区信息。
2. 在母文件夹 `oslab` 中找到 `run_qemu.sh` 脚本，修改其中的镜像路径，并运行。
3. 在 qemu 模拟器中输入 `loadboot` 指令加载镜像，检查是否能正确加载内核，并实现对应功能。
4. 可上板验证：插入 SD 卡，在文件夹中运行 `make floppy` 指令，将 SD 卡插入开发板，用 minicom 连接并运行 `loadboot` 指令加载内核。