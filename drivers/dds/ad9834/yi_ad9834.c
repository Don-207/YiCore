/**
 * @file yi_ad9834.c
 * @brief YiCore AD9834 DDS implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_ad9834.h"

#define AD9834_CTRL_B28      (1U << 13)
#define AD9834_CTRL_FSELECT  (1U << 11)
#define AD9834_CTRL_PSELECT  (1U << 10)
#define AD9834_CTRL_RESET    (1U << 8)
#define AD9834_CTRL_SLEEP1   (1U << 7)
#define AD9834_CTRL_SLEEP12  (1U << 6)
#define AD9834_CTRL_OPBITEN  (1U << 5)
#define AD9834_CTRL_DIV2     (1U << 3)
#define AD9834_CTRL_MODE     (1U << 1)
#define AD9834_CTRL_WAVE_MASK (AD9834_CTRL_OPBITEN | AD9834_CTRL_DIV2 | AD9834_CTRL_MODE)
#define AD9834_FREQ0         0x4000U
#define AD9834_FREQ1         0x8000U
#define AD9834_PHASE0        0xC000U
#define AD9834_PHASE1        0xE000U
#define AD9834_DATA_MASK     0x3FFFU
#define AD9834_PHASE_MASK    0x0FFFU
#define AD9834_TUNING_BITS   28U

/** @brief Write one 16-bit AD9834 word. */
static int yi_ad9834_write(const yi_ad9834_config_t *cfg, uint16_t word)
{
    uint8_t tx[2] = {(uint8_t)(word >> 8), (uint8_t)word};
    return yi_spi_transceive(cfg->spi, &cfg->spi_config, tx, NULL,
                             sizeof(tx), cfg->transfer_timeout_ms);
}

/** @brief Validate a device and return typed configuration and data. */
static int yi_ad9834_device(yi_device_t *dev,
                            const yi_ad9834_config_t **cfg,
                            yi_ad9834_data_t **data)
{
    if(!yi_device_is_ready(dev) || (dev->config == NULL) || (dev->data == NULL) ||
       (((yi_ad9834_data_t *)dev->data)->initialized == 0U)) { return -1; }
    *cfg = (const yi_ad9834_config_t *)dev->config;
    *data = (yi_ad9834_data_t *)dev->data;
    return 0;
}

/** @brief Write and cache the control register. */
static int yi_ad9834_write_control(const yi_ad9834_config_t *cfg,
                                   yi_ad9834_data_t *data, uint16_t control)
{
    if(yi_ad9834_write(cfg, control) != 0) { return -1; }
    data->control = control;
    return 0;
}

/** @brief Initialize the DDS in reset state. */
int yi_ad9834_init(const void *config)
{
    const yi_ad9834_config_t *cfg = config;
    yi_ad9834_data_t *data;
    uint16_t control = AD9834_CTRL_B28 | AD9834_CTRL_RESET;

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->spi) || !yi_device_is_ready(cfg->spi_config.cs_gpio) ||
       (cfg->spi_config.mode != 2U) || (cfg->spi_config.frequency == 0U) ||
       (cfg->mclk_frequency == 0U) || (cfg->transfer_timeout_ms == 0U)) { return -1; }
    data = (yi_ad9834_data_t *)cfg->self->data;
    data->initialized = 0U;
    if(yi_ad9834_write(cfg, control) != 0) { return -1; }
    data->control = control;
    data->initialized = 1U;
    return 0;
}

/** @brief Program one 28-bit frequency register. */
int yi_ad9834_set_frequency(yi_device_t *dev, uint8_t reg, uint32_t frequency_hz)
{
    const yi_ad9834_config_t *cfg;
    yi_ad9834_data_t *data;
    uint32_t tuning;
    uint16_t address;

    if((yi_ad9834_device(dev, &cfg, &data) != 0) ||
       (reg >= YI_AD9834_FREQUENCY_REGISTERS) ||
       (frequency_hz > (cfg->mclk_frequency / 2U))) { return -1; }
    tuning = (uint32_t)((((uint64_t)frequency_hz << AD9834_TUNING_BITS) +
                         (cfg->mclk_frequency / 2U)) / cfg->mclk_frequency);
    address = (reg == 0U) ? AD9834_FREQ0 : AD9834_FREQ1;
    if(yi_ad9834_write(cfg, (uint16_t)(address | (tuning & AD9834_DATA_MASK))) != 0)
    { return -1; }
    return yi_ad9834_write(cfg, (uint16_t)(address |
                            ((tuning >> 14) & AD9834_DATA_MASK)));
}

