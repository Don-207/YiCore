#include <stddef.h>
#include "yi_soft_spi.h"
#include "yi_gpio.h"
#include "yi_system.h"

static void yi_soft_spi_delay(uint32_t half_period_us)
{
    yi_system_delay_us(half_period_us);
}

static void yi_soft_spi_clock(const yi_soft_spi_config_t *cfg,
                              yi_gpio_value_t value)
{
    (void)yi_gpio_set(cfg->sck_gpio, value);
}

static void yi_soft_spi_mosi(const yi_soft_spi_config_t *cfg, uint8_t value)
{
    (void)yi_gpio_set(cfg->mosi_gpio,
                      value != 0U ? YI_GPIO_HIGH : YI_GPIO_LOW);
}

static uint8_t yi_soft_spi_miso(const yi_soft_spi_config_t *cfg)
{
    return yi_gpio_get(cfg->miso_gpio) == YI_GPIO_HIGH ? 1U : 0U;
}

int yi_soft_spi_init(const void *config)
{
    const yi_soft_spi_config_t *cfg = config;
    if((cfg == NULL) || !yi_device_is_ready(cfg->sck_gpio) ||
       !yi_device_is_ready(cfg->miso_gpio) ||
       !yi_device_is_ready(cfg->mosi_gpio) ||
       (cfg->max_frequency == 0U) || (cfg->max_frequency > 500000U))
    {
        return -1;
    }
    yi_soft_spi_clock(cfg, YI_GPIO_LOW);
    yi_soft_spi_mosi(cfg, 0U);
    return 0;
}

static uint8_t yi_soft_spi_byte(const yi_soft_spi_config_t *cfg,
                                uint8_t mode,
                                uint32_t half_period_us,
                                uint8_t output)
{
    uint8_t input = 0U;
    yi_gpio_value_t idle =
        (mode & 2U) != 0U ? YI_GPIO_HIGH : YI_GPIO_LOW;
    yi_gpio_value_t active = idle == YI_GPIO_HIGH ? YI_GPIO_LOW : YI_GPIO_HIGH;
    int second_edge = (mode & 1U) != 0U;

    for(uint8_t bit = 0U; bit < 8U; bit++)
    {
        if(!second_edge) { yi_soft_spi_mosi(cfg, output & 0x80U); }
        yi_soft_spi_delay(half_period_us);
        yi_soft_spi_clock(cfg, active);
        if(second_edge) { yi_soft_spi_mosi(cfg, output & 0x80U); }
        else { input = (uint8_t)((input << 1) | yi_soft_spi_miso(cfg)); }
        yi_soft_spi_delay(half_period_us);
        yi_soft_spi_clock(cfg, idle);
        if(second_edge)
        {
            input = (uint8_t)((input << 1) | yi_soft_spi_miso(cfg));
        }
        output <<= 1;
    }
    return input;
}

static int yi_soft_spi_transceive(yi_device_t *dev,
                                  const yi_spi_transfer_config_t *config,
                                  const uint8_t *tx,
                                  uint8_t *rx, uint16_t length,
                                  uint32_t timeout_ms)
{
    const yi_soft_spi_config_t *cfg = dev->config;
    yi_soft_spi_data_t *data = dev->data;
    uint32_t started = yi_system_uptime_ms();
    uint32_t half_period_us;
    int result = 0;

    if((cfg == NULL) || (data == NULL) || (config == NULL) ||
       (config->frequency == 0U) ||
       (config->frequency > cfg->max_frequency) ||
       (config->mode > 3U))
    {
        return -1;
    }
    half_period_us = (500000U + config->frequency - 1U) /
                     config->frequency;
    yi_soft_spi_clock(cfg, (config->mode & 2U) != 0U
                           ? YI_GPIO_HIGH : YI_GPIO_LOW);
    for(uint16_t index = 0U; index < length; index++)
    {
        uint8_t value = yi_soft_spi_byte(
            cfg, config->mode, half_period_us,
            tx != NULL ? tx[index] : 0xFFU);
        if(rx != NULL) { rx[index] = value; }
        if((uint32_t)(yi_system_uptime_ms() - started) >= timeout_ms)
        {
            result = -1;
            break;
        }
    }
    data->transfer_count++;
    if(result != 0) { data->error_count++; }
    return result;
}

const yi_spi_api_t yi_soft_spi_api = { .transceive = yi_soft_spi_transceive };
