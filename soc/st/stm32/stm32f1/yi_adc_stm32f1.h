/**
 * @file yi_adc_stm32f1.h
 * @brief YiCore adc stm32f1 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_ADC_STM32F1_H
#define YI_ADC_STM32F1_H

#include "yi_adc.h"
#include "yi_stm32_periph.h"

typedef struct
{
    yi_device_t *self; /**< Self value. */
    ADC_TypeDef *instance; /**< Instance value. */
    yi_stm32_periph_clock_t clock; /**< Clock value. */
    yi_device_t *input_pin; /**< Input pin value. */
    uint8_t channel; /**< Channel value. */
    uint8_t sample_cycles; /**< Sample cycles value. */
    uint8_t clock_divider; /**< Clock divider value. */} yi_adc_stm32f1_config_t;

typedef struct
{
    uint8_t initialized; /**< Initialized value. */} yi_adc_stm32f1_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_adc_stm32f1_init(const void *config);
extern const yi_adc_api_t yi_adc_stm32f1_api;

#define YI_ADC_STM32F1_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                                \
        _name, _level, _priority, yi_adc_stm32f1_init,                       \
        &_config, &_data, (const yi_device_api_t *)&yi_adc_stm32f1_api       \
    )

#endif
