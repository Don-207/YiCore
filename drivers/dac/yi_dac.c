/** @file yi_dac.c @brief YiCore DAC implementation. */
#include "yi_dac.h"
/** @brief Write a raw code through a DAC device API. */
int yi_dac_write(yi_device_t *dev, uint16_t value)
{
    const yi_dac_api_t *api;
    if(!yi_device_is_ready(dev) || dev->api == NULL) return -1;
    api = (const yi_dac_api_t *)dev->api;
    return api->write != NULL ? api->write(dev, value) : -1;
}
