#include "yi_led.h"
#include "yi_gpio.h"
/*
 * LED初始化
 */
int yi_led_init(const void *config)
{
    const yi_led_config_t *cfg = config;


    /*
     * 检查依赖GPIO是否存在
     */
    if((cfg == NULL) || !yi_device_is_ready(cfg->gpio))
    {
        return -1;
    }


    /*
     * 默认关闭LED
     */
    yi_gpio_set(
    cfg->gpio,
    cfg->active_low ? YI_GPIO_HIGH : YI_GPIO_LOW
);


    return 0;
}

/*
 * LED ON
 */
static int yi_led_on_impl(yi_device_t *dev)
{
    const yi_led_config_t *cfg;
    yi_led_data_t *data;

    if(!yi_device_is_ready(dev) || (dev->config == NULL) || (dev->data == NULL))
    {
        return -1;
    }

    cfg = (const yi_led_config_t *)dev->config;
    data = (yi_led_data_t *)dev->data;
    if(yi_gpio_set(cfg->gpio, cfg->active_low ? YI_GPIO_LOW : YI_GPIO_HIGH) != 0)
    {
        return -1;
    }

    data->state = 1U;
    return 0;
}

/*
 * LED OFF
 */
static int yi_led_off_impl(yi_device_t *dev)
{
    const yi_led_config_t *cfg;
    yi_led_data_t *data;

    if(!yi_device_is_ready(dev) || (dev->config == NULL) || (dev->data == NULL))
    {
        return -1;
    }

    cfg = (const yi_led_config_t *)dev->config;
    data = (yi_led_data_t *)dev->data;
    if(yi_gpio_set(cfg->gpio, cfg->active_low ? YI_GPIO_HIGH : YI_GPIO_LOW) != 0)
    {
        return -1;
    }

    data->state = 0U;
    return 0;
}

/*
 * LED Toggle
 */
static int yi_led_toggle_impl(yi_device_t *dev)
{
    const yi_led_config_t *cfg;
    yi_led_data_t *data;

    if(!yi_device_is_ready(dev) || (dev->config == NULL) || (dev->data == NULL))
    {
        return -1;
    }

    cfg = (const yi_led_config_t *)dev->config;
    data = (yi_led_data_t *)dev->data;
    if(yi_gpio_toggle(cfg->gpio) != 0)
    {
        return -1;
    }

    data->state = (uint8_t)!data->state;
    return 0;
}

/*
 * 获取状态
 */
static int yi_led_read(yi_device_t *dev, uint8_t *buf, uint32_t len)
{
    const yi_led_config_t *cfg;
    yi_led_data_t *data;
    int pin_state;

    if(!yi_device_is_ready(dev) || (dev->config == NULL) || (dev->data == NULL) ||
       (buf == NULL) || (len < 1U))
    {
        return -1;
    }

    cfg = (const yi_led_config_t *)dev->config;
    data = (yi_led_data_t *)dev->data;
    pin_state = yi_gpio_get(cfg->gpio);
    data->state = cfg->active_low ? (pin_state == 0) : (pin_state != 0);
    buf[0] = data->state;

    return 0;
}

static int yi_led_write(yi_device_t *dev, const uint8_t *buf, uint32_t len)
{
    if((buf == NULL) || (len < 1U))
    {
        return -1;
    }

    return (buf[0] != 0U) ? yi_led_on_impl(dev) : yi_led_off_impl(dev);
}

const yi_led_api_t yi_led_driver_api =
{
    .device =
    {
        .open = NULL,
        .close = NULL,
        .write = yi_led_write,
        .read = yi_led_read
    },
    .on = yi_led_on_impl,
    .off = yi_led_off_impl,
    .toggle = yi_led_toggle_impl
};
