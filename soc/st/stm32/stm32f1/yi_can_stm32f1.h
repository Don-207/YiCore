/**
 * @file yi_can_stm32f1.h
 * @brief YiCore can stm32f1 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_CAN_STM32F1_H
#define YI_CAN_STM32F1_H

#include "yi_can.h"
#include "yi_stm32_periph.h"

typedef struct
{
    yi_device_t *self; /**< Self value. */
    CAN_TypeDef *instance; /**< Instance value. */
    yi_stm32_periph_clock_t clock; /**< Clock value. */
    yi_device_t *tx_pin; /**< Tx pin value. */
    yi_device_t *rx_pin; /**< Rx pin value. */
    uint32_t bitrate; /**< Bitrate value. */
    uint16_t sample_point; /**< Sample point value. */
    IRQn_Type tx_irqn; /**< Tx irqn value. */
    IRQn_Type rx0_irqn; /**< Rx0 irqn value. */
    IRQn_Type rx1_irqn; /**< Rx1 irqn value. */
    IRQn_Type sce_irqn; /**< Sce irqn value. */
    uint8_t irq_priority; /**< Irq priority value. */} yi_can_config_t;

typedef struct
{
    CAN_HandleTypeDef hcan; /**< Hcan value. */} yi_can_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_can_init(const void *config);

#define YI_CAN_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_can_init,  \
                              &_config, &_data, NULL)

#endif
