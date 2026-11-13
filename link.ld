MEMORY
{
    FLASH (RX)  : ORIGIN = 0x00000000, LENGTH = 0x00040000
    SRAM  (RWX) : ORIGIN = 0x20200000, LENGTH = 0x00008000
}

ENTRY(reset_handler)

SECTIONS
{
    .text :
    {
        KEEP(*(.vectors))
        *(.text*)
        *(.rodata*)
    } > FLASH
}

_estack = ORIGIN(SRAM) + LENGTH(SRAM);
