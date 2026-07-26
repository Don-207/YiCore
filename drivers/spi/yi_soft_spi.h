/**
 * @file yi_soft_spi.h
 * @brief YiCore soft spi interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_SOFT_SPI_H
#define YI_SOFT_SPI_H

#include "yi_spi.h"

typedef struct
{
    yi_device_t *sck_gpio; /**< Sck gpio value. */
    yi_device_t *miso_gpio; /**< Miso gpio value. */
    yi_device_t *mosi_gpio; /**< Mosi gpio value. */
    uint32_t max_frequency; /**< Max frequency value. */} yi_soft_spi_config_t;

typedef struct
{
    uint32_t transfer_count; /**< Transfer count value. */
    uint32_t error_count; /**< Error count value. */} yi_soft_spi_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_soft_spi_init(const void *config);
extern const yi_spi_api_t yi_soft_spi_api;

#define YI_SOFT_SPI_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority,                   \
                              yi_soft_spi_init, &_config, &_data,         \
                              (const yi_device_api_t *)&yi_soft_spi_api)

#endif
