/**
 * @file yi_mcuboot_upgrade.h
 * @brief Stream MCUboot-formatted images into a secondary flash partition.
 * @author Don
 * @date 2026-07-28
 * @version 1.0.0
 */

#ifndef YI_MCUBOOT_UPGRADE_H
#define YI_MCUBOOT_UPGRADE_H

#include <stdbool.h>
#include <stdint.h>

#include "yi_partition.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    YI_MCUBOOT_UPGRADE_OK = 0, /**< Operation completed successfully. */
    YI_MCUBOOT_UPGRADE_BAD_ARGUMENT = -1, /**< An argument or configuration is invalid. */
    YI_MCUBOOT_UPGRADE_BAD_STATE = -2, /**< Operation is not valid in the current session state. */
    YI_MCUBOOT_UPGRADE_RANGE_ERROR = -3, /**< Image or block exceeds the secondary slot. */
    YI_MCUBOOT_UPGRADE_OFFSET_ERROR = -4, /**< A block was not supplied in sequential order. */
    YI_MCUBOOT_UPGRADE_FLASH_ERROR = -5, /**< Backing flash operation failed. */
    YI_MCUBOOT_UPGRADE_CRC_ERROR = -6, /**< Completed image CRC does not match. */
    YI_MCUBOOT_UPGRADE_IMAGE_ERROR = -7 /**< MCUboot image header is malformed. */
} yi_mcuboot_upgrade_status_t;

typedef struct
{
    const yi_partition_t *secondary; /**< Secondary slot receiving MCUboot images. */
    uint32_t image_header_size; /**< Required imgtool header size in bytes. */
    uint32_t trailer_size; /**< Reserved MCUboot trailer size in bytes. */
} yi_mcuboot_upgrade_config_t;

typedef struct
{
    yi_mcuboot_upgrade_config_t config; /**< Validated immutable session configuration. */
    uint32_t expected_size; /**< Complete MCUboot image size expected from the transport. */
    uint32_t expected_crc16; /**< Expected CRC-16/MODBUS in the low 16 bits. */
    uint32_t received_size; /**< Number of sequential image bytes committed to flash. */
    uint16_t running_crc16; /**< Incremental CRC-16/MODBUS accumulator. */
    bool active; /**< True while an image transfer is in progress. */
} yi_mcuboot_upgrade_t;

/**
 * @brief Initialize an MCUboot upgrade context.
 * @param upgrade Context owned by the caller and used outside interrupt context.
 * @param config Secondary-slot layout that remains valid for the context lifetime.
 * @return YI_MCUBOOT_UPGRADE_OK on success, otherwise an error status.
 * @note This function does not erase flash or start a transfer.
 */
yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_init(
    yi_mcuboot_upgrade_t *upgrade,
    const yi_mcuboot_upgrade_config_t *config);

/**
 * @brief Erase the secondary slot and start a sequential image transfer.
 * @param upgrade Initialized upgrade context.
 * @param image_size Complete MCUboot image size, excluding erased trailer space.
 * @param image_crc16 Expected CRC-16/MODBUS stored in the low 16 bits.
 * @return YI_MCUBOOT_UPGRADE_OK on success, otherwise an error status.
 * @note Flash erase is synchronous and must not run in interrupt context.
 */
yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_begin(
    yi_mcuboot_upgrade_t *upgrade,
    uint32_t image_size,
    uint32_t image_crc16);

/**
 * @brief Write the next sequential, flash-aligned image block.
 * @param upgrade Active upgrade context.
 * @param offset Required image offset; it must equal the received byte count.
 * @param data Image bytes owned by the caller for the duration of this call.
 * @param length Non-zero block length aligned to the flash write block.
 * @return YI_MCUBOOT_UPGRADE_OK on success, otherwise an error status.
 * @note Flash programming is synchronous and must not run in interrupt context.
 */
yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_write(
    yi_mcuboot_upgrade_t *upgrade,
    uint32_t offset,
    const void *data,
    uint32_t length);

/**
 * @brief Resume a previously checkpointed sequential transfer.
 * @param upgrade Initialized upgrade context.
 * @param image_size Complete MCUboot image size.
 * @param image_crc16 Expected CRC-16/MODBUS in the low 16 bits.
 * @param received_size Page-aligned byte count already stored in secondary.
 * @return YI_MCUBOOT_UPGRADE_OK on success, otherwise an error status.
 * @note The caller must erase the possibly torn page before writing it again.
 */
yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_resume(
    yi_mcuboot_upgrade_t *upgrade,
    uint32_t image_size,
    uint32_t image_crc16,
    uint32_t received_size);

/**
 * @brief Validate the downloaded image and mark it pending for MCUboot.
 * @param upgrade Active, completely received upgrade context.
 * @return YI_MCUBOOT_UPGRADE_OK on success, otherwise an error status.
 * @note Cryptographic authentication remains the bootloader's responsibility.
 */
yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_finish(
    yi_mcuboot_upgrade_t *upgrade);

/**
 * @brief Return the number of image bytes committed in the active session.
 * @param upgrade Upgrade context, or NULL.
 * @return Received byte count, or zero for NULL.
 */
uint32_t yi_mcuboot_upgrade_received(const yi_mcuboot_upgrade_t *upgrade);

/**
 * @brief Confirm the image currently running from the primary slot.
 * @param primary Primary MCUboot slot containing the running image.
 * @return YI_MCUBOOT_UPGRADE_OK on success, otherwise an error status.
 * @note Call only after application health checks pass; not from interrupt context.
 */
yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_confirm(
    const yi_partition_t *primary);

#ifdef __cplusplus
}
#endif

#endif
