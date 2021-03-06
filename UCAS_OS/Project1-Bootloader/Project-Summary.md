# 实验总结

本文档记录一些值得存档的要点，以及在调试中获得的经验。

1. 开发板的 I-cache 可能导致运行的代码与内存中代码不一致，因此必要的时候应该及时用 `fence.i` 更新。
2. 若需要跳转到不同文件中的入口，避免用 `j` 跳转，而是用 `la` 加载地址，再用 `jr` 跳转。
3. 利用好 `objdump` 工具：用 `-d` 查看反汇编代码；用 `hexdump` 查看 `image` 的数据排布情况等。
4. 一种较为实用的调试方法：在不同位置设置死循环，观察是否产生错误信息，可定位错误位置。
5. 注意多字节数在内存中的小端序存储。 