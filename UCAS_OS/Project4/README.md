# Project4

## 运行方法

1. 进入文件夹目录，在终端输入 `make all` 进行编译，检查是否正确生成 `image` 文件，且正确打印扇区信息。
2. 在母文件夹 `oslab` 中找到 `run_qemu.sh` 脚本，修改其中的镜像路径，并运行。
3. 在 qemu 模拟器中输入 `loadboot` 指令加载镜像，检查是否能正确加载内核，并实现对应功能。
4. 可上板验证：插入 SD 卡，在文件夹中运行 `make floppy` 指令，将 SD 卡插入开发板，用 minicom 连接并运行 `loadboot` 指令加载内核。