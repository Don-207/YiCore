#include "yi_gpio.h"
#include "yi_clock.h"
#include "stm32f1xx_hal.h"

static yi_device_t *yi_gpio_exti_devices[16];

static uint32_t yi_gpio_hal_pull(yi_gpio_pull_t pull)
{
    switch(pull)
    {
    case YI_GPIO_PULL_UP: return GPIO_PULLUP;
    case YI_GPIO_PULL_DOWN: return GPIO_PULLDOWN;
    case YI_GPIO_PULL_NONE:
    default: return GPIO_NOPULL;
    }
}

static int yi_gpio_pin_index(uint16_t pin)
{
    for(int index = 0; index < 16; index++)
    {
        if(pin == (uint16_t)(1U << index))
        {
            return index;
        }
    }
    return -1;
}

static IRQn_Type yi_gpio_exti_irqn(int index)
{
    if(index <= 4)
    {
        return (IRQn_Type)(EXTI0_IRQn + index);
    }
    return (index <= 9) ? EXTI9_5_IRQn : EXTI15_10_IRQn;
}

int yi_gpio_init(const void *config)
{
    const yi_gpio_config_t *cfg = config;

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       (yi_clock_enable(cfg->clock) != 0))
    {
        return -1;
    }

    GPIO_InitTypeDef gpio =
    {
        0
    };

    gpio.Pin = cfg->pin;
    gpio.Mode = (cfg->direction == YI_GPIO_DIRECTION_OUTPUT)
                ? ((cfg->drive == YI_GPIO_DRIVE_OPEN_DRAIN)
                   ? GPIO_MODE_OUTPUT_OD : GPIO_MODE_OUTPUT_PP)
                : GPIO_MODE_INPUT;
    if(cfg->interrupt != YI_GPIO_INTERRUPT_NONE)
    {
        int index = yi_gpio_pin_index(cfg->pin);
        if((cfg->direction != YI_GPIO_DIRECTION_INPUT) || (index < 0) ||
           (cfg->irq_priority > 15U) ||
           ((yi_gpio_exti_devices[index] != NULL) &&
            (yi_gpio_exti_devices[index] != cfg->self)))
        {
            return -1;
        }
        gpio.Mode = (cfg->interrupt == YI_GPIO_INTERRUPT_RISING)
                    ? GPIO_MODE_IT_RISING
                    : ((cfg->interrupt == YI_GPIO_INTERRUPT_FALLING)
                       ? GPIO_MODE_IT_FALLING : GPIO_MODE_IT_RISING_FALLING);
        yi_gpio_exti_devices[index] = cfg->self;
    }
    gpio.Pull = yi_gpio_hal_pull(cfg->pull);
    gpio.Speed = GPIO_SPEED_FREQ_LOW;

    if((cfg->direction == YI_GPIO_DIRECTION_OUTPUT) &&
       (cfg->drive == YI_GPIO_DRIVE_OPEN_DRAIN))
    {
        HAL_GPIO_WritePin((GPIO_TypeDef *)cfg->port, cfg->pin, GPIO_PIN_SET);
    }

    HAL_GPIO_Init((GPIO_TypeDef *)cfg->port, &gpio);

    if(cfg->interrupt != YI_GPIO_INTERRUPT_NONE)
    {
        int index = yi_gpio_pin_index(cfg->pin);
        IRQn_Type irqn = yi_gpio_exti_irqn(index);
        __HAL_GPIO_EXTI_CLEAR_IT(cfg->pin);
        HAL_NVIC_SetPriority(irqn, cfg->irq_priority, 0U);
        HAL_NVIC_EnableIRQ(irqn);
    }

    return 0;
}

int yi_gpio_set(yi_device_t *dev, yi_gpio_value_t value)
{
    const yi_gpio_config_t *cfg = dev->config;

    HAL_GPIO_WritePin(cfg->port, cfg->pin, value ? GPIO_PIN_SET : GPIO_PIN_RESET);

    return 0;
}

int yi_gpio_get(yi_device_t *dev)
{
    const yi_gpio_config_t *cfg = dev->config;

    return HAL_GPIO_ReadPin(cfg->port, cfg->pin);
}

int yi_gpio_toggle(yi_device_t *dev)
{
    const yi_gpio_config_t *cfg = dev->config;

    HAL_GPIO_TogglePin(cfg->port, cfg->pin);

    return 0;
}

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

int yi_gpio_add_callback(yi_device_t *dev, yi_gpio_callback_t *callback)
{
    yi_gpio_data_t *data;
    uint32_t primask;

    if((dev == NULL) || (dev->data == NULL) || (callback == NULL) ||
       (callback->handler == NULL))
    {
        return -1;
    }
    data = (yi_gpio_data_t *)dev->data;
    primask = __get_PRIMASK();
    __disable_irq();
    for(yi_gpio_callback_t *item = data->callbacks; item != NULL; item = item->next)
    {
        if(item == callback)
        {
            if(primask == 0U) { __enable_irq(); }
            return -1;
        }
    }
    callback->next = data->callbacks;
    data->callbacks = callback;
    if(primask == 0U) { __enable_irq(); }
    return 0;
}

int yi_gpio_remove_callback(yi_device_t *dev, yi_gpio_callback_t *callback)
{
    yi_gpio_data_t *data;
    yi_gpio_callback_t **link;
    uint32_t primask;

    if((dev == NULL) || (dev->data == NULL) || (callback == NULL))
    {
        return -1;
    }
    data = (yi_gpio_data_t *)dev->data;
    primask = __get_PRIMASK();
    __disable_irq();
    for(link = &data->callbacks; *link != NULL; link = &(*link)->next)
    {
        if(*link == callback)
        {
            *link = callback->next;
            callback->next = NULL;
            if(primask == 0U) { __enable_irq(); }
            return 0;
        }
    }
    if(primask == 0U) { __enable_irq(); }
    return -1;
}

void yi_gpio_irq_handler(uint16_t pins)
{
    uint16_t pending = (uint16_t)(EXTI->PR & EXTI->IMR & pins);

    EXTI->PR = pending;
    for(int index = 0; index < 16; index++)
    {
        uint16_t pin = (uint16_t)(1U << index);
        yi_device_t *dev;
        yi_gpio_data_t *data;
        if((pending & pin) == 0U) { continue; }
        dev = yi_gpio_exti_devices[index];
        if((dev == NULL) || (dev->data == NULL)) { continue; }
        data = (yi_gpio_data_t *)dev->data;
        for(yi_gpio_callback_t *cb = data->callbacks; cb != NULL; cb = cb->next)
        {
            if(((cb->pin_mask & pin) != 0U) && (cb->handler != NULL))
            {
                cb->handler(dev, cb, pin);
            }
        }
    }
}

void EXTI0_IRQHandler(void) { yi_gpio_irq_handler(GPIO_PIN_0); }
void EXTI1_IRQHandler(void) { yi_gpio_irq_handler(GPIO_PIN_1); }
void EXTI2_IRQHandler(void) { yi_gpio_irq_handler(GPIO_PIN_2); }
void EXTI3_IRQHandler(void) { yi_gpio_irq_handler(GPIO_PIN_3); }
void EXTI4_IRQHandler(void) { yi_gpio_irq_handler(GPIO_PIN_4); }
void EXTI9_5_IRQHandler(void) { yi_gpio_irq_handler(0x03E0U); }
void EXTI15_10_IRQHandler(void) { yi_gpio_irq_handler(0xFC00U); }
