/**
 * @file yi_ad9851.c
 * @brief YiCore AD9851 DDS implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_ad9851.h"
#include "yi_system.h"

#define AD9851_MULTIPLIER 6U
#define AD9851_CONTROL_MULTIPLIER (1U << 0)
#define AD9851_CONTROL_POWER_DOWN (1U << 2)

/** @brief Apply the configured GPIO pulse delay. */
static void yi_ad9851_delay(const yi_ad9851_config_t *cfg)
{
    if(cfg->pulse_delay_us != 0U) { yi_system_delay_us(cfg->pulse_delay_us); }
}

/** @brief Generate one active-high control pulse. */
static int yi_ad9851_pulse(const yi_ad9851_config_t *cfg, yi_device_t *gpio)
{
    if(yi_gpio_set(gpio, YI_GPIO_HIGH) != 0) { return -1; }
    yi_ad9851_delay(cfg);
    if(yi_gpio_set(gpio, YI_GPIO_LOW) != 0) { return -1; }
    yi_ad9851_delay(cfg);
    return 0;
}

/** @brief Validate a device and return typed pointers. */
static int yi_ad9851_device(yi_device_t *dev,
                            const yi_ad9851_config_t **cfg,
                            yi_ad9851_data_t **data)
{
    if(!yi_device_is_ready(dev) || (dev->config == NULL) || (dev->data == NULL) ||
       (((yi_ad9851_data_t *)dev->data)->initialized == 0U)) { return -1; }
    *cfg = (const yi_ad9851_config_t *)dev->config;
    *data = (yi_ad9851_data_t *)dev->data;
    return 0;
}

/** @brief Calculate the effective DDS system clock. */
static uint32_t yi_ad9851_system_clock(const yi_ad9851_config_t *cfg)
{
    return cfg->reference_clock_frequency *
           (cfg->clock_multiplier ? AD9851_MULTIPLIER : 1U);
}

/** @brief Shift one 40-bit tuning word LSB first. */
static int yi_ad9851_write(const yi_ad9851_config_t *cfg,
                           uint32_t frequency_hz, uint8_t phase,
                           bool power_down)
{
    uint32_t system_clock = yi_ad9851_system_clock(cfg);
    uint32_t tuning = (uint32_t)((((uint64_t)frequency_hz << 32) +
                                  (system_clock / 2U)) / system_clock);
    uint8_t control = (uint8_t)((phase & 0x1FU) << 3);
    uint8_t byte;

    if(cfg->clock_multiplier) { control |= AD9851_CONTROL_MULTIPLIER; }
    if(power_down) { control |= AD9851_CONTROL_POWER_DOWN; }
    for(uint8_t index = 0U; index < 5U; ++index) {
        byte = (index < 4U) ? (uint8_t)(tuning >> (index * 8U)) : control;
        for(uint8_t bit = 0U; bit < 8U; ++bit) {
            if(yi_gpio_set(cfg->data_gpio, (byte & 1U) ? YI_GPIO_HIGH :
                                                      YI_GPIO_LOW) != 0)
            { return -1; }
            yi_ad9851_delay(cfg);
            if(yi_ad9851_pulse(cfg, cfg->w_clk_gpio) != 0) { return -1; }
            byte >>= 1;
        }
    }
    return yi_ad9851_pulse(cfg, cfg->fq_ud_gpio);
}

/** @brief Reset the chip and enter serial mode. */
static int yi_ad9851_hardware_reset(const yi_ad9851_config_t *cfg)
{
    if(yi_gpio_set(cfg->reset_gpio, YI_GPIO_HIGH) != 0) { return -1; }
    yi_ad9851_delay(cfg);
    if(yi_gpio_set(cfg->reset_gpio, YI_GPIO_LOW) != 0) { return -1; }
    yi_ad9851_delay(cfg);
    if(yi_ad9851_pulse(cfg, cfg->w_clk_gpio) != 0) { return -1; }
    return yi_ad9851_pulse(cfg, cfg->fq_ud_gpio);
}

