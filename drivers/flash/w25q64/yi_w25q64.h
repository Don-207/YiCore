/**
 * @file yi_w25q64.h
 * @brief YiCore w25q64 interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_W25Q64_H
#define YI_W25Q64_H

#include "yi_flash.h"
#include "yi_spi.h"

#define YI_W25Q64_SIZE              0x00800000U
#define YI_W25Q64_SECTOR_SIZE       4096U
#define YI_W25Q64_PAGE_SIZE         256U
#define YI_W25Q64_JEDEC_ID          0x00EF4017U

typedef struct
{
    /* Must remain first for the common yi_flash geometry accessors. */
    yi_flash_config_t flash; /**< Flash value. */
    yi_device_t *self; /**< Self value. */
    yi_device_t *spi; /**< Spi value. */
    yi_spi_transfer_config_t spi_config; /**< Spi config value. */
    uint32_t transfer_timeout_ms; /**< Transfer timeout ms value. */
    uint32_t program_timeout_ms; /**< Program timeout ms value. */
    uint32_t erase_timeout_ms; /**< Erase timeout ms value. */} yi_w25q64_config_t;

typedef struct
{
    uint32_t jedec_id; /**< Jedec id value. */
    uint32_t error_count; /**< Error count value. */
    uint8_t tx_buffer[YI_W25Q64_PAGE_SIZE + 4U]; /**< Tx buffer value. */
    uint8_t rx_buffer[YI_W25Q64_PAGE_SIZE + 4U]; /**< Rx buffer value. */} yi_w25q64_data_t;

/**
 * @brief Initialize the module.
 * @param config Device configuration.
 */
int yi_w25q64_init(const void *config);
extern const yi_flash_api_t yi_w25q64_api;

#define YI_W25Q64_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                          \
        _name, _level, _priority, yi_w25q64_init,                       \
        &_config, &_data, (const yi_device_api_t *)&yi_w25q64_api       \
    )

#endif
