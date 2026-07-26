/*
 * GNU ld fragment for the YiCore device registry.
 *
 * Add this file after the target's main linker script, for example:
 *   arm-none-eabi-gcc ... -TSTM32F103XEx_FLASH.ld -Tlinker/gcc/yi_device.ld
 */
SECTIONS
{
  .yi_device ALIGN(4) :
  {
    __yi_device_start = .;
    KEEP(*(.yi_device))
    __yi_device_end = .;
  }
  .yi_build_info ALIGN(4) :
  {
    KEEP(*(.yi_build_info))
  }
}
INSERT AFTER .rodata;
