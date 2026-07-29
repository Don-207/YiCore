/**
 * @file yi_spi_hpm.c
 * @brief Implement the YiCore SPI API with HPMicro polling transfers.
 * @author Don
 * @date 2026-07-29
 * @version 1.0.0
 */

#include "yi_spi_hpm.h"

#include <stddef.h>

/**
 * @brief Execute a YiCore SPI transaction with the HPM polling driver.
 * @param dev YiCore SPI device.
 * @param config Per-transaction frequency, mode, and bit order.
 * @param tx Bytes shifted on MOSI, or NULL for dummy bytes.
 * @param rx Destination for MISO bytes, or NULL to discard input.
 * @param length Number of eight-bit frames.
 * @param timeout_ms Requested timeout; HPM polling driver uses its retry bound.
 * @return Zero on success or negative one on failure.
 */
static int yi_spi_hpm_transceive(yi_device_t *dev,
                                 const yi_spi_transfer_config_t *config,
                                 const uint8_t *tx, uint8_t *rx,
                                 uint16_t length, uint32_t timeout_ms)
{
    const yi_spi_hpm_config_t *cfg;
    yi_spi_hpm_data_t *data;
    spi_timing_config_t timing_config;
    spi_format_config_t format_config;
    spi_control_config_t control_config;
    hpm_stat_t status;

    (void)timeout_ms;
    if((dev == NULL) || (dev->config == NULL) || (dev->data == NULL) ||
       (config == NULL) || (config->frequency == 0U) ||
       (config->mode > 3U) || (length == 0U) ||
       ((tx == NULL) && (rx == NULL))) {
        return -1;
    }
    cfg = (const yi_spi_hpm_config_t *)dev->config;
    data = (yi_spi_hpm_data_t *)dev->data;
    if(config->frequency > cfg->max_frequency) {
        return -1;
    }

    spi_master_get_default_timing_config(&timing_config);
    timing_config.master_config.clk_src_freq_in_hz = data->source_clock_hz;
    timing_config.master_config.sclk_freq_in_hz = config->frequency;
    if(spi_master_timing_init(cfg->instance, &timing_config) !=
       status_success) {
        return -1;
    }

    spi_master_get_default_format_config(&format_config);
    format_config.common_config.data_len_in_bits = 8U;
    format_config.common_config.mode = spi_master_mode;
    format_config.common_config.cpol =
        ((config->mode & 2U) != 0U) ? spi_sclk_high_idle
                                    : spi_sclk_low_idle;
    format_config.common_config.cpha =
        ((config->mode & 1U) != 0U) ? spi_sclk_sampling_even_clk_edges
                                    : spi_sclk_sampling_odd_clk_edges;
    format_config.common_config.lsb = config->lsb_first;
    spi_format_init(cfg->instance, &format_config);

    spi_master_get_default_control_config(&control_config);
    control_config.master_config.cmd_enable = false;
    control_config.master_config.addr_enable = false;
    control_config.common_config.trans_mode =
        ((tx != NULL) && (rx != NULL)) ? spi_trans_write_read_together
        : (tx != NULL) ? spi_trans_write_only : spi_trans_read_only;
#if defined(HPM_IP_FEATURE_SPI_CS_SELECT) && (HPM_IP_FEATURE_SPI_CS_SELECT == 1)
    control_config.common_config.cs_index = spi_cs_0;
#endif
    status = spi_transfer(cfg->instance, &control_config, NULL, NULL,
                          (uint8_t *)tx, (tx != NULL) ? length : 0U,
                          rx, (rx != NULL) ? length : 0U);
    if(status == status_success) {
        return 0;
    }
    return (status == status_spi_master_busy) ? -2 : -1;
}

/**
 * @brief Initialize an HPM SPI device from its board configuration.
 * @param config Pointer to yi_spi_hpm_config_t.
 * @return Zero on success or negative one on failure.
 */
int yi_spi_hpm_init(const void *config)
{
    const yi_spi_hpm_config_t *cfg =
        (const yi_spi_hpm_config_t *)config;
    yi_spi_hpm_data_t *data;

    if((cfg == NULL) || (cfg->instance == NULL) ||
       (cfg->hardware_init == NULL) || (cfg->max_frequency == 0U) ||
       (cfg->runtime == NULL)) {
        return -1;
    }
    data = cfg->runtime;
    data->source_clock_hz = cfg->hardware_init();
    return (data->source_clock_hz != 0U) ? 0 : -1;
}

/** YiCore dispatch table implemented by the HPM SPI backend. */
const yi_spi_api_t yi_spi_hpm_api = {
    .transceive = yi_spi_hpm_transceive
};
