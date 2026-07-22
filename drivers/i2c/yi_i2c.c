#include <stddef.h>
#include "yi_i2c.h"

int yi_i2c_transfer(yi_device_t *dev, uint8_t address,
                    yi_i2c_msg_t *messages, uint8_t message_count,
                    uint32_t timeout_ms)
{
    const yi_i2c_api_t *api;

    if(!yi_device_is_ready(dev) || (dev->api == NULL) ||
       (address > 0x7FU) || (messages == NULL) ||
       (message_count == 0U))
    {
        return -1;
    }
    for(uint8_t index = 0U; index < message_count; index++)
    {
        if((messages[index].buffer == NULL) || (messages[index].length == 0U))
        {
            return -1;
        }
    }
    api = (const yi_i2c_api_t *)dev->api;
    return (api->transfer != NULL)
           ? api->transfer(dev, address, messages, message_count, timeout_ms)
           : -1;
}

int yi_i2c_master_write(yi_device_t *dev, uint8_t address,
                        const uint8_t *data, uint16_t length,
                        uint32_t timeout_ms)
{
    yi_i2c_msg_t message = {
        .buffer = (uint8_t *)data,
        .length = length,
        .flags = YI_I2C_MSG_STOP
    };
    return yi_i2c_transfer(dev, address, &message, 1U, timeout_ms);
}

int yi_i2c_master_read(yi_device_t *dev, uint8_t address, uint8_t *data,
                       uint16_t length, uint32_t timeout_ms)
{
    yi_i2c_msg_t message = {
        .buffer = data,
        .length = length,
        .flags = YI_I2C_MSG_READ | YI_I2C_MSG_STOP
    };
    return yi_i2c_transfer(dev, address, &message, 1U, timeout_ms);
}

int yi_i2c_master_write_read(yi_device_t *dev, uint8_t address,
                             const uint8_t *tx_data, uint16_t tx_length,
                             uint8_t *rx_data, uint16_t rx_length,
                             uint32_t timeout_ms)
{
    yi_i2c_msg_t messages[2] = {
        { .buffer = (uint8_t *)tx_data, .length = tx_length, .flags = 0U },
        { .buffer = rx_data, .length = rx_length,
          .flags = YI_I2C_MSG_READ | YI_I2C_MSG_RESTART | YI_I2C_MSG_STOP }
    };
    return yi_i2c_transfer(dev, address, messages, 2U, timeout_ms);
}
