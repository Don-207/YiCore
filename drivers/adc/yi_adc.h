#ifndef YI_ADC_H
#define YI_ADC_H

#include "yi_device.h"

typedef struct
{
    int (*read)(yi_device_t *dev, uint16_t *value, uint32_t timeout_ms);
    int (*read_channel)(yi_device_t *dev, uint8_t channel,
                        uint16_t *value, uint32_t timeout_ms);
} yi_adc_api_t;

int yi_adc_read(yi_device_t *dev, uint16_t *value, uint32_t timeout_ms);
int yi_adc_channel_read(yi_device_t *dev, uint8_t channel,
                        uint16_t *value, uint32_t timeout_ms);

#endif
