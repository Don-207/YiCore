/**
 * @file yi_adc_stm32f1.c
 * @brief YiCore adc stm32f1 implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_adc_stm32f1.h"
#include "yi_pinmux.h"

/**
 * @brief Perform the yi adc clock bits operation.
 * @param divider Divider value.
 */
static uint32_t yi_adc_clock_bits(uint8_t divider)
{
    switch(divider)
    {
    case 2U: return RCC_CFGR_ADCPRE_DIV2;
    case 4U: return RCC_CFGR_ADCPRE_DIV4;
    case 6U: return RCC_CFGR_ADCPRE_DIV6;
    case 8U: return RCC_CFGR_ADCPRE_DIV8;
    default: return UINT32_MAX;
    }
}

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_adc_stm32f1_init(const void *config)
{
    const yi_adc_stm32f1_config_t *cfg = config;
    yi_adc_stm32f1_data_t *data;
    uint32_t clock_bits;
    uint32_t timeout;

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->instance == NULL) ||
       (cfg->self->data == NULL) || !yi_device_is_ready(cfg->input_pin) ||
       (cfg->channel > 15U) || (cfg->sample_cycles > 7U))
    {
        return -1;
    }
    clock_bits = yi_adc_clock_bits(cfg->clock_divider);
    if((clock_bits == UINT32_MAX) ||
       ((HAL_RCC_GetPCLK2Freq() / cfg->clock_divider) > 14000000U))
    {
        return -1;
    }
    if(yi_stm32_periph_clock_enable(&cfg->clock) != 0)
    {
        return -1;
    }
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_ADCPRE) | clock_bits;
    if(yi_pinmux_apply(cfg->input_pin) != 0)
    {
        (void)yi_stm32_periph_clock_disable(&cfg->clock);
        return -1;
    }

    cfg->instance->CR1 = 0U;
    cfg->instance->CR2 = ADC_CR2_ADON;
    cfg->instance->SQR1 = 0U;
    cfg->instance->SQR2 = 0U;
    cfg->instance->SQR3 = cfg->channel;
    if(cfg->channel < 10U)
    {
        cfg->instance->SMPR2 = cfg->sample_cycles << (cfg->channel * 3U);
    }
    else
    {
        cfg->instance->SMPR1 = cfg->sample_cycles << ((cfg->channel - 10U) * 3U);
    }

    cfg->instance->CR2 |= ADC_CR2_RSTCAL;
    timeout = 1000000U;
    while(((cfg->instance->CR2 & ADC_CR2_RSTCAL) != 0U) && (--timeout != 0U)) { }
    if(timeout == 0U) { goto fail; }
    cfg->instance->CR2 |= ADC_CR2_CAL;
    timeout = 1000000U;
    while(((cfg->instance->CR2 & ADC_CR2_CAL) != 0U) && (--timeout != 0U)) { }
    if(timeout == 0U) { goto fail; }

    data = (yi_adc_stm32f1_data_t *)cfg->self->data;
    data->initialized = 1U;
    return 0;

fail:
    cfg->instance->CR2 = 0U;
    (void)yi_stm32_periph_clock_disable(&cfg->clock);
    return -1;
}

/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param value Value to process.
 * @param timeout_ms Operation timeout in milliseconds.
 */
static int yi_adc_stm32f1_read(yi_device_t *dev, uint16_t *value,
                               uint32_t timeout_ms)
{
    const yi_adc_stm32f1_config_t *cfg;
    uint32_t start;

    if((dev == NULL) || (dev->config == NULL) || (dev->data == NULL) ||
       (((yi_adc_stm32f1_data_t *)dev->data)->initialized == 0U))
    {
        return -1;
    }
    cfg = (const yi_adc_stm32f1_config_t *)dev->config;
    cfg->instance->SR &= ~ADC_SR_EOC;
    cfg->instance->CR2 |= ADC_CR2_EXTTRIG | ADC_CR2_EXTSEL | ADC_CR2_SWSTART;
    start = HAL_GetTick();
    while((cfg->instance->SR & ADC_SR_EOC) == 0U)
    {
        if((HAL_GetTick() - start) >= timeout_ms) { return -1; }
    }
    *value = (uint16_t)cfg->instance->DR;
    return 0;
}

const yi_adc_api_t yi_adc_stm32f1_api =
{
    .read = yi_adc_stm32f1_read,
    .read_channel = NULL
};
