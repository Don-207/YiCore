/**
 * @file yi_mcuboot_update.c
 * @brief Implement resumable package policy above the MCUboot slot writer.
 * @author Don
 * @date 2026-07-28
 * @version 1.0.0
 */

#include "yi_mcuboot_update.h"

#include <stddef.h>
#include <string.h>

#define YI_UPDATE_STATE_MAGIC 0x55505354U

typedef char yi_package_header_must_be_64[
    (sizeof(yi_mcuboot_package_header_t) == 64U) ? 1 : -1];

typedef struct
{
    uint32_t magic; /**< Record commit marker written last. */
    uint32_t received_size; /**< Durable page-boundary byte count. */
    uint32_t image_size; /**< Package identity field. */
    uint16_t header_crc16; /**< Package identity CRC. */
    uint16_t record_crc16; /**< CRC of fields following magic. */
} yi_update_record_t;

/**
 * @brief Calculate CRC-16/MODBUS.
 * @param data Input bytes.
 * @param length Input byte count.
 * @return CRC value.
 */
static uint16_t yi_update_crc16(const void *data, uint32_t length)
{
    const uint8_t *bytes = data; /**< Current input byte. */
    uint16_t crc = 0xFFFFU; /**< Running CRC accumulator. */
    uint8_t bit; /**< Current input bit. */

    while(length-- != 0U)
    {
        crc ^= *bytes++;
        for(bit = 0U; bit < 8U; bit++)
        {
            crc = ((crc & 1U) != 0U) ?
                  (uint16_t)((crc >> 1U) ^ 0xA001U) :
                  (uint16_t)(crc >> 1U);
        }
    }
    return crc;
}

/**
 * @brief Compare a package version with the configured running version.
 * @param update Update context.
 * @param header Candidate package.
 * @return True only when the candidate is older.
 */
static bool yi_update_version_older(const yi_mcuboot_update_t *update,
                                    const yi_mcuboot_package_header_t *header)
{
    const yi_mcuboot_version_t *version = &update->config.current_version; /**< Running version. */
    if(header->version_major != version->major)
    {
        return header->version_major < version->major;
    }
    if(header->version_minor != version->minor)
    {
        return header->version_minor < version->minor;
    }
    if(header->version_patch != version->patch)
    {
        return header->version_patch < version->patch;
    }
    return header->version_build < version->build;
}

/**
 * @brief Validate package syntax and product policy.
 * @param update Update context.
 * @param header Candidate package header.
 * @return Update status.
 */
static yi_mcuboot_update_status_t yi_update_validate_header(
    const yi_mcuboot_update_t *update,
    const yi_mcuboot_package_header_t *header)
{
    uint16_t crc; /**< Calculated package-header CRC. */

    if(header == NULL)
    {
        return YI_MCUBOOT_UPDATE_BAD_ARGUMENT;
    }
    crc = yi_update_crc16(header,
        (uint32_t)offsetof(yi_mcuboot_package_header_t, header_crc32));
    if((header->magic != YI_MCUBOOT_PACKAGE_MAGIC) ||
       (header->header_version != YI_MCUBOOT_PACKAGE_HEADER_VERSION) ||
       (header->header_size != sizeof(*header)) ||
       (header->payload_offset < sizeof(*header)) ||
       (header->firmware_type != YI_MCUBOOT_FIRMWARE_TYPE_APPLICATION) ||
       (header->reserved0 != 0U) || (header->reserved1[0] != 0U) ||
       (header->reserved1[1] != 0U) || (header->reserved1[2] != 0U) ||
       (header->flags != 0U) || (crc != (uint16_t)header->header_crc32))
    {
        return YI_MCUBOOT_UPDATE_BAD_HEADER;
    }
    if(header->hardware_id != update->config.hardware_id)
    {
        return YI_MCUBOOT_UPDATE_HW_MISMATCH;
    }
    if(yi_update_version_older(update, header))
    {
        return YI_MCUBOOT_UPDATE_VERSION_REJECTED;
    }
    if((header->image_size < update->config.image_header_size) ||
       (header->image_size >
        (update->config.secondary.size - update->config.trailer_size)) ||
       ((header->image_size %
         yi_flash_get_write_block_size(update->config.secondary.flash)) != 0U))
    {
        return YI_MCUBOOT_UPDATE_SIZE_ERROR;
    }
    return YI_MCUBOOT_UPDATE_OK;
}