/** @brief Initialize GPIOs and clear the DDS output. */
int yi_ad9851_init(const void *config)
{
    const yi_ad9851_config_t *cfg = config;
    yi_ad9851_data_t *data;
    uint64_t system_clock;

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->w_clk_gpio) || !yi_device_is_ready(cfg->fq_ud_gpio) ||
       !yi_device_is_ready(cfg->data_gpio) || !yi_device_is_ready(cfg->reset_gpio) ||
       (cfg->reference_clock_frequency == 0U) ||
       (cfg->clock_multiplier && (cfg->reference_clock_frequency > 30000000U)))
    { return -1; }
    system_clock = (uint64_t)cfg->reference_clock_frequency *
                   (cfg->clock_multiplier ? AD9851_MULTIPLIER : 1U);
    if(system_clock > 180000000ULL) { return -1; }
    data = (yi_ad9851_data_t *)cfg->self->data;
    data->initialized = 0U;
    if((yi_gpio_set(cfg->w_clk_gpio, YI_GPIO_LOW) != 0) ||
       (yi_gpio_set(cfg->fq_ud_gpio, YI_GPIO_LOW) != 0) ||
       (yi_gpio_set(cfg->data_gpio, YI_GPIO_LOW) != 0) ||
       (yi_gpio_set(cfg->reset_gpio, YI_GPIO_LOW) != 0) ||
       (yi_ad9851_hardware_reset(cfg) != 0) ||
       (yi_ad9851_write(cfg, 0U, 0U, false) != 0)) { return -1; }
    data->frequency_hz = 0U;
    data->phase = 0U;
    data->power_down = false;
    data->initialized = 1U;
    return 0;
}

/** @brief Reset and clear runtime output state. */
int yi_ad9851_reset(yi_device_t *dev)
{
    const yi_ad9851_config_t *cfg;
    yi_ad9851_data_t *data;
    if(yi_ad9851_device(dev, &cfg, &data) != 0) { return -1; }
    if((yi_ad9851_hardware_reset(cfg) != 0) ||
       (yi_ad9851_write(cfg, 0U, 0U, false) != 0)) { return -1; }
    data->frequency_hz = 0U; data->phase = 0U; data->power_down = false;
    return 0;
}

/** @brief Update frequency and phase on one FQ_UD edge. */
int yi_ad9851_set_frequency_phase(yi_device_t *dev, uint32_t frequency_hz,
                                  uint16_t phase_tenths)
{
    const yi_ad9851_config_t *cfg;
    yi_ad9851_data_t *data;
    uint8_t phase;
    if((yi_ad9851_device(dev, &cfg, &data) != 0) || (phase_tenths >= 3600U) ||
       (frequency_hz > (yi_ad9851_system_clock(cfg) / 2U))) { return -1; }
    phase = (uint8_t)((((uint32_t)phase_tenths * 32U) + 1800U) / 3600U) & 0x1FU;
    if(yi_ad9851_write(cfg, frequency_hz, phase, data->power_down) != 0) { return -1; }
    data->frequency_hz = frequency_hz; data->phase = phase;
    return 0;
}

/** @brief Set the output frequency. */
int yi_ad9851_set_frequency(yi_device_t *dev, uint32_t frequency_hz)
{
    const yi_ad9851_config_t *cfg;
    yi_ad9851_data_t *data;
    if((yi_ad9851_device(dev, &cfg, &data) != 0) ||
       (frequency_hz > (yi_ad9851_system_clock(cfg) / 2U))) { return -1; }
    if(yi_ad9851_write(cfg, frequency_hz, data->phase, data->power_down) != 0)
    { return -1; }
    data->frequency_hz = frequency_hz;
    return 0;
}

/** @brief Set the five-bit phase offset. */
int yi_ad9851_set_phase(yi_device_t *dev, uint16_t phase_tenths)
{
    const yi_ad9851_config_t *cfg;
    yi_ad9851_data_t *data;
    uint8_t phase;
    if((yi_ad9851_device(dev, &cfg, &data) != 0) || (phase_tenths >= 3600U))
    { return -1; }
    phase = (uint8_t)((((uint32_t)phase_tenths * 32U) + 1800U) / 3600U) & 0x1FU;
    if(yi_ad9851_write(cfg, data->frequency_hz, phase, data->power_down) != 0)
    { return -1; }
    data->phase = phase;
    return 0;
}

/** @brief Enable or disable power-down mode. */
int yi_ad9851_set_power_down(yi_device_t *dev, bool power_down)
{
    const yi_ad9851_config_t *cfg;
    yi_ad9851_data_t *data;
    if(yi_ad9851_device(dev, &cfg, &data) != 0) { return -1; }
    if(yi_ad9851_write(cfg, data->frequency_hz, data->phase, power_down) != 0)
    { return -1; }
    data->power_down = power_down;
    return 0;
}
