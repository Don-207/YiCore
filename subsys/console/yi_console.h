/**
 * @file yi_console.h
 * @brief YiCore console interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_CONSOLE_H
#define YI_CONSOLE_H

#include "yi_device.h"

typedef struct
{
    yi_device_t *self; /**< Self value. */
    yi_device_t *backend; /**< Backend value. */
    bool default_console; /**< Default console value. */} yi_console_config_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_console_init(const void *config);
/**
 * @brief Write the module.
 * @param console Console value.
 * @param buf Buf value.
 * @param len Len value.
 */
int yi_console_write(yi_device_t *console, const uint8_t *buf, uint32_t len);
/**
 * @brief Get default.
 */
yi_device_t *yi_console_get_default(void);
/**
 * @brief Set default.
 * @param console Console value.
 */
int yi_console_set_default(yi_device_t *console);

extern const yi_device_api_t yi_console_driver_api;

#define YI_CONSOLE_DEFINE_LEVEL(_name, _level, _priority, _config) \
    YI_DEVICE_DEFINE_WITH_API(                                  \
        _name, _level, _priority, yi_console_init,               \
        &_config, NULL, &yi_console_driver_api                   \
    )

#endif
