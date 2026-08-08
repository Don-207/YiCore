# YiDAP

YiDAP provides a stable lifecycle boundary between YiLink products and a
CMSIS-DAP protocol engine. YiCore owns the lifecycle, CMSIS-DAP command
processor, SWD transaction state machine, and raw JTAG/SWD sequence engines.
Products bind USB transport, target GPIO, timing, reset, and vendor commands
through `yi_dap_protocol_port.h`.

Products remain responsible for USB descriptors and endpoints, GPIO electrical
modes, CPU clock reporting, and CMSIS-DAP vendor commands. The common engine
does not include product or vendor SDK headers, so additional MCU ports can
reuse it without adopting the CH32H417 implementation.

The foreground contract is deliberately non-blocking: initialize once with
`yi_dap_init()`, then call `yi_dap_process()` continuously from the main loop.
Products can expose initialization diagnostics through `yi_dap_get_state()`
and `yi_dap_get_last_error()`. The normal transition is `UNINITIALIZED` to
`INITIALIZING` to `READY`; invalid configuration or a backend failure enters
`ERROR`. Calling `yi_dap_init()` again from `ERROR` retries initialization.
