/**
 * @file yi_log.c
 * @brief YiCore log implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_log.h"
#include "yi_console.h"
#include "yi_poll.h"
#include <stddef.h>

#define YI_LOG_BUFFER_SIZE 192U
#define YI_LOG_QUEUE_DEPTH 8U

typedef struct
{
    uint8_t data[YI_LOG_BUFFER_SIZE]; /**< Complete formatted log frame. */
    uint16_t length; /**< Valid bytes in the frame. */
} yi_log_frame_t;

static yi_log_level_t minimum_level = YI_LOG_LEVEL_DEBUG;
static yi_log_frame_t log_queue[YI_LOG_QUEUE_DEPTH];
static volatile uint8_t log_write_index;
static volatile uint8_t log_read_index;

/**
 * @brief Perform the yi log prefix operation.
 * @param level Initialization level.
 */
static const char *yi_log_prefix(yi_log_level_t level)
{
    static const char * const prefixes[] =
    {
        "[DEBUG] ", "[INFO] ", "[WARN] ", "[ERROR] "
    };
    return (level <= YI_LOG_LEVEL_ERROR) ? prefixes[level] : NULL;
}

/**
 * @brief Set level.
 * @param level Initialization level.
 */
void yi_log_set_level(yi_log_level_t level)
{
    if(level <= YI_LOG_LEVEL_NONE)
    {
        minimum_level = level;
    }
}

/**
 * @brief Write the module.
 * @param level Initialization level.
 * @param message Message value.
 */
int yi_log_write(yi_log_level_t level, const char *message)
{
    yi_log_frame_t *frame;
    const char *prefix;
    uint32_t length = 0U;
    uint8_t write_index;
    uint8_t next_index;

    if((message == NULL) || (level > YI_LOG_LEVEL_ERROR))
    {
        return -1;
    }
    if(level < minimum_level)
    {
        return 0;
    }
    prefix = yi_log_prefix(level);
    if(prefix == NULL)
    {
        return -1;
    }
    write_index = __atomic_load_n(&log_write_index, __ATOMIC_RELAXED);
    next_index = (uint8_t)((write_index + 1U) % YI_LOG_QUEUE_DEPTH);
    if(next_index == __atomic_load_n(&log_read_index, __ATOMIC_ACQUIRE))
    {
        return -1;
    }
    frame = &log_queue[write_index];
    while((*prefix != '\0') && (length < (YI_LOG_BUFFER_SIZE - 2U)))
    {
        frame->data[length++] = (uint8_t)*prefix++;
    }
    while((*message != '\0') && (length < (YI_LOG_BUFFER_SIZE - 2U)))
    {
        frame->data[length++] = (uint8_t)*message++;
    }
    frame->data[length++] = '\r';
    frame->data[length++] = '\n';
    frame->length = (uint16_t)length;
    __atomic_store_n(&log_write_index, next_index, __ATOMIC_RELEASE);
    return (int)length;
}

bool yi_log_process(void)
{
    uint8_t read_index = __atomic_load_n(&log_read_index, __ATOMIC_RELAXED);
    yi_device_t *console;
    yi_log_frame_t *frame;

    if(read_index == __atomic_load_n(&log_write_index, __ATOMIC_ACQUIRE))
    {
        return false;
    }
    console = yi_console_get_default();
    if(console == NULL)
    {
        return false;
    }
    frame = &log_queue[read_index];
    if(yi_console_write(console, frame->data, frame->length) < 0)
    {
        return false;
    }
    __atomic_store_n(
        &log_read_index,
        (uint8_t)((read_index + 1U) % YI_LOG_QUEUE_DEPTH),
        __ATOMIC_RELEASE
    );
    return true;
}

static bool yi_log_poll(void *user_data)
{
    (void)user_data;
    return yi_log_process();
}

int yi_log_poll_register(void)
{
    return yi_poll_hook_register(yi_log_poll, NULL);
}

/**
 * @brief Perform the yi log debug operation.
 * @param message Message value.
 */
int yi_log_debug(const char *message)   { return yi_log_write(YI_LOG_LEVEL_DEBUG, message); }
/**
 * @brief Perform the yi log info operation.
 * @param message Message value.
 */
int yi_log_info(const char *message)    { return yi_log_write(YI_LOG_LEVEL_INFO, message); }
/**
 * @brief Perform the yi log warning operation.
 * @param message Message value.
 */
int yi_log_warning(const char *message) { return yi_log_write(YI_LOG_LEVEL_WARNING, message); }
/**
 * @brief Perform the yi log error operation.
 * @param message Message value.
 */
int yi_log_error(const char *message)   { return yi_log_write(YI_LOG_LEVEL_ERROR, message); }
