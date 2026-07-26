#include "yi_adc.h"

int yi_adc_read(yi_device_t *dev, uint16_t *value, uint32_t timeout_ms)
{
    const yi_adc_api_t *api;

    if(!yi_device_is_ready(dev) || (value == NULL) || (timeout_ms == 0U) ||
       (dev->api == NULL))
    {
        return -1;
    }
    api = (const yi_adc_api_t *)dev->api;
    return (api->read != NULL) ? api->read(dev, value, timeout_ms) : -1;
}

int yi_adc_channel_read(yi_device_t *dev, uint8_t channel,
                        uint16_t *value, uint32_t timeout_ms)
{
    const yi_adc_api_t *api;

    if(!yi_device_is_ready(dev) || (value == NULL) || (timeout_ms == 0U) ||
       (dev->api == NULL))
    {
        return -1;
    }
    api = (const yi_adc_api_t *)dev->api;
    return (api->read_channel != NULL)
           ? api->read_channel(dev, channel, value, timeout_ms) : -1;
}
