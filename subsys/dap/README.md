# YiDAP

YiDAP provides a stable lifecycle boundary between YiLink products and a
CMSIS-DAP protocol engine. YiCore owns only the lifecycle and opaque backend
contract; products adapt CherryDAP or another engine without exposing its USB
headers, descriptors, command processor, or timing-sensitive SWD/JTAG code to
the common subsystem.

Products remain responsible for `DAP_config.h`, USB endpoint configuration,
GPIO/SPI pin timing, CDC UART callbacks, and CMSIS-DAP vendor commands. This
keeps hardware policy outside the reusable lifecycle wrapper.

The foreground contract is deliberately non-blocking: initialize once with
`yi_dap_init()`, then call `yi_dap_process()` continuously from the main loop.
