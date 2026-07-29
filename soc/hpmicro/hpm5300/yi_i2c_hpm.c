/**
 * @file yi_i2c_hpm.c
 * @brief Implement the YiCore I2C API with HPMicro polling transfers.
 * @author Don
 * @date 2026-07-29
 * @version 1.0.0
 */

#include "yi_i2c_hpm.h"

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Convert a supported bus frequency to an HPM I2C operating mode.
 * @param frequency Bus frequency in hertz.
 * @param mode Destination for the corresponding HPM operating mode.
 * @return Zero when supported or negative one otherwise.
 */
static int yi_i2c_hpm_mode(uint32_t frequency, uint8_t *mode)
{
    if((mode == NULL) ||
       ((frequency != 100000U) && (frequency != 400000U) &&
        (frequency != 1000000U))) {
        return -1;
    }
    *mode = (frequency == 100000U) ? i2c_mode_normal
          : (frequency == 400000U) ? i2c_mode_fast
                                   : i2c_mode_fast_plus;
    return 0;
}

/**
 * @brief Convert an HPM I2C status to a stable YiCore result.
 * @param status HPM driver status value.
 * @return Corresponding yi_i2c_result_t value.
 */
static int yi_i2c_hpm_result(hpm_stat_t status)
{
    if(status == status_success) {
        return YI_I2C_RESULT_OK;
    }
    if(status == status_invalid_argument) {
        return YI_I2C_RESULT_INVALID;
    }
    if(status == status_i2c_no_addr_hit) {
        return YI_I2C_RESULT_NACK;
    }
    if(status == status_timeout) {
        return YI_I2C_RESULT_TIMEOUT;
    }
    return YI_I2C_RESULT_BUS_ERROR;
}

/**
 * @brief Apply a new bus clock to an initialized HPM I2C device.
 * @param dev YiCore I2C device.
 * @param frequency Requested bus frequency in hertz.
 * @return Zero on success or negative one on failure.
 */
static int yi_i2c_hpm_configure(yi_device_t *dev, uint32_t frequency)
{
    const yi_i2c_hpm_config_t *cfg;
    yi_i2c_hpm_data_t *data;
    i2c_config_t controller_config;
    hpm_stat_t status;

    if((dev == NULL) || (dev->config == NULL) || (dev->data == NULL)) {
        return -1;
    }
    cfg = (const yi_i2c_hpm_config_t *)dev->config;
    data = (yi_i2c_hpm_data_t *)dev->data;
    if(yi_i2c_hpm_mode(frequency, &controller_config.i2c_mode) != 0) {
        return -1;
    }
    controller_config.is_10bit_addressing = false;
    status = i2c_init_master(cfg->instance, data->source_clock_hz,
                             &controller_config);
    if(status != status_success) {
        return -1;
    }
    data->frequency = frequency;
    return 0;
}

/**
 * @brief Execute YiCore I2C messages with the HPM polling driver.
 * @param dev YiCore I2C device.
 * @param address Seven-bit slave address.
 * @param messages Ordered transfer messages.
 * @param count Number of messages.
 * @param timeout_ms Requested timeout; HPM polling driver uses its retry bound.
 * @return Zero on success or the negated HPM status group value on failure.
 */
static int yi_i2c_hpm_transfer(yi_device_t *dev, uint8_t address,
                               yi_i2c_msg_t *messages, uint8_t count,
                               uint32_t timeout_ms)
{
    const yi_i2c_hpm_config_t *cfg;
    hpm_stat_t status = status_invalid_argument;

    (void)timeout_ms;
    if((dev == NULL) || (dev->config == NULL) || (messages == NULL) ||
       (address > 0x7FU)) {
        return -1;
    }
    cfg = (const yi_i2c_hpm_config_t *)dev->config;

    if(count == 1U) {
        uint16_t flags = ((messages[0].flags & YI_I2C_MSG_READ) != 0U)
                       ? I2C_RD : (I2C_WR | I2C_WRITE_CHECK_ACK);
        status = i2c_master_transfer(cfg->instance, address,
                                     messages[0].buffer,
                                     messages[0].length, flags);
    } else if((count == 2U) &&
              ((messages[0].flags & YI_I2C_MSG_READ) == 0U) &&
              ((messages[1].flags & (YI_I2C_MSG_READ |
                                     YI_I2C_MSG_RESTART |
                                     YI_I2C_MSG_STOP)) ==
               (YI_I2C_MSG_READ | YI_I2C_MSG_RESTART |
                YI_I2C_MSG_STOP))) {
        status = i2c_master_seq_transmit_check_ack(
            cfg->instance, address, messages[0].buffer, messages[0].length,
            i2c_frist_frame, true);
        if(status == status_success) {
            status = i2c_master_seq_receive(
                cfg->instance, address, messages[1].buffer,
                messages[1].length, i2c_last_frame);
        }
    }
    return yi_i2c_hpm_result(status);
}

/**
 * @brief Initialize an HPM I2C device from its board configuration.
 * @param config Pointer to yi_i2c_hpm_config_t.
 * @return Zero on success or negative one on failure.
 */
int yi_i2c_hpm_init(const void *config)
{
    const yi_i2c_hpm_config_t *cfg =
        (const yi_i2c_hpm_config_t *)config;
    yi_i2c_hpm_data_t *data;
    i2c_config_t controller_config;

    if((cfg == NULL) || (cfg->instance == NULL) ||
       (cfg->hardware_init == NULL) || (cfg->runtime == NULL) ||
       (yi_i2c_hpm_mode(cfg->initial_frequency,
                        &controller_config.i2c_mode) != 0)) {
        return -1;
    }
    data = cfg->runtime;
    data->source_clock_hz = cfg->hardware_init();
    controller_config.is_10bit_addressing = false;
    if((data->source_clock_hz == 0U) ||
       (i2c_init_master(cfg->instance, data->source_clock_hz,
                        &controller_config) != status_success)) {
        return -1;
    }
    data->frequency = cfg->initial_frequency;
    return 0;
}

/** YiCore dispatch table implemented by the HPM I2C backend. */
const yi_i2c_api_t yi_i2c_hpm_api = {
    .configure = yi_i2c_hpm_configure,
    .transfer = yi_i2c_hpm_transfer
};
