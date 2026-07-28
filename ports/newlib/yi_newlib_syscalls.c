/**
 * @file yi_newlib_syscalls.c
 * @brief Provide deterministic bare-metal Newlib syscall fallbacks.
 * @author Don
 * @date 2026-07-28
 * @version 1.0.0
 */

#include <stddef.h>
#include <sys/types.h>

#if defined(__GNUC__)
#define YI_NEWLIB_WEAK __attribute__((weak))
#else
#define YI_NEWLIB_WEAK
#endif

/**
 * @brief Reject closing a host file descriptor on bare metal.
 * @param file File descriptor supplied by Newlib.
 * @return Always -1 because no host filesystem is attached.
 * @note A platform may override this weak function.
 */
YI_NEWLIB_WEAK int _close(int file)
{
    (void)file;
    return -1;
}

/**
 * @brief Reject repositioning a host file descriptor on bare metal.
 * @param file File descriptor supplied by Newlib.
 * @param offset Requested byte offset.
 * @param whence Requested seek origin.
 * @return Always -1 because no host filesystem is attached.
 * @note A platform may override this weak function.
 */
YI_NEWLIB_WEAK off_t _lseek(int file, off_t offset, int whence)
{
    (void)file;
    (void)offset;
    (void)whence;
    return (off_t)-1;
}

/**
 * @brief Reject reading from a host file descriptor on bare metal.
 * @param file File descriptor supplied by Newlib.
 * @param buffer Destination buffer that remains unchanged.
 * @param length Requested byte count.
 * @return Always -1 because no default input stream is attached.
 * @note A console backend may override this weak function.
 */
YI_NEWLIB_WEAK ssize_t _read(int file, void *buffer, size_t length)
{
    (void)file;
    (void)buffer;
    (void)length;
    return (ssize_t)-1;
}

/**
 * @brief Reject writing to a host file descriptor on bare metal.
 * @param file File descriptor supplied by Newlib.
 * @param buffer Source bytes that are not consumed.
 * @param length Requested byte count.
 * @return Always -1 because no default output stream is attached.
 * @note An RTT or UART console may override this weak function.
 */
YI_NEWLIB_WEAK ssize_t _write(
    int file,
    const void *buffer,
    size_t length
)
{
    (void)file;
    (void)buffer;
    (void)length;
    return (ssize_t)-1;
}
