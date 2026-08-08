/**
 * @file yi_clock_stm32.c
 * @brief Implement shared STM32 GPIO and basic peripheral clock control.
 * @author Don
 * @date 2026-07-31
 * @version 1.0.0
 */

#include "yi_clock_stm32.h"

#if defined(STM32F407xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32H743xx)
#include "stm32h7xx_hal.h"
#else
#error "yi_clock_stm32.c requires a supported STM32 device define"
#endif

/** @brief Enable or disable one clock selected by its stable YiCore ID. */
static int yi_clock_set(yi_stm32_clock_id_t id, bool enabled)
{
    switch(id)
    {
    case YI_STM32_CLOCK_GPIOA:
        if(enabled) { __HAL_RCC_GPIOA_CLK_ENABLE(); }
        else { __HAL_RCC_GPIOA_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_GPIOB:
        if(enabled) { __HAL_RCC_GPIOB_CLK_ENABLE(); }
        else { __HAL_RCC_GPIOB_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_GPIOC:
        if(enabled) { __HAL_RCC_GPIOC_CLK_ENABLE(); }
        else { __HAL_RCC_GPIOC_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_GPIOD:
        if(enabled) { __HAL_RCC_GPIOD_CLK_ENABLE(); }
        else { __HAL_RCC_GPIOD_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_GPIOE:
        if(enabled) { __HAL_RCC_GPIOE_CLK_ENABLE(); }
        else { __HAL_RCC_GPIOE_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_GPIOF:
        if(enabled) { __HAL_RCC_GPIOF_CLK_ENABLE(); }
        else { __HAL_RCC_GPIOF_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_GPIOG:
        if(enabled) { __HAL_RCC_GPIOG_CLK_ENABLE(); }
        else { __HAL_RCC_GPIOG_CLK_DISABLE(); }
        break;
    default:
        return -1;
    }
    return 0;
}

/** @brief Validate one board-generated STM32 clock configuration. */
int yi_clock_init(const void *config)
{
    const yi_clock_config_t *cfg = config;
    return ((cfg != NULL) && (cfg->id < YI_STM32_CLOCK_COUNT)) ? 0 : -1;
}

/** @brief Acquire and enable a clock on the first reference. */
int yi_clock_enable(yi_device_t *dev)
{
    const yi_clock_config_t *cfg;
    yi_clock_data_t *data;
    uint32_t key;

    if(!yi_device_is_ready(dev) || (dev->config == NULL) || (dev->data == NULL))
    {
        return -1;
    }
    cfg = dev->config;
    data = dev->data;
    key = __get_PRIMASK();
    __disable_irq();
    if((data->reference_count == UINT16_MAX) ||
       ((data->reference_count == 0U) && (yi_clock_set(cfg->id, true) != 0)))
    {
        if(key == 0U) { __enable_irq(); }
        return -1;
    }
    data->reference_count++;
    if(key == 0U) { __enable_irq(); }
    return 0;
}

/** @brief Release and disable a clock after the final reference. */
int yi_clock_disable(yi_device_t *dev)
{
    const yi_clock_config_t *cfg;
    yi_clock_data_t *data;
    uint32_t key;

    if(!yi_device_is_ready(dev) || (dev->config == NULL) || (dev->data == NULL))
    {
        return -1;
    }
    cfg = dev->config;
    data = dev->data;
    key = __get_PRIMASK();
    __disable_irq();
    if(data->reference_count == 0U)
    {
        if(key == 0U) { __enable_irq(); }
        return -1;
    }
    data->reference_count--;
    if((data->reference_count == 0U) && (yi_clock_set(cfg->id, false) != 0))
    {
        data->reference_count++;
        if(key == 0U) { __enable_irq(); }
        return -1;
    }
    if(key == 0U) { __enable_irq(); }
    return 0;
}

/** @brief Return the HCLK rate used by currently supported GPIO clocks. */
uint32_t yi_clock_get_rate(yi_device_t *dev)
{
    return (dev != NULL) ? HAL_RCC_GetHCLKFreq() : 0U;
}
