/**
 * @file yidap_ch32h417_peripherals.h
 * @brief Define CH32H417 I2C2 and SPI3 peripheral access for YiDAP.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */
#ifndef YIDAP_CH32H417_PERIPHERALS_H
#define YIDAP_CH32H417_PERIPHERALS_H
#include <stddef.h>
#include <stdint.h>

/** Peripheral operation result exposed through vendor commands. */
typedef enum yidap_bus_status {
    YIDAP_BUS_OK = 0, YIDAP_BUS_INVALID = 1, YIDAP_BUS_NOT_READY = 2,
    YIDAP_BUS_NACK = 3, YIDAP_BUS_TIMEOUT = 4, YIDAP_BUS_ERROR = 5
} yidap_bus_status_t;

/** Initialize I2C2 on PC0/PC1 and SPI3 on PA14/PA15/PA13 with CS on PB12. */
void yidap_peripherals_init(void);
/** Configure I2C2 bus frequency. */
yidap_bus_status_t yidap_i2c_configure(uint32_t speed_hz);
/** Return active I2C2 bus frequency. */
uint32_t yidap_i2c_speed(void);
/** Execute a bounded I2C write/read transaction with repeated START. */
yidap_bus_status_t yidap_i2c_transfer(uint8_t address, const uint8_t *tx,
                                     size_t tx_size, uint8_t *rx, size_t rx_size);
/** Configure SPI3 clock mode and bit order. */
yidap_bus_status_t yidap_spi_configure(uint32_t speed_hz, uint8_t mode,
                                      uint8_t lsb_first);
/** Return requested SPI3 frequency. */
uint32_t yidap_spi_speed(void);
/** Return active SPI mode. */
uint8_t yidap_spi_mode(void);
/** Return nonzero for LSB-first. */
uint8_t yidap_spi_lsb_first(void);
/** Transfer bytes using SPI3 while PB12 CS remains asserted. */
yidap_bus_status_t yidap_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t size);
#endif
