# Newlib port

The default syscall layer makes unsupported bare-metal file operations
deterministic and prevents toolchain-provided warning stubs from entering the
image.

The functions are weak. A platform or product may provide strong `_read`,
`_write`, `_close` or `_lseek` implementations when standard C streams should
use RTT, UART or a filesystem.
