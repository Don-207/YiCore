#ifndef YI_SOFT_SPI_H
#define YI_SOFT_SPI_H

#include "yi_spi.h"

typedef struct
{
    yi_device_t *sck_gpio;
    yi_device_t *miso_gpio;
    yi_device_t *mosi_gpio;
    uint32_t max_frequency;
    uint32_t half_period_us;
    uint8_t mode;
} yi_soft_spi_config_t;

typedef struct
{
    uint32_t transfer_count;
    uint32_t error_count;
} yi_soft_spi_data_t;

int yi_soft_spi_init(const void *config);
extern const yi_spi_api_t yi_soft_spi_api;

#define YI_SOFT_SPI_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority,                   \
                              yi_soft_spi_init, &_config, &_data,         \
                              (const yi_device_api_t *)&yi_soft_spi_api)

#endif
