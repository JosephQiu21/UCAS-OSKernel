# 实验总结

本文档记录一些值得存档的要点，以及在调试中获得的经验。

1. 一个调了很久的 bug：把 current_running (pid = 2) 加入 pid = 3 的 wait_list，但没有将对应 list 域加入，而是将 pcb 本身的指针加入；而 pcb 指针和 kernel_sp 指向相同地址。这使得 pcb[1].kernel_sp 被修改。