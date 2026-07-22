#include <stddef.h>
#include "yi_soft_i2c.h"
#include "yi_gpio.h"
#include "yi_system.h"

static void yi_soft_i2c_delay(const yi_soft_i2c_config_t *cfg)
{
    yi_system_delay_us(cfg->half_period_us);
}

static void yi_soft_i2c_scl(const yi_soft_i2c_config_t *cfg, yi_gpio_value_t value)
{
    (void)yi_gpio_set(cfg->scl_gpio, value);
}

static void yi_soft_i2c_sda(const yi_soft_i2c_config_t *cfg, yi_gpio_value_t value)
{
    (void)yi_gpio_set(cfg->sda_gpio, value);
}

static int yi_soft_i2c_wait_scl_high(const yi_soft_i2c_config_t *cfg)
{
    uint32_t start = yi_system_uptime_us();
    yi_soft_i2c_scl(cfg, YI_GPIO_HIGH);
    while(yi_gpio_get(cfg->scl_gpio) != YI_GPIO_HIGH)
    {
        if((uint32_t)(yi_system_uptime_us() - start) >= cfg->stretch_timeout_us)
        {
            return -1;
        }
    }
    return 0;
}

static int yi_soft_i2c_start(const yi_soft_i2c_config_t *cfg)
{
    yi_soft_i2c_sda(cfg, YI_GPIO_HIGH);
    if(yi_soft_i2c_wait_scl_high(cfg) != 0) { return -1; }
    yi_soft_i2c_delay(cfg);
    yi_soft_i2c_sda(cfg, YI_GPIO_LOW);
    yi_soft_i2c_delay(cfg);
    yi_soft_i2c_scl(cfg, YI_GPIO_LOW);
    yi_soft_i2c_delay(cfg);
    return 0;
}

static int yi_soft_i2c_stop(const yi_soft_i2c_config_t *cfg)
{
    yi_soft_i2c_sda(cfg, YI_GPIO_LOW);
    yi_soft_i2c_delay(cfg);
    if(yi_soft_i2c_wait_scl_high(cfg) != 0) { return -1; }
    yi_soft_i2c_delay(cfg);
    yi_soft_i2c_sda(cfg, YI_GPIO_HIGH);
    yi_soft_i2c_delay(cfg);
    return 0;
}

static int yi_soft_i2c_write_byte(const yi_soft_i2c_config_t *cfg, uint8_t value)
{
    for(uint8_t bit = 0U; bit < 8U; bit++)
    {
        yi_soft_i2c_sda(cfg, ((value & 0x80U) != 0U) ? YI_GPIO_HIGH : YI_GPIO_LOW);
        yi_soft_i2c_delay(cfg);
        if(yi_soft_i2c_wait_scl_high(cfg) != 0) { return -1; }
        yi_soft_i2c_delay(cfg);
        yi_soft_i2c_scl(cfg, YI_GPIO_LOW);
        yi_soft_i2c_delay(cfg);
        value <<= 1;
    }

    yi_soft_i2c_sda(cfg, YI_GPIO_HIGH);
    yi_soft_i2c_delay(cfg);
    if(yi_soft_i2c_wait_scl_high(cfg) != 0) { return -1; }
    yi_soft_i2c_delay(cfg);
    int ack = yi_gpio_get(cfg->sda_gpio);
    yi_soft_i2c_scl(cfg, YI_GPIO_LOW);
    yi_soft_i2c_delay(cfg);
    return (ack == YI_GPIO_LOW) ? 0 : -1;
}

