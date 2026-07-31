/**
 * @file yi_clock_stm32.h
 * @brief Define shared STM32 peripheral clock identifiers and devices.
 * @author Don
 * @date 2026-07-31
 * @version 1.0.0
 */

#ifndef YI_CLOCK_STM32_H
#define YI_CLOCK_STM32_H

#include "yi_clock.h"

/** @brief STM32 clocks currently exposed through board DeviceTree nodes. */
typedef enum
{
    YI_STM32_CLOCK_GPIOA = 0,
    YI_STM32_CLOCK_GPIOB,
    YI_STM32_CLOCK_GPIOC,
    YI_STM32_CLOCK_GPIOD,
    YI_STM32_CLOCK_GPIOE,
    YI_STM32_CLOCK_GPIOF,
    YI_STM32_CLOCK_GPIOG,
    YI_STM32_CLOCK_USART1,
    YI_STM32_CLOCK_USART2,
    YI_STM32_CLOCK_USART3,
    YI_STM32_CLOCK_SPI1,
    YI_STM32_CLOCK_SPI2,
    YI_STM32_CLOCK_COUNT
} yi_stm32_clock_id_t;

/** @brief Immutable configuration for one STM32 peripheral clock. */
typedef struct
{
    yi_stm32_clock_id_t id; /**< Hardware clock selected by the board. */
} yi_clock_config_t;

/** @brief Mutable reference-count state for one STM32 clock device. */
typedef struct
{
    uint16_t reference_count; /**< Number of active peripheral consumers. */
} yi_clock_data_t;

/** @brief Validate an STM32 clock device configuration. */
int yi_clock_init(const void *config);

#define YI_CLOCK_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                         \
        _name, _level, _priority, yi_clock_init,                       \
        &_config, &_data, NULL                                         \
    )

#endif
