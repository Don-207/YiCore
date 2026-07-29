/**
 * @file yi_i2c_hpm.h
 * @brief Adapt HPMicro hardware I2C controllers to the YiCore I2C API.
 * @author Don
 * @date 2026-07-29
 * @version 1.0.0
 */

#ifndef YI_I2C_HPM_H
#define YI_I2C_HPM_H

#include "yi_i2c.h"
#include "hpm_i2c_drv.h"

/** HPM I2C hardware initialization callback returning its source clock. */
typedef uint32_t (*yi_i2c_hpm_hardware_init_t)(void);

/** Immutable configuration for one HPM-backed YiCore I2C device. */
typedef struct yi_i2c_hpm_config {
    I2C_Type *instance; /**< HPM I2C register block. */
    yi_i2c_hpm_hardware_init_t hardware_init; /**< Board pin/clock setup. */
    uint32_t initial_frequency; /**< Startup bus frequency in hertz. */
    struct yi_i2c_hpm_data *runtime; /**< Linked device runtime state. */
} yi_i2c_hpm_config_t;

/** Runtime state for one HPM-backed YiCore I2C device. */
typedef struct yi_i2c_hpm_data {
    uint32_t source_clock_hz; /**< Peripheral source clock in hertz. */
    uint32_t frequency; /**< Active bus frequency in hertz. */
} yi_i2c_hpm_data_t;

/**
 * @brief Initialize an HPM I2C device from its board configuration.
 * @param config Pointer to yi_i2c_hpm_config_t with a linked runtime object.
 * @return Zero on success or negative one on failure.
 */
int yi_i2c_hpm_init(const void *config);

/** YiCore dispatch table implemented by the HPM I2C backend. */
extern const yi_i2c_api_t yi_i2c_hpm_api;

#endif
