.section .text.startup
.global Reset_Handler
.type Reset_Handler, @function

Reset_Handler:
    .option push
    .option norelax
    la gp, __global_pointer$
    la sp, _stack_top
    .option pop
    la t0, _sidata
    la t1, _sdata
    la t2, _edata

copy_data:
    bgeu t1, t2, clear_bss
    lbu a0, 0(t0)
    sb a0, 0(t1)
    addi t0, t0, 1
    addi t1, t1, 1
    j copy_data


clear_bss:
    la t1, _sbss
    la t2, _ebss

clear_bss_loop:
    bgeu t1, t2, call_main
    sb zero, 0(t1)
    addi t1, t1, 1
    j clear_bss_loop


call_main:
    jal ra, main

hang:
    j hang

.size Reset_Handler, . - Reset_Handler