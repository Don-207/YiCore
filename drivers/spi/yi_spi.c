#include "yi_spi.h"
#include "yi_gpio.h"
#include <stddef.h>

int yi_spi_transceive(yi_device_t *dev,
                      const yi_spi_transfer_config_t *config,
                      const uint8_t *tx, uint8_t *rx,
                      uint16_t length, uint32_t timeout_ms)
{
    const yi_spi_api_t *api;
    yi_gpio_value_t cs_active;
    int result;

    if(!yi_device_is_ready(dev) || (dev->api == NULL) ||
       (config == NULL) || (config->frequency == 0U) ||
       (config->mode > 3U) || (length == 0U) ||
       (timeout_ms == 0U) || ((tx == NULL) && (rx == NULL))) { return -1; }
    if((config->cs_gpio != NULL) && !yi_device_is_ready(config->cs_gpio))
    {
        return -1;
    }

    api = (const yi_spi_api_t *)dev->api;
    if(api->transceive == NULL) { return -1; }

    cs_active = config->cs_active_high ? YI_GPIO_HIGH : YI_GPIO_LOW;
    if(config->cs_gpio != NULL)
    {
        if((yi_gpio_set(config->cs_gpio,
                        cs_active == YI_GPIO_HIGH
                        ? YI_GPIO_LOW : YI_GPIO_HIGH) != 0) ||
           (yi_gpio_set(config->cs_gpio, cs_active) != 0))
        {
            return -1;
        }
    }

    result = api->transceive(dev, config, tx, rx, length, timeout_ms);
    if((config->cs_gpio != NULL) &&
       (yi_gpio_set(config->cs_gpio,
                    cs_active == YI_GPIO_HIGH
                    ? YI_GPIO_LOW : YI_GPIO_HIGH) != 0))
    {
        result = -1;
    }
    return result;
}
