/**
 * @file yi_adc.h
 * @brief YiCore adc interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_ADC_H
#define YI_ADC_H

#include "yi_device.h"

typedef struct
{
    int (*read)(yi_device_t *dev, uint16_t *value, uint32_t timeout_ms);
    int (*read_channel)(yi_device_t *dev, uint8_t channel,
                        uint16_t *value, uint32_t timeout_ms);
} yi_adc_api_t;

/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param value Value to process.
 * @param timeout_ms Operation timeout in milliseconds.
 */
int yi_adc_read(yi_device_t *dev, uint16_t *value, uint32_t timeout_ms);
/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param channel Channel value.
 * @param value Value to process.
 * @param timeout_ms Operation timeout in milliseconds.
 */
int yi_adc_channel_read(yi_device_t *dev, uint8_t channel,
                        uint16_t *value, uint32_t timeout_ms);

#endif
