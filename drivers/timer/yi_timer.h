/**
 * @file yi_timer.h
 * @brief YiCore timer interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_TIMER_H
#define YI_TIMER_H

#include "yi_device.h"

/**
 * @brief Start the module.
 * @param dev Device instance.
 */
int yi_timer_start(yi_device_t *dev);
/**
 * @brief Stop the module.
 * @param dev Device instance.
 */
int yi_timer_stop(yi_device_t *dev);
/**
 * @brief Get period count.
 * @param dev Device instance.
 */
uint32_t yi_timer_get_period_count(const yi_device_t *dev);
/**
 * @brief Perform the yi timer irq handler operation.
 * @param dev Device instance.
 */
void yi_timer_irq_handler(yi_device_t *dev);

#endif
