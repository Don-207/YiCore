/** @file yi_dac.h @brief YiCore DAC interface. */
#ifndef YI_DAC_H
#define YI_DAC_H
#include "yi_device.h"
typedef struct {
    int (*write)(yi_device_t *dev, uint16_t value); /**< Write a raw DAC code. */
} yi_dac_api_t;
/** @brief Write a raw DAC code. @param dev Device instance. @param value Raw code. @return Zero on success. */
int yi_dac_write(yi_device_t *dev, uint16_t value);
#endif
