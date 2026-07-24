#include "yi_pinmux.h"
#include "yi_clock.h"
#include "stm32f1xx_hal.h"

static uint32_t yi_pinmux_hal_mode(yi_pinmux_mode_t mode)
{
    static const uint32_t values[] = {
        GPIO_MODE_INPUT, GPIO_MODE_OUTPUT_PP, GPIO_MODE_OUTPUT_OD,
        GPIO_MODE_AF_PP, GPIO_MODE_AF_OD, GPIO_MODE_ANALOG
    };
    return ((unsigned int)mode < (sizeof(values) / sizeof(values[0])))
           ? values[mode] : GPIO_MODE_INPUT;
}

static uint32_t yi_pinmux_hal_pull(yi_pinmux_pull_t pull)
{
    static const uint32_t values[] = {GPIO_NOPULL, GPIO_PULLUP, GPIO_PULLDOWN};
    return ((unsigned int)pull < (sizeof(values) / sizeof(values[0])))
           ? values[pull] : GPIO_NOPULL;
}

static uint32_t yi_pinmux_hal_speed(yi_pinmux_speed_t speed)
{
    static const uint32_t values[] = {
        GPIO_SPEED_FREQ_LOW, GPIO_SPEED_FREQ_MEDIUM, GPIO_SPEED_FREQ_HIGH
    };
    return ((unsigned int)speed < (sizeof(values) / sizeof(values[0])))
           ? values[speed] : GPIO_SPEED_FREQ_LOW;
}

static int yi_pinmux_apply_config(const yi_pinmux_config_t *cfg)
{
    GPIO_InitTypeDef gpio = {0};

    if((cfg == NULL) || (cfg->port == NULL) || (cfg->pin == 0U) ||
       ((cfg->pin & (uint16_t)(cfg->pin - 1U)) != 0U))
    {
        return -1;
    }
    gpio.Pin = cfg->pin;
    gpio.Mode = yi_pinmux_hal_mode(cfg->mode);
    gpio.Pull = yi_pinmux_hal_pull(cfg->pull);
    gpio.Speed = yi_pinmux_hal_speed(cfg->speed);
    HAL_GPIO_Init((GPIO_TypeDef *)cfg->port, &gpio);
    return 0;
}

int yi_pinmux_init(const void *config)
{
    const yi_pinmux_config_t *cfg = (const yi_pinmux_config_t *)config;

    if((cfg == NULL) || (yi_clock_enable(cfg->clock) != 0))
    {
        return -1;
    }
    if(yi_pinmux_apply_config(cfg) != 0)
    {
        (void)yi_clock_disable(cfg->clock);
        return -1;
    }
    return 0;
}

int yi_pinmux_apply(yi_device_t *dev)
{
    if(dev == NULL)
    {
        return -1;
    }
    return yi_pinmux_apply_config((const yi_pinmux_config_t *)dev->config);
}

int yi_pinmux_release(yi_device_t *dev)
{
    const yi_pinmux_config_t *cfg;

    if((dev == NULL) || (dev->config == NULL))
    {
        return -1;
    }
    cfg = (const yi_pinmux_config_t *)dev->config;
    HAL_GPIO_DeInit((GPIO_TypeDef *)cfg->port, cfg->pin);
    return yi_clock_disable(cfg->clock);
}