/**
 * @brief Calculate one checkpoint record CRC.
 * @param record Record to check.
 * @return CRC value.
 */
static uint16_t yi_update_record_crc(const yi_update_record_t *record)
{
    return yi_update_crc16(&record->received_size,
        sizeof(*record) - sizeof(record->magic) - sizeof(record->record_crc16));
}

/**
 * @brief Locate the last valid checkpoint for an identical package.
 * @param update Update context.
 * @param header Package header.
 * @return Durable received offset, or zero.
 */
static uint32_t yi_update_resume_offset(
    const yi_mcuboot_update_t *update,
    const yi_mcuboot_package_header_t *header)
{
    yi_mcuboot_package_header_t stored; /**< Header stored at state offset zero. */
    yi_update_record_t record; /**< Current checkpoint record. */
    uint32_t offset = sizeof(stored); /**< Current state-partition record offset. */
    uint32_t received = 0U; /**< Last valid durable byte count. */
    uint32_t erase_size = yi_flash_get_erase_block_size(
        update->config.secondary.flash); /**< Image page size. */

    if((yi_partition_read(&update->config.state, 0U,
                           &stored, sizeof(stored)) != 0) ||
       (memcmp(&stored, header, sizeof(stored)) != 0))
    {
        return 0U;
    }
    while((offset + sizeof(record)) <= update->config.state.size)
    {
        if(yi_partition_read(&update->config.state, offset,
                              &record, sizeof(record)) != 0)
        {
            break;
        }
        if(record.magic == 0xFFFFFFFFU)
        {
            break;
        }
        if((record.magic != YI_UPDATE_STATE_MAGIC) ||
           (record.image_size != header->image_size) ||
           (record.header_crc16 != (uint16_t)header->header_crc32) ||
           (record.received_size > header->image_size) ||
           (((record.received_size % erase_size) != 0U) &&
            (record.received_size != header->image_size)) ||
           (record.record_crc16 != yi_update_record_crc(&record)))
        {
            break;
        }
        received = record.received_size;
        offset += sizeof(record);
    }
    return received;
}

/**
 * @brief Append a power-loss-safe checkpoint.
 * @param update Active update context.
 * @param received Durable received offset.
 * @return True on success.
 */
static bool yi_update_append_record(yi_mcuboot_update_t *update,
                                    uint32_t received)
{
    yi_update_record_t record; /**< Checkpoint written payload-first and magic-last. */
    uint32_t offset = sizeof(update->header); /**< Candidate state slot offset. */
    uint32_t magic; /**< Existing slot commit word. */

    memset(&record, 0, sizeof(record));
    record.magic = YI_UPDATE_STATE_MAGIC;
    record.received_size = received;
    record.image_size = update->header.image_size;
    record.header_crc16 = (uint16_t)update->header.header_crc32;
    record.record_crc16 = yi_update_record_crc(&record);

    while((offset + sizeof(record)) <= update->config.state.size)
    {
        if(yi_partition_read(&update->config.state, offset,
                              &magic, sizeof(magic)) != 0)
        {
            return false;
        }
        if(magic == 0xFFFFFFFFU)
        {
            return (yi_partition_write(&update->config.state,
                        offset + sizeof(record.magic), &record.received_size,
                        sizeof(record) - sizeof(record.magic)) == 0) &&
                   (yi_partition_write(&update->config.state, offset,
                        &record.magic, sizeof(record.magic)) == 0);
        }
        offset += sizeof(record);
    }
    return false;
}

