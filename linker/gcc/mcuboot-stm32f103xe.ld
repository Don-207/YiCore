/*
 * File: mcuboot-stm32f103xe.ld
 * Function: Link MCUboot into the first 48 KiB of STM32F103xE flash.
 * Author: Don
 * Date: 2026-07-28
 * Version: 1.0.0
 */

ENTRY(Reset_Handler)
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x08000000, LENGTH = 0x0000B800
    BUILDINFO (rx) : ORIGIN = 0x0800B800, LENGTH = 0x00000800
    RAM   (xrw) : ORIGIN = 0x20000000, LENGTH = 48K
}
_estack = ORIGIN(RAM) + LENGTH(RAM);
_Min_Heap_Size = 0x000;
_Min_Stack_Size = 0x400;
SECTIONS
{
    .isr_vector : { . = ALIGN(4); KEEP(*(.isr_vector)) . = ALIGN(4); } > FLASH
    .text :
    {
        . = ALIGN(4); *(.text .text*) *(.glue_7) *(.glue_7t) *(.eh_frame)
        KEEP(*(.init)) KEEP(*(.fini)) . = ALIGN(4); _etext = .;
    } > FLASH
    .rodata : { . = ALIGN(4); *(.rodata .rodata*) . = ALIGN(4); } > FLASH
    .yi_device :
    {
        . = ALIGN(4); __yi_device_start = .; KEEP(*(.yi_device));
        __yi_device_end = .; . = ALIGN(4);
    } > FLASH
    .yi_build_info : { KEEP(*(.yi_build_info)) } > BUILDINFO
    .ARM.extab : { *(.ARM.extab* .gnu.linkonce.armextab.*) } > FLASH
    .ARM.exidx : { __exidx_start = .; *(.ARM.exidx*) __exidx_end = .; } > FLASH
    .preinit_array :
    {
        PROVIDE_HIDDEN(__preinit_array_start = .); KEEP(*(.preinit_array*))
        PROVIDE_HIDDEN(__preinit_array_end = .);
    } > FLASH
    .init_array :
    {
        PROVIDE_HIDDEN(__init_array_start = .); KEEP(*(SORT(.init_array.*)))
        KEEP(*(.init_array*)) PROVIDE_HIDDEN(__init_array_end = .);
    } > FLASH
    .fini_array :
    {
        PROVIDE_HIDDEN(__fini_array_start = .); KEEP(*(SORT(.fini_array.*)))
        KEEP(*(.fini_array*)) PROVIDE_HIDDEN(__fini_array_end = .);
    } > FLASH
    _sidata = LOADADDR(.data);
    .data :
    {
        . = ALIGN(4); _sdata = .; *(.data .data*) . = ALIGN(4); _edata = .;
    } > RAM AT > FLASH
    .bss (NOLOAD) :
    {
        . = ALIGN(4); _sbss = .; *(.bss .bss*) *(COMMON)
        . = ALIGN(4); _ebss = .;
    } > RAM
    ._user_heap_stack (NOLOAD) :
    {
        . = ALIGN(8); PROVIDE(end = .); PROVIDE(_end = .);
        . += _Min_Heap_Size; . += _Min_Stack_Size; . = ALIGN(8);
    } > RAM
    /DISCARD/ : { *(.note.GNU-stack) }
}
