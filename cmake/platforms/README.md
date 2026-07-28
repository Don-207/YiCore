# Platform build adapters

Each ready platform provides `<vendor>-<series>.cmake` and implements:

```cmake
yi_platform_application(NAME <name> SOURCES <sources...>)
```

The adapter owns toolchain flags, startup and linker selection, vendor sources,
SoC sources, DeviceTree generation and final image formats. Thin applications
remain unaware of these details.

Reserved platforms do not receive an adapter until they can produce and
validate a complete image.

Currently available:

- `st-stm32f1.cmake`: STM32F103xE GCC ELF, HEX and BIN images.

Reserved:

- `gigadevice-gd32f30x.cmake`: added only after the official package, exact
  target memory layout, startup source and first board are validated.
