#ifndef YI_SPI_H
#define YI_SPI_H

#include "yi_device.h"

typedef struct
{
    uint32_t frequency;
    yi_device_t *cs_gpio;
    uint8_t mode;
    bool cs_active_high;
} yi_spi_transfer_config_t;

typedef struct
{
    int (*transceive)(yi_device_t *dev,
                      const yi_spi_transfer_config_t *config,
                      const uint8_t *tx, uint8_t *rx,
                      uint16_t length, uint32_t timeout_ms);
} yi_spi_api_t;

int yi_spi_transceive(yi_device_t *dev,
                      const yi_spi_transfer_config_t *config,
                      const uint8_t *tx, uint8_t *rx,
                      uint16_t length, uint32_t timeout_ms);

#endif