yi_mcuboot_update_status_t yi_mcuboot_update_init(
    yi_mcuboot_update_t *update,
    const yi_mcuboot_update_config_t *config)
{
    yi_mcuboot_upgrade_config_t writer_config; /**< Low-level writer configuration. */

    if((update == NULL) || (config == NULL) ||
       (yi_partition_validate(&config->primary) != 0) ||
       (yi_partition_validate(&config->secondary) != 0) ||
       (yi_partition_validate(&config->state) != 0))
    {
        return YI_MCUBOOT_UPDATE_BAD_ARGUMENT;
    }
    memset(update, 0, sizeof(*update));
    update->config = *config;
    writer_config.secondary = &update->config.secondary;
    writer_config.image_header_size = config->image_header_size;
    writer_config.trailer_size = config->trailer_size;
    return (yi_mcuboot_upgrade_init(&update->writer, &writer_config) ==
            YI_MCUBOOT_UPGRADE_OK) ?
           YI_MCUBOOT_UPDATE_OK : YI_MCUBOOT_UPDATE_BAD_ARGUMENT;
}

yi_mcuboot_update_status_t yi_mcuboot_update_begin(
    yi_mcuboot_update_t *update,
    const yi_mcuboot_package_header_t *header)
{
    yi_mcuboot_update_status_t status; /**< Header validation result. */
    uint32_t resume; /**< Durable resume byte offset. */
    uint32_t erase_size; /**< Secondary erase page size. */
    yi_mcuboot_package_header_t stored_header; /**< Previously persisted package identity. */
    bool same_header; /**< Whether the persisted session belongs to this package. */

    if(update == NULL)
    {
        return YI_MCUBOOT_UPDATE_BAD_ARGUMENT;
    }
    status = yi_update_validate_header(update, header);
    if(status != YI_MCUBOOT_UPDATE_OK)
    {
        return status;
    }
    if(update->active)
    {
        return YI_MCUBOOT_UPDATE_STATE_ERROR;
    }

    resume = yi_update_resume_offset(update, header);
    same_header =
        (yi_partition_read(&update->config.state, 0U, &stored_header,
                           sizeof(stored_header)) == 0) &&
        (memcmp(&stored_header, header, sizeof(*header)) == 0);
    update->header = *header;
    if((resume != 0U) || same_header)
    {
        erase_size = yi_flash_get_erase_block_size(
            update->config.secondary.flash);
        if((resume < header->image_size) &&
           (yi_partition_erase(&update->config.secondary, resume,
                                erase_size) != 0))
        {
            return YI_MCUBOOT_UPDATE_FLASH_ERROR;
        }
        if(yi_mcuboot_upgrade_resume(&update->writer, header->image_size,
             header->image_crc32, resume) != YI_MCUBOOT_UPGRADE_OK)
        {
            return YI_MCUBOOT_UPDATE_STATE_ERROR;
        }
        update->active = true;
        return YI_MCUBOOT_UPDATE_OK;
    }

    if((yi_mcuboot_upgrade_begin(&update->writer, header->image_size,
                                  header->image_crc32) !=
        YI_MCUBOOT_UPGRADE_OK) ||
       (yi_partition_erase(&update->config.state, 0U,
                            update->config.state.size) != 0) ||
       (yi_partition_write(&update->config.state, 0U,
                            header, sizeof(*header)) != 0))
    {
        return YI_MCUBOOT_UPDATE_FLASH_ERROR;
    }
    update->active = true;
    return YI_MCUBOOT_UPDATE_OK;
}

