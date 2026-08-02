# YiCore bare-metal runtime

File: `bare-metal-runtime.md`  
Function: Define the Zephyr-style YiCore execution contract.  
Author: Don  
Date: 2026-08-02  
Version: 1.0.0

YiCore uses Zephyr-style configuration, Devicetree, devices and west commands,
but it does not contain a scheduler, threads, kernel ticks, workqueues or
`k_*` compatibility shims.

The application lifecycle is:

```text
reset → SoC init → device init levels → main → poll/event loop → idle
```

A normal application loop is:

```c
for (;;) {
    (void)yi_poll();
    application_poll();
    yi_idle();
}
```

`yi_event_post()` may be called by an ISR to atomically set event bits. Event
handlers execute later from `yi_poll()` in main-loop context. Handlers and poll
hooks must be bounded and non-blocking. The fixed tables use no heap memory.

`yi_idle()` calls the weak `yi_arch_idle()` hook only when the previous poll
found no work and no event became pending. ARM and RISC-V defaults execute WFI.
Applications may override the hook when a board needs a different low-power
sequence.

Software timers remain driven by the monotonic `yi_system_uptime_ms()` clock.
Register them once with `yi_soft_timer_poll_register()`. Register queued log
flushing with `yi_log_poll_register()`. Timer callbacks and console writes then
run from the main loop, never from the tick ISR.

The supported west workflow is:

```powershell
west boards
west build -b <board> <application-or-product>
west flash -b <board> -d <build-dir>
west debugserver -b <board> -d <build-dir>
west test [root] -p <board>
west sysbuild <product-root>
```

Runner command generation supports OpenOCD, pyOCD, J-Link and WCH-Link. Use
`--dry-run` to inspect the exact external command. `sysbuild.yml` declares only
real image directories and orders them through `depends_on`; it does not copy
or rename the application image to simulate a bootloader.
