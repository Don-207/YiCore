#ifndef YI_ADC_STM32F1_H
#define YI_ADC_STM32F1_H

#include "yi_adc.h"
#include "yi_stm32_periph.h"

typedef struct
{
    yi_device_t *self;
    ADC_TypeDef *instance;
    yi_stm32_periph_clock_t clock;
    yi_device_t *input_pin;
    uint8_t channel;
    uint8_t sample_cycles;
    uint8_t clock_divider;
} yi_adc_stm32f1_config_t;

typedef struct
{
    uint8_t initialized;
} yi_adc_stm32f1_data_t;

int yi_adc_stm32f1_init(const void *config);
extern const yi_adc_api_t yi_adc_stm32f1_api;

#define YI_ADC_STM32F1_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                                \
        _name, _level, _priority, yi_adc_stm32f1_init,                       \
        &_config, &_data, (const yi_device_api_t *)&yi_adc_stm32f1_api       \
    )

#endif
