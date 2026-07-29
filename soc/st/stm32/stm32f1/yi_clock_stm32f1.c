/**
 * @file yi_clock_stm32f1.c
 * @brief YiCore clock stm32f1 implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_clock_stm32f1.h"
#include "stm32f1xx_hal.h"

/**
 * @brief Set the module.
 * @param id Id value.
 * @param enabled Enabled value.
 */
static int yi_clock_set(yi_stm32_clock_id_t id, bool enabled)
{
    switch(id)
    {
    case YI_STM32_CLOCK_GPIOA:
        if(enabled) { __HAL_RCC_GPIOA_CLK_ENABLE(); } else { __HAL_RCC_GPIOA_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_GPIOB:
        if(enabled) { __HAL_RCC_GPIOB_CLK_ENABLE(); } else { __HAL_RCC_GPIOB_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_GPIOC:
        if(enabled) { __HAL_RCC_GPIOC_CLK_ENABLE(); } else { __HAL_RCC_GPIOC_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_GPIOD:
        if(enabled) { __HAL_RCC_GPIOD_CLK_ENABLE(); } else { __HAL_RCC_GPIOD_CLK_DISABLE(); }
        break;
#if defined(GPIOE)
    case YI_STM32_CLOCK_GPIOE:
        if(enabled) { __HAL_RCC_GPIOE_CLK_ENABLE(); } else { __HAL_RCC_GPIOE_CLK_DISABLE(); }
        break;
#endif
#if defined(GPIOF)
    case YI_STM32_CLOCK_GPIOF:
        if(enabled) { __HAL_RCC_GPIOF_CLK_ENABLE(); } else { __HAL_RCC_GPIOF_CLK_DISABLE(); }
        break;
#endif
#if defined(GPIOG)
    case YI_STM32_CLOCK_GPIOG:
        if(enabled) { __HAL_RCC_GPIOG_CLK_ENABLE(); } else { __HAL_RCC_GPIOG_CLK_DISABLE(); }
        break;
#endif
    case YI_STM32_CLOCK_USART1:
        if(enabled) { __HAL_RCC_USART1_CLK_ENABLE(); } else { __HAL_RCC_USART1_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_USART2:
        if(enabled) { __HAL_RCC_USART2_CLK_ENABLE(); } else { __HAL_RCC_USART2_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_USART3:
        if(enabled) { __HAL_RCC_USART3_CLK_ENABLE(); } else { __HAL_RCC_USART3_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_SPI1:
        if(enabled) { __HAL_RCC_SPI1_CLK_ENABLE(); } else { __HAL_RCC_SPI1_CLK_DISABLE(); }
        break;
    case YI_STM32_CLOCK_SPI2:
        if(enabled) { __HAL_RCC_SPI2_CLK_ENABLE(); } else { __HAL_RCC_SPI2_CLK_DISABLE(); }
        break;
    default:
        return -1;
    }
    return 0;
}

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_clock_init(const void *config)
{
    const yi_clock_config_t *cfg = (const yi_clock_config_t *)config;
    return ((cfg != NULL) && (cfg->id < YI_STM32_CLOCK_COUNT)) ? 0 : -1;
}

/**
 * @brief Enable the module.
 * @param dev Device instance.
 */
int yi_clock_enable(yi_device_t *dev)
{
    const yi_clock_config_t *cfg;
    yi_clock_data_t *data;
    uint32_t primask;

    if(!yi_device_is_ready(dev) || (dev->config == NULL) || (dev->data == NULL))
    {
        return -1;
    }
    cfg = (const yi_clock_config_t *)dev->config;
    data = (yi_clock_data_t *)dev->data;
    primask = __get_PRIMASK();
    __disable_irq();
    if(data->reference_count == UINT16_MAX)
    {
        if(primask == 0U) { __enable_irq(); }
        return -1;
    }
    if((data->reference_count == 0U) && (yi_clock_set(cfg->id, true) != 0))
    {
        if(primask == 0U) { __enable_irq(); }
        return -1;
    }
    data->reference_count++;
    if(primask == 0U) { __enable_irq(); }
    return 0;
}

/**
 * @brief Disable the module.
 * @param dev Device instance.
 */
int yi_clock_disable(yi_device_t *dev)
{
    const yi_clock_config_t *cfg;
    yi_clock_data_t *data;
    uint32_t primask;

    if(!yi_device_is_ready(dev) || (dev->config == NULL) || (dev->data == NULL))
    {
        return -1;
    }
    cfg = (const yi_clock_config_t *)dev->config;
    data = (yi_clock_data_t *)dev->data;
    primask = __get_PRIMASK();
    __disable_irq();
    if(data->reference_count == 0U)
    {
        if(primask == 0U) { __enable_irq(); }
        return -1;
    }
    data->reference_count--;
    if((data->reference_count == 0U) && (yi_clock_set(cfg->id, false) != 0))
    {
        data->reference_count++;
        if(primask == 0U) { __enable_irq(); }
        return -1;
    }
    if(primask == 0U) { __enable_irq(); }
    return 0;
}

/**
 * @brief Get rate.
 * @param dev Device instance.
 */
uint32_t yi_clock_get_rate(yi_device_t *dev)
{
    const yi_clock_config_t *cfg;

    if((dev == NULL) || (dev->config == NULL))
    {
        return 0U;
    }
    cfg = (const yi_clock_config_t *)dev->config;
    switch(cfg->id)
    {
    case YI_STM32_CLOCK_USART1:
    case YI_STM32_CLOCK_SPI1:
        /**
         * @brief Perform the HAL RCC GetPCLK2Freq operation.
         */
        return HAL_RCC_GetPCLK2Freq();
    case YI_STM32_CLOCK_USART2:
    case YI_STM32_CLOCK_USART3:
    case YI_STM32_CLOCK_SPI2:
        /**
         * @brief Perform the HAL RCC GetPCLK1Freq operation.
         */
        return HAL_RCC_GetPCLK1Freq();
    case YI_STM32_CLOCK_GPIOA:
    case YI_STM32_CLOCK_GPIOB:
    case YI_STM32_CLOCK_GPIOC:
    case YI_STM32_CLOCK_GPIOD:
    case YI_STM32_CLOCK_GPIOE:
    case YI_STM32_CLOCK_GPIOF:
    case YI_STM32_CLOCK_GPIOG:
        /**
         * @brief Perform the HAL RCC GetHCLKFreq operation.
         */
        return HAL_RCC_GetHCLKFreq();
    default:
        return 0U;
    }
}