static int yi_soft_i2c_read_byte(const yi_soft_i2c_config_t *cfg,
                                 uint8_t *value, int send_ack)
{
    uint8_t result = 0U;
    yi_soft_i2c_sda(cfg, YI_GPIO_HIGH);
    for(uint8_t bit = 0U; bit < 8U; bit++)
    {
        result <<= 1;
        yi_soft_i2c_delay(cfg);
        if(yi_soft_i2c_wait_scl_high(cfg) != 0) { return -1; }
        yi_soft_i2c_delay(cfg);
        if(yi_gpio_get(cfg->sda_gpio) == YI_GPIO_HIGH) { result |= 1U; }
        yi_soft_i2c_scl(cfg, YI_GPIO_LOW);
        yi_soft_i2c_delay(cfg);
    }
    yi_soft_i2c_sda(cfg, send_ack ? YI_GPIO_LOW : YI_GPIO_HIGH);
    yi_soft_i2c_delay(cfg);
    if(yi_soft_i2c_wait_scl_high(cfg) != 0) { return -1; }
    yi_soft_i2c_delay(cfg);
    yi_soft_i2c_scl(cfg, YI_GPIO_LOW);
    yi_soft_i2c_sda(cfg, YI_GPIO_HIGH);
    yi_soft_i2c_delay(cfg);
    *value = result;
    return 0;
}

static int yi_soft_i2c_recover(const yi_soft_i2c_config_t *cfg)
{
    yi_soft_i2c_sda(cfg, YI_GPIO_HIGH);
    yi_soft_i2c_scl(cfg, YI_GPIO_HIGH);
    yi_soft_i2c_delay(cfg);
    if(yi_gpio_get(cfg->sda_gpio) == YI_GPIO_HIGH) { return 0; }

    for(uint8_t pulse = 0U; pulse < cfg->recovery_clocks; pulse++)
    {
        yi_soft_i2c_scl(cfg, YI_GPIO_LOW);
        yi_soft_i2c_delay(cfg);
        if(yi_soft_i2c_wait_scl_high(cfg) != 0) { return -1; }
        yi_soft_i2c_delay(cfg);
        if(yi_gpio_get(cfg->sda_gpio) == YI_GPIO_HIGH) { break; }
    }
    return yi_soft_i2c_stop(cfg);
}

int yi_soft_i2c_init(const void *config)
{
    const yi_soft_i2c_config_t *cfg = config;
    if((cfg == NULL) || !yi_device_is_ready(cfg->scl_gpio) ||
       !yi_device_is_ready(cfg->sda_gpio) ||
       (cfg->clock_frequency == 0U) || (cfg->clock_frequency > 100000U) ||
       (cfg->half_period_us == 0U) || (cfg->stretch_timeout_us == 0U) ||
       (cfg->recovery_clocks == 0U))
    {
        return -1;
    }
    yi_soft_i2c_scl(cfg, YI_GPIO_HIGH);
    yi_soft_i2c_sda(cfg, YI_GPIO_HIGH);
    return yi_soft_i2c_recover(cfg);
}

static int yi_soft_i2c_transfer(yi_device_t *dev, uint8_t address,
                                yi_i2c_msg_t *messages, uint8_t count,
                                uint32_t timeout_ms)
{
    const yi_soft_i2c_config_t *cfg = dev->config;
    yi_soft_i2c_data_t *data = dev->data;
    uint32_t started_ms = yi_system_uptime_ms();
    int result = -1;

    if((cfg == NULL) || (data == NULL) || (timeout_ms == 0U)) { return -1; }
    for(uint8_t message_index = 0U; message_index < count; message_index++)
    {
        yi_i2c_msg_t *message = &messages[message_index];
        int read = ((message->flags & YI_I2C_MSG_READ) != 0U);
        if(yi_soft_i2c_start(cfg) != 0) { goto finish; }
        if(yi_soft_i2c_write_byte(cfg, (uint8_t)((address << 1) | (read ? 1U : 0U))) != 0)
        {
            goto finish;
        }
        for(uint16_t index = 0U; index < message->length; index++)
        {
            if((uint32_t)(yi_system_uptime_ms() - started_ms) >= timeout_ms)
            {
                goto finish;
            }
            if(read)
            {
                if(yi_soft_i2c_read_byte(cfg, &message->buffer[index],
                                         index + 1U < message->length) != 0)
                {
                    goto finish;
                }
            }
            else if(yi_soft_i2c_write_byte(cfg, message->buffer[index]) != 0)
            {
                goto finish;
            }
        }
    }
    result = 0;

finish:
    if(yi_soft_i2c_stop(cfg) != 0) { result = -1; }
    data->transfer_count++;
    if(result != 0) { data->error_count++; }
    return result;
}

const yi_i2c_api_t yi_soft_i2c_api = { .transfer = yi_soft_i2c_transfer };
