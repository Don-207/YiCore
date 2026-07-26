/**
 * @file yi_i2c.h
 * @brief YiCore i2c interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_I2C_H
#define YI_I2C_H

#include <stdint.h>
#include "yi_device.h"

#define YI_I2C_MSG_READ    (1U << 0)
#define YI_I2C_MSG_RESTART (1U << 1)
#define YI_I2C_MSG_STOP    (1U << 2)

typedef struct
{
    uint8_t *buffer; /**< Buffer value. */
    uint16_t length; /**< Length value. */
    uint8_t flags; /**< Flags value. */} yi_i2c_msg_t;

typedef struct
{
    int (*transfer)(yi_device_t *dev, uint8_t address,
                    yi_i2c_msg_t *messages, uint8_t message_count,
                    uint32_t timeout_ms);
} yi_i2c_api_t;

/**
 * @brief Transfer the module.
 * @param dev Device instance.
 * @param address Address value.
 * @param messages Messages value.
 * @param message_count Message count value.
 * @param timeout_ms Operation timeout in milliseconds.
 */
int yi_i2c_transfer(yi_device_t *dev, uint8_t address,
                    yi_i2c_msg_t *messages, uint8_t message_count,
                    uint32_t timeout_ms);
/**
 * @brief Write the module.
 * @param dev Device instance.
 * @param address Address value.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 * @param timeout_ms Operation timeout in milliseconds.
 */
int yi_i2c_master_write(yi_device_t *dev, uint8_t address,
                        const uint8_t *data, uint16_t length,
                        uint32_t timeout_ms);
/**
 * @brief Read the module.
 * @param dev Device instance.
 * @param address Address value.
 * @param data Driver runtime data.
 * @param length Number of bytes to process.
 * @param timeout_ms Operation timeout in milliseconds.
 */
int yi_i2c_master_read(yi_device_t *dev, uint8_t address, uint8_t *data,
                       uint16_t length, uint32_t timeout_ms);
/**
 * @brief Write read.
 * @param dev Device instance.
 * @param address Address value.
 * @param tx_data Tx data value.
 * @param tx_length Tx length value.
 * @param rx_data Rx data value.
 * @param rx_length Rx length value.
 * @param timeout_ms Operation timeout in milliseconds.
 */
int yi_i2c_master_write_read(yi_device_t *dev, uint8_t address,
                             const uint8_t *tx_data, uint16_t tx_length,
                             uint8_t *rx_data, uint16_t rx_length,
                             uint32_t timeout_ms);

#endif
