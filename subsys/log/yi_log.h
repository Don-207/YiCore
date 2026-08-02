/**
 * @file yi_log.h
 * @brief YiCore log interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_LOG_H
#define YI_LOG_H

#include <stdbool.h>

typedef enum
{
    YI_LOG_LEVEL_DEBUG = 0,
    YI_LOG_LEVEL_INFO,
    YI_LOG_LEVEL_WARNING,
    YI_LOG_LEVEL_ERROR,
    YI_LOG_LEVEL_NONE
} yi_log_level_t;

/**
 * @brief Set level.
 * @param level Initialization level.
 */
void yi_log_set_level(yi_log_level_t level);
/**
 * @brief Write the module.
 * @param level Initialization level.
 * @param message Message value.
 */
int yi_log_write(yi_log_level_t level, const char *message);
/**
 * @brief Perform the yi log debug operation.
 * @param message Message value.
 */
int yi_log_debug(const char *message);
/**
 * @brief Perform the yi log info operation.
 * @param message Message value.
 */
int yi_log_info(const char *message);
/**
 * @brief Perform the yi log warning operation.
 * @param message Message value.
 */
int yi_log_warning(const char *message);
/**
 * @brief Perform the yi log error operation.
 * @param message Message value.
 */
int yi_log_error(const char *message);

/** Flush one queued log frame from main-loop context. */
bool yi_log_process(void);

/** Register non-blocking log flushing with yi_poll(). */
int yi_log_poll_register(void);

#endif
