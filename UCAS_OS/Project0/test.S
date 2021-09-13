.global main
main:
    li a1, 0	// a = 0
    li a2, 0	// i = 0
    li a3, 101
    // if (i >= 101) goto done
    bge a2, a3, done
loop:
    add a1, a1, a2
    addi a2, a2, 1
    // if (i < 101) goto loop
    blt a2, a3, loop
done:
    j done
