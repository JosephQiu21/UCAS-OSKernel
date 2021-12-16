# 实验总结

本文档记录一些值得存档的要点，以及在调试中获得的经验。

1. 在时钟中断切换到 shell 时嵌套了一个时钟中断，此时再 `do_scheduler` 时 `ready_queue` 就空了，于是调度到 `pid0_pcb` 去，因此卡住。这个 `pid0_pcb` 可以说没有用了，用 shell 的 pcb 就行。