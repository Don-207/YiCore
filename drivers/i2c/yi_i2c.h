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

/** Stable YiCore I2C driver result values. */
typedef enum yi_i2c_result {
    YI_I2C_RESULT_OK = 0,
    YI_I2C_RESULT_INVALID = -1,
    YI_I2C_RESULT_NACK = -2,
    YI_I2C_RESULT_TIMEOUT = -3,
    YI_I2C_RESULT_BUS_ERROR = -4
} yi_i2c_result_t;

typedef struct
{
    uint8_t *buffer; /**< Buffer value. */
    uint16_t length; /**< Length value. */
    uint8_t flags; /**< Flags value. */} yi_i2c_msg_t;

typedef struct
{
    int (*configure)(yi_device_t *dev, uint32_t frequency);
    int (*transfer)(yi_device_t *dev, uint8_t address,
                    yi_i2c_msg_t *messages, uint8_t message_count,
                    uint32_t timeout_ms);
    uint32_t (*get_frequency)(yi_device_t *dev);
} yi_i2c_api_t;

/** Return the active hardware I2C clock in hertz, or zero when unavailable. */
uint32_t yi_i2c_get_frequency(yi_device_t *dev);

/**
 * @brief Configure the I2C bus clock frequency.
 * @param dev Device instance.
 * @param frequency Requested bus frequency in hertz.
 * @return Zero on success or a negative driver error.
 */
int yi_i2c_configure(yi_device_t *dev, uint32_t frequency);

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