yi_mcuboot_update_status_t yi_mcuboot_update_write(
    yi_mcuboot_update_t *update,
    uint32_t offset,
    const void *data,
    uint32_t length)
{
    yi_mcuboot_upgrade_status_t status; /**< Low-level write result. */
    uint32_t received; /**< Byte count after a successful write. */
    uint32_t erase_size; /**< Checkpoint interval in bytes. */

    if((update == NULL) || !update->active)
    {
        return YI_MCUBOOT_UPDATE_STATE_ERROR;
    }
    status = yi_mcuboot_upgrade_write(&update->writer, offset, data, length);
    if(status == YI_MCUBOOT_UPGRADE_OFFSET_ERROR)
    {
        return YI_MCUBOOT_UPDATE_OFFSET_ERROR;
    }
    if(status != YI_MCUBOOT_UPGRADE_OK)
    {
        return YI_MCUBOOT_UPDATE_FLASH_ERROR;
    }
    received = yi_mcuboot_upgrade_received(&update->writer);
    erase_size = yi_flash_get_erase_block_size(update->config.secondary.flash);
    if((((received % erase_size) == 0U) ||
        (received == update->header.image_size)) &&
       !yi_update_append_record(update, received))
    {
        return YI_MCUBOOT_UPDATE_RECORD_ERROR;
    }
    return YI_MCUBOOT_UPDATE_OK;
}

yi_mcuboot_update_status_t yi_mcuboot_update_finish(
    yi_mcuboot_update_t *update)
{
    uint8_t raw_header[32]; /**< MCUboot image header read for version matching. */
    yi_mcuboot_upgrade_status_t status; /**< Low-level validation result. */
    uint16_t revision; /**< MCUboot revision decoded from the header. */
    uint32_t build; /**< MCUboot build number decoded from the header. */

    if((update == NULL) || !update->active)
    {
        return YI_MCUBOOT_UPDATE_STATE_ERROR;
    }
    if(yi_partition_read(&update->config.secondary, 0U,
                          raw_header, sizeof(raw_header)) != 0)
    {
        return YI_MCUBOOT_UPDATE_FLASH_ERROR;
    }
    revision = (uint16_t)raw_header[22] | ((uint16_t)raw_header[23] << 8U);
    build = (uint32_t)raw_header[24] |
            ((uint32_t)raw_header[25] << 8U) |
            ((uint32_t)raw_header[26] << 16U) |
            ((uint32_t)raw_header[27] << 24U);
    if((raw_header[20] != (uint8_t)update->header.version_major) ||
       (raw_header[21] != (uint8_t)update->header.version_minor) ||
       (revision != update->header.version_patch) ||
       (build != update->header.version_build))
    {
        return YI_MCUBOOT_UPDATE_IMAGE_ERROR;
    }
    status = yi_mcuboot_upgrade_finish(&update->writer);
    if(status == YI_MCUBOOT_UPGRADE_CRC_ERROR)
    {
        return YI_MCUBOOT_UPDATE_CRC_ERROR;
    }
    if(status == YI_MCUBOOT_UPGRADE_IMAGE_ERROR)
    {
        return YI_MCUBOOT_UPDATE_IMAGE_ERROR;
    }
    if(status != YI_MCUBOOT_UPGRADE_OK)
    {
        return YI_MCUBOOT_UPDATE_FLASH_ERROR;
    }
    update->active = false;
    (void)yi_partition_erase(&update->config.state, 0U,
                              update->config.state.size);
    return YI_MCUBOOT_UPDATE_OK;
}

yi_mcuboot_update_status_t yi_mcuboot_update_confirm(
    yi_mcuboot_update_t *update)
{
    if(update == NULL)
    {
        return YI_MCUBOOT_UPDATE_BAD_ARGUMENT;
    }
    return (yi_mcuboot_upgrade_confirm(&update->config.primary) ==
            YI_MCUBOOT_UPGRADE_OK) ?
           YI_MCUBOOT_UPDATE_OK : YI_MCUBOOT_UPDATE_FLASH_ERROR;
}

uint32_t yi_mcuboot_update_received(const yi_mcuboot_update_t *update)
{
    return (update != NULL) ?
           yi_mcuboot_upgrade_received(&update->writer) : 0U;
}

bool yi_mcuboot_update_active(const yi_mcuboot_update_t *update)
{
    return (update != NULL) && update->active;
}
