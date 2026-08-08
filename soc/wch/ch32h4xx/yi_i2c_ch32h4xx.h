/**
 * @file yi_i2c_ch32h4xx.h
 * @brief Define the CH32H4xx backend for the YiCore I2C interface.
 * @author Don
 * @date 2026-08-03
 * @version 1.0.0
 */
#ifndef YI_I2C_CH32H4XX_H
#define YI_I2C_CH32H4XX_H

#include "ch32h417.h"
#include "yi_i2c.h"

/** Immutable description of one CH32H4xx hardware I2C controller. */
typedef struct {
    I2C_TypeDef *instance; /**< Vendor register block hidden below YiCore. */
    GPIO_TypeDef *scl_port; /**< Clock pin GPIO port. */
    GPIO_TypeDef *sda_port; /**< Data pin GPIO port. */
    uint16_t scl_pin; /**< Clock pin mask. */
    uint16_t sda_pin; /**< Data pin mask. */
    uint8_t scl_source; /**< Clock pin number used by AF selection. */
    uint8_t sda_source; /**< Data pin number used by AF selection. */
    uint8_t alternate; /**< WCH alternate-function selector. */
    uint32_t initial_frequency; /**< Initial bus rate in hertz. */
} yi_i2c_ch32h4xx_config_t;

/** Mutable clock state for one CH32H4xx I2C controller. */
typedef struct {
    uint32_t frequency; /**< Active requested bus rate in hertz. */
} yi_i2c_ch32h4xx_data_t;

/** Initialize pins, clocks, and the initial master bus rate. */
int yi_i2c_ch32h4xx_init(const void *config);
/** YiCore I2C dispatch table implemented by the CH32H4xx backend. */
extern const yi_i2c_api_t yi_i2c_ch32h4xx_api;

#endif
