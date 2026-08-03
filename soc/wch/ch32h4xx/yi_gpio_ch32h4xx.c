/**
 * @file yi_gpio_ch32h4xx.c
 * @brief Implement the YiCore GPIO contract for CH32H417 V3F.
 * @author Don
 * @date 2026-08-02
 * @version 1.0.0
 */

#include "ch32h417.h"
#include "yi_gpio.h"
#include "yi_system.h"
#include <stddef.h>

/** Return the HB2 clock mask associated with a CH32 GPIO port. */
static uint32_t yi_ch32h4xx_gpio_clock(const void *port)
{
    if(port == GPIOA) { return RCC_HB2Periph_GPIOA; }
    if(port == GPIOB) { return RCC_HB2Periph_GPIOB; }
    if(port == GPIOC) { return RCC_HB2Periph_GPIOC; }
    if(port == GPIOD) { return RCC_HB2Periph_GPIOD; }
    if(port == GPIOE) { return RCC_HB2Periph_GPIOE; }
    if(port == GPIOF) { return RCC_HB2Periph_GPIOF; }
    return 0U;
}

/** Convert YiCore electrical configuration into the WCH GPIO mode. */
static GPIOMode_TypeDef yi_ch32h4xx_gpio_mode(const yi_gpio_config_t *config)
{
    if(config->direction == YI_GPIO_DIRECTION_OUTPUT)
    {
        return (config->drive == YI_GPIO_DRIVE_OPEN_DRAIN)
            ? GPIO_Mode_Out_OD : GPIO_Mode_Out_PP;
    }
    if(config->pull == YI_GPIO_PULL_UP) { return GPIO_Mode_IPU; }
    if(config->pull == YI_GPIO_PULL_DOWN) { return GPIO_Mode_IPD; }
    return GPIO_Mode_IN_FLOATING;
}

/** Initialize one CH32 GPIO device; interrupt mode is intentionally deferred. */
int yi_gpio_init(const void *config)
{
    const yi_gpio_config_t *gpio_config = config;
    GPIO_InitTypeDef vendor_config = {0};
    uint32_t clock_mask;

    if((gpio_config == NULL) || (gpio_config->port == NULL) ||
       (gpio_config->pin == 0U) ||
       (gpio_config->interrupt != YI_GPIO_INTERRUPT_NONE))
    {
        return -1;
    }
    clock_mask = yi_ch32h4xx_gpio_clock(gpio_config->port);
    if(clock_mask == 0U)
    {
        return -1;
    }
    RCC_HB2PeriphClockCmd(clock_mask, ENABLE);
    vendor_config.GPIO_Pin = gpio_config->pin;
    vendor_config.GPIO_Mode = yi_ch32h4xx_gpio_mode(gpio_config);
    vendor_config.GPIO_Speed = GPIO_Speed_Very_High;
    GPIO_Init((GPIO_TypeDef *)gpio_config->port, &vendor_config);
    return 0;
}

/** Set a CH32 GPIO output to a logical YiCore value. */
int yi_gpio_set(yi_device_t *device, yi_gpio_value_t value)
{
    const yi_gpio_config_t *config;

    if((device == NULL) || !yi_device_is_ready(device) ||
       (device->config == NULL))
    {
        return -1;
    }
    config = device->config;
    GPIO_WriteBit((GPIO_TypeDef *)config->port, config->pin,
                  value == YI_GPIO_HIGH ? Bit_SET : Bit_RESET);
    return 0;
}

/** Read the current CH32 GPIO input state. */
int yi_gpio_get(yi_device_t *device)
{
    const yi_gpio_config_t *config;

    if((device == NULL) || !yi_device_is_ready(device) ||
       (device->config == NULL))
    {
        return -1;
    }
    config = device->config;
    return GPIO_ReadInputDataBit((GPIO_TypeDef *)config->port, config->pin)
        == Bit_SET ? 1 : 0;
}

/** Toggle a CH32 GPIO output atomically through the vendor bit operation. */
int yi_gpio_toggle(yi_device_t *device)
{
    int value = yi_gpio_get(device);

    if(value < 0)
    {
        return -1;
    }
    return yi_gpio_set(device, value == 0 ? YI_GPIO_HIGH : YI_GPIO_LOW);
}

/** Initialize callback metadata; CH32 EXTI registration is not enabled yet. */
void yi_gpio_callback_init(yi_gpio_callback_t *callback,
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

/** Reject callback registration until the CH32 EXTI backend is implemented. */
int yi_gpio_add_callback(yi_device_t *device, yi_gpio_callback_t *callback)
{
    (void)device;
    (void)callback;
    return -1;
}

/** Reject callback removal until the CH32 EXTI backend is implemented. */
int yi_gpio_remove_callback(yi_device_t *device, yi_gpio_callback_t *callback)
{
    (void)device;
    (void)callback;
    return -1;
}

/** Reserve the common IRQ dispatch symbol for the future CH32 EXTI backend. */
void yi_gpio_irq_handler(uint16_t pins)
{
    (void)pins;
}
