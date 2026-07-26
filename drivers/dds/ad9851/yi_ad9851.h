/**
 * @file yi_ad9851.h
 * @brief YiCore AD9851 DDS interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_AD9851_H
#define YI_AD9851_H

#include "yi_gpio.h"

typedef struct {
    yi_device_t *self; /**< DDS device instance. */
    yi_device_t *w_clk_gpio; /**< Word-clock output GPIO. */
    yi_device_t *fq_ud_gpio; /**< Frequency-update output GPIO. */
    yi_device_t *data_gpio; /**< Serial-data output GPIO. */
    yi_device_t *reset_gpio; /**< Hardware-reset output GPIO. */
    uint32_t reference_clock_frequency; /**< External reference clock in hertz. */
    uint32_t pulse_delay_us; /**< GPIO pulse delay in microseconds. */
    bool clock_multiplier; /**< Enable the internal 6x PLL. */
} yi_ad9851_config_t;

typedef struct {
    uint32_t frequency_hz; /**< Cached output frequency. */
    uint8_t phase; /**< Cached five-bit phase code. */
    bool power_down; /**< Cached power-down state. */
    uint8_t initialized; /**< Initialization state. */
} yi_ad9851_data_t;

/** @brief Initialize the AD9851. */
int yi_ad9851_init(const void *config);
/** @brief Hardware-reset the AD9851. */
int yi_ad9851_reset(yi_device_t *dev);
/** @brief Set output frequency. */
int yi_ad9851_set_frequency(yi_device_t *dev, uint32_t frequency_hz);
/** @brief Set phase in tenths of a degree. */
int yi_ad9851_set_phase(yi_device_t *dev, uint16_t phase_tenths);
/** @brief Update frequency and phase atomically. */
int yi_ad9851_set_frequency_phase(yi_device_t *dev, uint32_t frequency_hz,
                                  uint16_t phase_tenths);
/** @brief Control power-down mode. */
int yi_ad9851_set_power_down(yi_device_t *dev, bool power_down);

#define YI_AD9851_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, yi_ad9851_init,  \
                              &_config, &_data, NULL)

#endif
