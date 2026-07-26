/**
 * @file yi_rtt.h
 * @brief YiCore rtt interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_RTT_H
#define YI_RTT_H

#include "yi_device.h"

typedef enum
{
    YI_RTT_MODE_NO_BLOCK_SKIP = 0,
    YI_RTT_MODE_NO_BLOCK_TRIM,
    YI_RTT_MODE_BLOCK
} yi_rtt_mode_t;

typedef struct
{
    uint32_t up_buffer; /**< Up buffer value. */
    yi_rtt_mode_t mode; /**< Mode value. */} yi_rtt_config_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_rtt_init(const void *config);
extern const yi_device_api_t yi_rtt_driver_api;

#define YI_RTT_DEFINE_LEVEL(_name, _level, _priority, _config) \
    YI_DEVICE_DEFINE_WITH_API(                              \
        _name, _level, _priority, yi_rtt_init,              \
        &_config, NULL, &yi_rtt_driver_api                  \
    )

#endif
