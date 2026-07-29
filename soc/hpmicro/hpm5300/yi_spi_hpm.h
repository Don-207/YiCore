/**
 * @file yi_spi_hpm.h
 * @brief Adapt HPMicro hardware SPI controllers to the YiCore SPI API.
 * @author Don
 * @date 2026-07-29
 * @version 1.0.0
 */

#ifndef YI_SPI_HPM_H
#define YI_SPI_HPM_H

#include "yi_spi.h"
#include "hpm_spi_drv.h"

/** HPM SPI hardware initialization callback returning its source clock. */
typedef uint32_t (*yi_spi_hpm_hardware_init_t)(void);

/** Immutable configuration for one HPM-backed YiCore SPI device. */
typedef struct yi_spi_hpm_config {
    SPI_Type *instance; /**< HPM SPI register block. */
    yi_spi_hpm_hardware_init_t hardware_init; /**< Board pin/clock setup. */
    uint32_t max_frequency; /**< Maximum supported SCLK in hertz. */
    struct yi_spi_hpm_data *runtime; /**< Linked device runtime state. */
} yi_spi_hpm_config_t;

/** Runtime state for one HPM-backed YiCore SPI device. */
typedef struct yi_spi_hpm_data {
    uint32_t source_clock_hz; /**< Peripheral source clock in hertz. */
} yi_spi_hpm_data_t;

/**
 * @brief Initialize an HPM SPI device from its board configuration.
 * @param config Pointer to yi_spi_hpm_config_t.
 * @return Zero on success or negative one on failure.
 */
int yi_spi_hpm_init(const void *config);

/** YiCore dispatch table implemented by the HPM SPI backend. */
extern const yi_spi_api_t yi_spi_hpm_api;

#endif
