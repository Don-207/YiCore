/**
 * @file yi_gpio_stm32.c
 * @brief Implement polling GPIO for STM32F407 and STM32H743.
 * @author Don
 * @date 2026-07-31
 * @version 1.0.0
 */

#include "yi_clock.h"
#include "yi_gpio.h"

#if defined(STM32F407xx)
#include "stm32f4xx_hal.h"
#elif defined(STM32H743xx)
#include "stm32h7xx_hal.h"
#else
#error "yi_gpio_stm32.c requires a supported STM32 device define"
#endif

/** @brief Convert the YiCore pull setting to its STM32 HAL value. */
static uint32_t yi_gpio_hal_pull(yi_gpio_pull_t pull)
{
    if(pull == YI_GPIO_PULL_UP) { return GPIO_PULLUP; }
    if(pull == YI_GPIO_PULL_DOWN) { return GPIO_PULLDOWN; }
    return GPIO_NOPULL;
}

/** @brief Configure one non-interrupt GPIO from its DeviceTree data. */
int yi_gpio_init(const void *config)
{
    const yi_gpio_config_t *cfg = config;
    GPIO_InitTypeDef gpio = {0};

    if((cfg == NULL) || (cfg->port == NULL) || (cfg->pin == 0U) ||
       (cfg->interrupt != YI_GPIO_INTERRUPT_NONE) ||
       (yi_clock_enable(cfg->clock) != 0))
    {
        return -1;
    }
    gpio.Pin = cfg->pin;
    gpio.Mode = (cfg->direction == YI_GPIO_DIRECTION_OUTPUT)
        ? ((cfg->drive == YI_GPIO_DRIVE_OPEN_DRAIN)
            ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT_PP)
        : GPIO_MODE_INPUT;
    gpio.Pull = yi_gpio_hal_pull(cfg->pull);
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init((GPIO_TypeDef *)cfg->port, &gpio);
    return 0;
}

/** @brief Drive one output GPIO to the requested logical value. */
int yi_gpio_set(yi_device_t *dev, yi_gpio_value_t value)
{
    const yi_gpio_config_t *cfg = dev->config;
    HAL_GPIO_WritePin(
        (GPIO_TypeDef *)cfg->port,
        cfg->pin,
        (value != YI_GPIO_LOW) ? GPIO_PIN_SET : GPIO_PIN_RESET
    );
    return 0;
}

/** @brief Read one GPIO input or output latch state. */
int yi_gpio_get(yi_device_t *dev)
{
    const yi_gpio_config_t *cfg = dev->config;
    return HAL_GPIO_ReadPin((GPIO_TypeDef *)cfg->port, cfg->pin);
}

/** @brief Toggle one output GPIO. */
int yi_gpio_toggle(yi_device_t *dev)
{
    const yi_gpio_config_t *cfg = dev->config;
    HAL_GPIO_TogglePin((GPIO_TypeDef *)cfg->port, cfg->pin);
    return 0;
}

/** @brief Initialize a callback object; interrupt GPIO is not enabled yet. */
void yi_gpio_callback_init(
    yi_gpio_callback_t *callback,
    yi_gpio_callback_handler_t handler,
    uint16_t pin_mask)
{
    if(callback != NULL)
    {
        callback->next = NULL;
        callback->handler = handler;
        callback->pin_mask = pin_mask;
    }
}

/** @brief Reject callback registration until the EXTI backend is added. */
int yi_gpio_add_callback(yi_device_t *dev, yi_gpio_callback_t *callback)
{
    (void)dev;
    (void)callback;
    return -1;
}

/** @brief Reject callback removal until the EXTI backend is added. */
int yi_gpio_remove_callback(yi_device_t *dev, yi_gpio_callback_t *callback)
{
    (void)dev;
    (void)callback;
    return -1;
}

/** @brief Ignore GPIO IRQ dispatch until the EXTI backend is added. */
void yi_gpio_irq_handler(uint16_t pins)
{
    (void)pins;
}
