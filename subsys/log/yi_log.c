/**
 * @file yi_log.c
 * @brief YiCore log implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_log.h"
#include "yi_console.h"
#include <stddef.h>

#define YI_LOG_BUFFER_SIZE 192U

static yi_log_level_t minimum_level = YI_LOG_LEVEL_DEBUG;

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
    uint8_t frame[YI_LOG_BUFFER_SIZE];
    const char *prefix;
    yi_device_t *console;
    uint32_t length = 0U;

    if((message == NULL) || (level > YI_LOG_LEVEL_ERROR))
    {
        return -1;
    }
    if(level < minimum_level)
    {
        return 0;
    }
    console = yi_console_get_default();
    prefix = yi_log_prefix(level);
    if((console == NULL) || (prefix == NULL))
    {
        return -1;
    }
    while((*prefix != '\0') && (length < (YI_LOG_BUFFER_SIZE - 2U)))
    {
        frame[length++] = (uint8_t)*prefix++;
    }
    while((*message != '\0') && (length < (YI_LOG_BUFFER_SIZE - 2U)))
    {
        frame[length++] = (uint8_t)*message++;
    }
    frame[length++] = '\r';
    frame[length++] = '\n';
    /**
     * @brief Write the module.
     * @param console Console value.
     * @param frame Frame value.
     * @param length Number of bytes to process.
     */
    return yi_console_write(console, frame, length);
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
