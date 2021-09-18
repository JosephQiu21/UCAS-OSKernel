# Project1-Bootloader

本实验编写 bootloader 进行操作系统引导，并在初始化之后实现简单的输入字符回显功能。

将 bootloader 和 kernel 写入到 image 文件，并加载到 SD 卡上，可在开发板的 Nutshell 核上运行操作系统内核。

## 文件树

以下只列出修改过的文件：

```bash
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
```

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

## 运行方法

1. 选择需要运行的内核，进入对应文件夹。
2. 在终端输入 `make all` 进行编译，检查是否正确生成 `image` 文件，且正确打印扇区信息。
3. 在母文件夹 `oslab` 中找到 `run_qemu.sh` 脚本，修改其中的镜像路径，并运行。
4. 在 qemu 模拟器中输入 `loadboot` 指令加载镜像，检查是否能正确加载内核，且实现字符回显的操作系统功能。
5. 可上板验证：插入 SD 卡，在对应文件夹 `X-core` 中运行 `make floppy` 指令，将 SD 卡插入开发板，用 minicom 连接并运行 `loadboot` 指令加载内核。

