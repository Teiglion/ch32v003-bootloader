ENTRY(Reset_Handler)

MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 1920
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 2K
}

_stack_top = ORIGIN(RAM)+LENGTH(RAM);

SECTIONS
{
    .text :
    {
        KEEP(*(.text.startup))
        *(.text .text.*)
    } > FLASH

    .rodata :
    {
        *(.rodata .rodata.*)
        *(.srodata .srodata.*)
    } > FLASH

    .data :
    {
        _sdata = .;
        PROVIDE(__global_pointer$ = . + 0x800);
        *(.data .data.*)
        *(.sdata .sdata.*)
        _edata = .;
    } > RAM AT > FLASH

    _sidata = LOADADDR(.data);

    .bss (NOLOAD) :
    {
        _sbss = .;
        *(.bss .bss.*)
        *(.sbss .sbss.*)
        *(COMMON)
        _ebss = .;
    } > RAM
}