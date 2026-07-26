/**
 * @file yi_clock.h
 * @brief YiCore clock interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_CLOCK_H
#define YI_CLOCK_H

#include "yi_device.h"

/**
 * @brief Enable the module.
 * @param dev Device instance.
 */
int yi_clock_enable(yi_device_t *dev);
/**
 * @brief Disable the module.
 * @param dev Device instance.
 */
int yi_clock_disable(yi_device_t *dev);
/**
 * @brief Get rate.
 * @param dev Device instance.
 */
uint32_t yi_clock_get_rate(yi_device_t *dev);

#endif
