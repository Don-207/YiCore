# Architecture layer

Architecture code contains CPU-core behavior shared by multiple silicon
vendors. Peripheral drivers do not belong here.

Examples include interrupt save/restore, memory barriers, reset primitives,
exception context and architecture-specific startup helpers. Existing
platforms may continue to provide these functions from their SoC backend while
the common contracts are extracted incrementally.

Do not expose CMSIS types through public YiCore driver APIs. This keeps a
future RISC-V backend possible without changing application code.
