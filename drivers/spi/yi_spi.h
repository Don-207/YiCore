/**
 * @file yi_spi.h
 * @brief YiCore spi interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_SPI_H
#define YI_SPI_H

#include "yi_device.h"

typedef struct
{
    uint32_t frequency; /**< Frequency value. */
    yi_device_t *cs_gpio; /**< Cs gpio value. */
    uint8_t mode; /**< Mode value. */
    bool lsb_first; /**< True to shift the least-significant bit first. */
    bool cs_active_high; /**< Cs active high value. */} yi_spi_transfer_config_t;

typedef struct
{
    int (*configure)(yi_device_t *dev,
                     const yi_spi_transfer_config_t *config);
    int (*transceive)(yi_device_t *dev,
                      const yi_spi_transfer_config_t *config,
                      const uint8_t *tx, uint8_t *rx,
                      uint16_t length, uint32_t timeout_ms);
    uint32_t (*get_frequency)(yi_device_t *dev);
} yi_spi_api_t;

/**
 * @brief Configure a hardware SPI controller without starting a transfer.
 * @param dev Ready YiCore SPI device.
 * @param config Requested bus frequency, mode, and bit order.
 * @return Zero on success or a negative backend error.
 */
int yi_spi_configure(yi_device_t *dev,
                     const yi_spi_transfer_config_t *config);

/**
 * @brief Read the actual SPI clock selected by the hardware backend.
 * @param dev Ready YiCore SPI device.
 * @return Active clock in hertz, or zero when unavailable.
 */
uint32_t yi_spi_get_frequency(yi_device_t *dev);

/**
 * @brief Perform the yi spi transceive operation.
 * @param dev Device instance.
 * @param config Device configuration.
 * @param tx Tx value.
 * @param rx Rx value.
 * @param length Number of bytes to process.
 * @param timeout_ms Operation timeout in milliseconds.
 */
int yi_spi_transceive(yi_device_t *dev,
                      const yi_spi_transfer_config_t *config,
                      const uint8_t *tx, uint8_t *rx,
                      uint16_t length, uint32_t timeout_ms);

#endif
