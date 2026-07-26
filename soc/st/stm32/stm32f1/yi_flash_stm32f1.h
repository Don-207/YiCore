/**
 * @file yi_flash_stm32f1.h
 * @brief YiCore flash stm32f1 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_FLASH_STM32F1_H
#define YI_FLASH_STM32F1_H

#include "yi_flash.h"

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_stm32_flash_init(const void *config);
extern const yi_flash_api_t yi_stm32_flash_driver_api;

#define YI_STM32_FLASH_DEFINE_LEVEL(_name, _level, _priority, _config)     \
    YI_DEVICE_DEFINE_WITH_API(                                             \
        _name, _level, _priority, yi_stm32_flash_init,                     \
        &_config, NULL,                                                     \
        (const yi_device_api_t *)&yi_stm32_flash_driver_api                 \
    )

#endif
