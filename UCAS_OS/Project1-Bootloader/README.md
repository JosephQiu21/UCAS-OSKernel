# Project1-Bootloader

本实验编写 bootloader 进行操作系统引导，并在初始化之后实现简单的输入字符回显功能。

将 bootloader 和 kernel 写入到 image 文件，并加载到 SD 卡上，可在开发板的 Nutshell 核上运行操作系统内核。

## 文件树

以下只列出修改过的文件：

├── A-core
│   ├── bootblock.S
│   ├── createimage.c
│   ├── head.S
│   ├── kernel.c
├── C-core
│   ├── bootblock.S
│   ├── createimage.c
│   ├── head.S
│   ├── kernel-A.c
│   ├── kernel-B.c
│   ├── Makefile
└── S-core
    ├── bootblock.S
    ├── createimage.c
    ├── head.S
    ├── kernel.c	

## S-core

* Bootloader 将 kernel 加载到 0x50201000 处，然后跳转至 kernel 入口进行初始化等操作。

* Createimage 将 bootloader、kernel 文件写入到 image 文件中。

## A-core

与 S-core 相比：

* 添加对大内核（超过一个扇区）的支持。
* 通过拷贝 bootloader 至空闲地址并跳转，实现将 kernel 写入原 bootloader 地址。

## C-core

与 A-core 相比：

* 修改 createimage，将两个 kernel 写入到 image 中。
* 支持在 bootloader 中选择加载双系统之一。