/** @brief Program one 12-bit phase register. */
int yi_ad9834_set_phase(yi_device_t *dev, uint8_t reg, uint16_t phase_tenths)
{
    const yi_ad9834_config_t *cfg;
    yi_ad9834_data_t *data;
    uint16_t value, address;

    if((yi_ad9834_device(dev, &cfg, &data) != 0) ||
       (reg >= YI_AD9834_PHASE_REGISTERS) || (phase_tenths >= 3600U)) { return -1; }
    value = (uint16_t)((((uint32_t)phase_tenths * 4096U) + 1800U) / 3600U);
    address = (reg == 0U) ? AD9834_PHASE0 : AD9834_PHASE1;
    return yi_ad9834_write(cfg, (uint16_t)(address | (value & AD9834_PHASE_MASK)));
}

/** @brief Select active frequency and phase registers. */
int yi_ad9834_select(yi_device_t *dev, uint8_t frequency_reg, uint8_t phase_reg)
{
    const yi_ad9834_config_t *cfg;
    yi_ad9834_data_t *data;
    uint16_t control;

    if((yi_ad9834_device(dev, &cfg, &data) != 0) ||
       (frequency_reg >= 2U) || (phase_reg >= 2U)) { return -1; }
    control = data->control & (uint16_t)~(AD9834_CTRL_FSELECT | AD9834_CTRL_PSELECT);
    if(frequency_reg != 0U) { control |= AD9834_CTRL_FSELECT; }
    if(phase_reg != 0U) { control |= AD9834_CTRL_PSELECT; }
    return yi_ad9834_write_control(cfg, data, control);
}

/** @brief Select the output waveform. */
int yi_ad9834_set_waveform(yi_device_t *dev, yi_ad9834_waveform_t waveform)
{
    const yi_ad9834_config_t *cfg;
    yi_ad9834_data_t *data;
    uint16_t control;

    if((yi_ad9834_device(dev, &cfg, &data) != 0) ||
       (waveform > YI_AD9834_WAVE_SQUARE_DIV2)) { return -1; }
    control = data->control & (uint16_t)~AD9834_CTRL_WAVE_MASK;
    if(waveform == YI_AD9834_WAVE_TRIANGLE) { control |= AD9834_CTRL_MODE; }
    else if(waveform == YI_AD9834_WAVE_SQUARE) {
        control |= AD9834_CTRL_OPBITEN | AD9834_CTRL_DIV2;
    } else if(waveform == YI_AD9834_WAVE_SQUARE_DIV2) {
        control |= AD9834_CTRL_OPBITEN;
    }
    return yi_ad9834_write_control(cfg, data, control);
}

/** @brief Assert or release digital reset. */
int yi_ad9834_set_reset(yi_device_t *dev, bool reset)
{
    const yi_ad9834_config_t *cfg;
    yi_ad9834_data_t *data;
    uint16_t control;
    if(yi_ad9834_device(dev, &cfg, &data) != 0) { return -1; }
    control = reset ? (data->control | AD9834_CTRL_RESET) :
                      (data->control & (uint16_t)~AD9834_CTRL_RESET);
    return yi_ad9834_write_control(cfg, data, control);
}

/** @brief Control MCLK and DAC sleep states. */
int yi_ad9834_set_sleep(yi_device_t *dev, bool disable_mclk, bool disable_dac)
{
    const yi_ad9834_config_t *cfg;
    yi_ad9834_data_t *data;
    uint16_t control;
    if(yi_ad9834_device(dev, &cfg, &data) != 0) { return -1; }
    control = data->control & (uint16_t)~(AD9834_CTRL_SLEEP1 | AD9834_CTRL_SLEEP12);
    if(disable_mclk) { control |= AD9834_CTRL_SLEEP1; }
    if(disable_dac) { control |= AD9834_CTRL_SLEEP12; }
    return yi_ad9834_write_control(cfg, data, control);
}
