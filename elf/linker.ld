ENTRY(Reset_Handler)

MEMORY
{
    FLASH(rx) : ORIGIN = 0x00000000, LENGTH = 16K
    RAM(rwx)  : ORIGIN = 0x20000000, LENGTH = 4K
}

SECTIONS
{
    .text :
    {
        KEEP(*(.isr_vector))
        *(.text*)
        *(.rodata*)
        _etext = .;
    } > FLASH

    /* .data load address */
    _sidata = LOADADDR(.data);

    .data : 
    {
        _sdata = .;
        *(.data*)
        _edata = .;
    } > RAM AT > FLASH

    .bss :
    {
        _sbss = .;
        *(.bss*)
        *(COMMON)
        _ebss = .;
    } > RAM

    /* Heap region */
    _sheap = .;
    _eheap = ORIGIN(RAM) + LENGTH(RAM) - 0x400;

    /* Stack top */
    _estack = ORIGIN(RAM) + LENGTH(RAM);
}