/**
 * @file yi_system.h
 * @brief YiCore system interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_SYSTEM_H
#define YI_SYSTEM_H

#include <stdint.h>

/**
 * @brief Initialize the module.
 */
int yi_system_init(void);
/**
 * @brief Perform the yi system uptime ms operation.
 */
uint32_t yi_system_uptime_ms(void);
/**
 * @brief Perform the yi system uptime us operation.
 */
uint32_t yi_system_uptime_us(void);
/**
 * @brief Perform the yi system delay ms operation.
 * @param delay_ms Delay ms value.
 */
void yi_system_delay_ms(uint32_t delay_ms);
/**
 * @brief Perform the yi system delay us operation.
 * @param delay_us Delay us value.
 */
void yi_system_delay_us(uint32_t delay_us);
/**
 * @brief Perform the yi system irq lock operation.
 */
void yi_system_irq_lock(void);
/**
 * @brief Perform the yi system irq save operation.
 */
uint32_t yi_system_irq_save(void);
/**
 * @brief Perform the yi system irq restore operation.
 * @param key Key value.
 */
void yi_system_irq_restore(uint32_t key);

#endif
