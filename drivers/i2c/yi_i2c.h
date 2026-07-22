#ifndef YI_I2C_H
#define YI_I2C_H

#include <stdint.h>
#include "yi_device.h"

#define YI_I2C_MSG_READ    (1U << 0)
#define YI_I2C_MSG_RESTART (1U << 1)
#define YI_I2C_MSG_STOP    (1U << 2)

typedef struct
{
    uint8_t *buffer;
    uint16_t length;
    uint8_t flags;
} yi_i2c_msg_t;

typedef struct
{
    int (*transfer)(yi_device_t *dev, uint8_t address,
                    yi_i2c_msg_t *messages, uint8_t message_count,
                    uint32_t timeout_ms);
} yi_i2c_api_t;

int yi_i2c_transfer(yi_device_t *dev, uint8_t address,
                    yi_i2c_msg_t *messages, uint8_t message_count,
                    uint32_t timeout_ms);
int yi_i2c_master_write(yi_device_t *dev, uint8_t address,
                        const uint8_t *data, uint16_t length,
                        uint32_t timeout_ms);
int yi_i2c_master_read(yi_device_t *dev, uint8_t address, uint8_t *data,
                       uint16_t length, uint32_t timeout_ms);
int yi_i2c_master_write_read(yi_device_t *dev, uint8_t address,
                             const uint8_t *tx_data, uint16_t tx_length,
                             uint8_t *rx_data, uint16_t rx_length,
                             uint32_t timeout_ms);

#endif
