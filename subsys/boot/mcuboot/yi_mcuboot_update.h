/**
 * @file yi_mcuboot_update.h
 * @brief Provide package policy, resume records, and health confirmation for MCUboot upgrades.
 * @author Don
 * @date 2026-07-28
 * @version 1.0.0
 */

#ifndef YI_MCUBOOT_UPDATE_H
#define YI_MCUBOOT_UPDATE_H

#include <stdbool.h>
#include <stdint.h>

#include "yi_mcuboot_upgrade.h"

#ifdef __cplusplus
extern "C" {
#endif

#define YI_MCUBOOT_PACKAGE_MAGIC 0x55504744U
#define YI_MCUBOOT_PACKAGE_HEADER_VERSION 1U
#define YI_MCUBOOT_FIRMWARE_TYPE_APPLICATION 2U

typedef struct
{
    uint32_t magic; /**< Upgrade package marker. */
    uint16_t header_version; /**< Package header format version. */
    uint16_t header_size; /**< Header size, currently 64 bytes. */
    uint16_t hardware_id; /**< Product hardware compatibility identifier. */
    uint8_t firmware_type; /**< Image role, normally application. */
    uint8_t reserved0; /**< Must be zero. */
    uint32_t payload_offset; /**< File offset of the MCUboot-formatted image. */
    uint32_t image_size; /**< Signed MCUboot image byte count. */
    uint32_t image_crc32; /**< Low 16 bits contain image CRC-16/MODBUS. */
    uint16_t version_major; /**< Semantic major version. */
    uint16_t version_minor; /**< Semantic minor version. */
    uint16_t version_patch; /**< Semantic patch version. */
    uint16_t version_build; /**< Build sequence. */
    char build_date[12]; /**< Package build date text. */
    char build_time[9]; /**< Package build time text. */
    uint8_t reserved1[3]; /**< Must be zero. */
    uint32_t flags; /**< Reserved flags; must be zero. */
    uint32_t header_crc32; /**< Low 16 bits contain header CRC-16/MODBUS. */
} yi_mcuboot_package_header_t;

typedef struct
{
    uint16_t major; /**< Current major version. */
    uint16_t minor; /**< Current minor version. */
    uint16_t patch; /**< Current patch version. */
    uint16_t build; /**< Current build sequence. */
} yi_mcuboot_version_t;

typedef struct
{
    yi_partition_t primary; /**< Running primary slot. */
    yi_partition_t secondary; /**< Download secondary slot. */
    yi_partition_t state; /**< Dedicated persistent resume-record partition. */
    uint16_t hardware_id; /**< Accepted package hardware identifier. */
    yi_mcuboot_version_t current_version; /**< Version used for downgrade rejection. */
    uint32_t image_header_size; /**< Required MCUboot image header size. */
    uint32_t trailer_size; /**< Reserved MCUboot trailer bytes. */
} yi_mcuboot_update_config_t;

typedef enum
{
    YI_MCUBOOT_UPDATE_OK = 0, /**< Operation completed successfully. */
    YI_MCUBOOT_UPDATE_BAD_ARGUMENT = 1, /**< Invalid argument. */
    YI_MCUBOOT_UPDATE_BAD_HEADER = 2, /**< Malformed package header. */
    YI_MCUBOOT_UPDATE_HW_MISMATCH = 3, /**< Package targets different hardware. */
    YI_MCUBOOT_UPDATE_SIZE_ERROR = 4, /**< Image does not fit the slot. */
    YI_MCUBOOT_UPDATE_VERSION_REJECTED = 5, /**< Package version is older. */
    YI_MCUBOOT_UPDATE_STATE_ERROR = 6, /**< Session state is invalid. */
    YI_MCUBOOT_UPDATE_FLASH_ERROR = 7, /**< Flash operation failed. */
    YI_MCUBOOT_UPDATE_OFFSET_ERROR = 8, /**< Data was not supplied sequentially. */
    YI_MCUBOOT_UPDATE_CRC_ERROR = 9, /**< Image CRC failed. */
    YI_MCUBOOT_UPDATE_IMAGE_ERROR = 10, /**< MCUboot header or version failed. */
    YI_MCUBOOT_UPDATE_RECORD_ERROR = 11 /**< Persistent checkpoint failed. */
} yi_mcuboot_update_status_t;

typedef struct
{
    yi_mcuboot_update_config_t config; /**< Validated configuration owned by the context. */
    yi_mcuboot_upgrade_t writer; /**< Low-level secondary-slot writer. */
    yi_mcuboot_package_header_t header; /**< Active package header. */
    bool active; /**< True while receiving a package. */
} yi_mcuboot_update_t;

/**
 * @brief Initialize a complete MCUboot application update context.
 * @param update Caller-owned context used outside interrupt context.
 * @param config Product partitions and compatibility policy.
 * @return Update status.
 */
yi_mcuboot_update_status_t yi_mcuboot_update_init(
    yi_mcuboot_update_t *update,
    const yi_mcuboot_update_config_t *config);

/**
 * @brief Start or resume a package download.
 * @param update Initialized context.
 * @param header Validated package header received from the transport.
 * @return Update status; received offset is available through received().
 */
yi_mcuboot_update_status_t yi_mcuboot_update_begin(
    yi_mcuboot_update_t *update,
    const yi_mcuboot_package_header_t *header);

/**
 * @brief Store one sequential package payload block and checkpoint page boundaries.
 * @param update Active context.
 * @param offset Payload-relative byte offset.
 * @param data Block bytes.
 * @param length Non-zero flash-aligned byte count.
 * @return Update status.
 */
yi_mcuboot_update_status_t yi_mcuboot_update_write(
    yi_mcuboot_update_t *update,
    uint32_t offset,
    const void *data,
    uint32_t length);

/**
 * @brief Validate the complete MCUboot image and request a test swap.
 * @param update Active context.
 * @return Update status.
 */
yi_mcuboot_update_status_t yi_mcuboot_update_finish(
    yi_mcuboot_update_t *update);

/**
 * @brief Confirm the running test image after product health checks.
 * @param update Initialized context.
 * @return Update status.
 */
yi_mcuboot_update_status_t yi_mcuboot_update_confirm(
    yi_mcuboot_update_t *update);

/**
 * @brief Return the active session byte count.
 * @param update Context, or NULL.
 * @return Received image bytes.
 */
uint32_t yi_mcuboot_update_received(const yi_mcuboot_update_t *update);

/**
 * @brief Report whether a download session is active.
 * @param update Context, or NULL.
 * @return True while receiving.
 */
bool yi_mcuboot_update_active(const yi_mcuboot_update_t *update);

#ifdef __cplusplus
}
#endif

#endif
