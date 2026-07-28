/**
 * @file yi_mcuboot_upgrade.c
 * @brief Implement transport-independent MCUboot secondary-slot updates.
 * @author Don
 * @date 2026-07-28
 * @version 1.0.0
 */

#include "yi_mcuboot_upgrade.h"

#include <string.h>

#include "yi_flash.h"

#define YI_MCUBOOT_IMAGE_MAGIC 0x96F3B83DU
#define YI_MCUBOOT_HEADER_BYTES 32U
#define YI_MCUBOOT_MAGIC_BYTES 16U
#define YI_MCUBOOT_ALIGN_BYTES 8U
#define YI_MCUBOOT_SWAP_TYPE_TEST 2U

typedef struct
{
    uint32_t magic; /**< MCUboot image header magic. */
    uint32_t load_address; /**< Image load address, normally zero for XIP. */
    uint16_t header_size; /**< Signed image header size in bytes. */
    uint16_t protected_tlv_size; /**< Protected TLV byte count. */
    uint32_t image_size; /**< Executable payload size in bytes. */
    uint32_t flags; /**< MCUboot image flags. */
    uint8_t version[8]; /**< Encoded MCUboot image version. */
    uint32_t pad; /**< Header padding word. */
} yi_mcuboot_raw_header_t;

static const uint8_t yi_mcuboot_trailer_magic[YI_MCUBOOT_MAGIC_BYTES] =
{
    0x77U, 0xC2U, 0x95U, 0xF3U, 0x60U, 0xD2U, 0xEFU, 0x7FU,
    0x35U, 0x52U, 0x50U, 0x0FU, 0x2CU, 0xB6U, 0x79U, 0x80U
};

/**
 * @brief Update a CRC-16/MODBUS accumulator.
 * @param crc Current accumulator value.
 * @param data Input bytes.
 * @param length Number of input bytes.
 * @return Updated CRC value.
 */
static uint16_t yi_mcuboot_crc16_update(uint16_t crc,
                                        const uint8_t *data,
                                        uint32_t length)
{
    uint32_t index; /**< Current input byte index. */
    uint8_t bit; /**< Current bit within one input byte. */

    for(index = 0U; index < length; index++)
    {
        crc ^= data[index];
        for(bit = 0U; bit < 8U; bit++)
        {
            crc = ((crc & 1U) != 0U) ?
                  (uint16_t)((crc >> 1U) ^ 0xA001U) :
                  (uint16_t)(crc >> 1U);
        }
    }
    return crc;
}

yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_init(
    yi_mcuboot_upgrade_t *upgrade,
    const yi_mcuboot_upgrade_config_t *config)
{
    uint32_t erase_size; /**< Secondary-slot erase granularity in bytes. */
    uint32_t write_size; /**< Secondary-slot write granularity in bytes. */

    if((upgrade == NULL) || (config == NULL) ||
       (yi_partition_validate(config->secondary) != 0))
    {
        return YI_MCUBOOT_UPGRADE_BAD_ARGUMENT;
    }

    erase_size = yi_flash_get_erase_block_size(config->secondary->flash);
    write_size = yi_flash_get_write_block_size(config->secondary->flash);
    if((erase_size == 0U) || (write_size == 0U) ||
       (config->image_header_size < YI_MCUBOOT_HEADER_BYTES) ||
       (config->trailer_size < YI_MCUBOOT_MAGIC_BYTES) ||
       ((config->secondary->offset % erase_size) != 0U) ||
       ((config->secondary->size % erase_size) != 0U) ||
       ((config->secondary->size % write_size) != 0U) ||
       (config->image_header_size >=
        (config->secondary->size - config->trailer_size)))
    {
        return YI_MCUBOOT_UPGRADE_BAD_ARGUMENT;
    }

    memset(upgrade, 0, sizeof(*upgrade));
    upgrade->config = *config;
    return YI_MCUBOOT_UPGRADE_OK;
}

yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_begin(
    yi_mcuboot_upgrade_t *upgrade,
    uint32_t image_size,
    uint32_t image_crc16)
{
    uint32_t write_size; /**< Required image-size alignment in bytes. */

    if((upgrade == NULL) || (upgrade->config.secondary == NULL))
    {
        return YI_MCUBOOT_UPGRADE_BAD_ARGUMENT;
    }
    if(upgrade->active)
    {
        return YI_MCUBOOT_UPGRADE_BAD_STATE;
    }

    write_size = yi_flash_get_write_block_size(
        upgrade->config.secondary->flash);
    if((image_size < upgrade->config.image_header_size) ||
       (image_size >
        (upgrade->config.secondary->size - upgrade->config.trailer_size)) ||
       ((image_size % write_size) != 0U))
    {
        return YI_MCUBOOT_UPGRADE_RANGE_ERROR;
    }
    if(yi_partition_erase(upgrade->config.secondary, 0U,
                          upgrade->config.secondary->size) != 0)
    {
        return YI_MCUBOOT_UPGRADE_FLASH_ERROR;
    }

    upgrade->expected_size = image_size;
    upgrade->expected_crc16 = image_crc16 & 0xFFFFU;
    upgrade->received_size = 0U;
    upgrade->running_crc16 = 0xFFFFU;
    upgrade->active = true;
    return YI_MCUBOOT_UPGRADE_OK;
}

yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_write(
    yi_mcuboot_upgrade_t *upgrade,
    uint32_t offset,
    const void *data,
    uint32_t length)
{
    uint32_t write_size; /**< Required block alignment in bytes. */

    if((upgrade == NULL) || (data == NULL) || (length == 0U))
    {
        return YI_MCUBOOT_UPGRADE_BAD_ARGUMENT;
    }
    if(!upgrade->active)
    {
        return YI_MCUBOOT_UPGRADE_BAD_STATE;
    }

    write_size = yi_flash_get_write_block_size(
        upgrade->config.secondary->flash);
    if(offset != upgrade->received_size)
    {
        return YI_MCUBOOT_UPGRADE_OFFSET_ERROR;
    }
    if((length > (upgrade->expected_size - upgrade->received_size)) ||
       ((offset % write_size) != 0U) || ((length % write_size) != 0U))
    {
        return YI_MCUBOOT_UPGRADE_RANGE_ERROR;
    }
    if(yi_partition_write(upgrade->config.secondary, offset,
                           data, length) != 0)
    {
        return YI_MCUBOOT_UPGRADE_FLASH_ERROR;
    }

    upgrade->running_crc16 = yi_mcuboot_crc16_update(
        upgrade->running_crc16, data, length);
    upgrade->received_size += length;
    return YI_MCUBOOT_UPGRADE_OK;
}

yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_resume(
    yi_mcuboot_upgrade_t *upgrade,
    uint32_t image_size,
    uint32_t image_crc16,
    uint32_t received_size)
{
    uint32_t write_size; /**< Required transfer alignment in bytes. */

    if((upgrade == NULL) || (upgrade->config.secondary == NULL) ||
       upgrade->active)
    {
        return YI_MCUBOOT_UPGRADE_BAD_ARGUMENT;
    }
    write_size = yi_flash_get_write_block_size(
        upgrade->config.secondary->flash);
    if((image_size < upgrade->config.image_header_size) ||
       (image_size >
        (upgrade->config.secondary->size - upgrade->config.trailer_size)) ||
       (received_size > image_size) ||
       ((image_size % write_size) != 0U) ||
       ((received_size % write_size) != 0U))
    {
        return YI_MCUBOOT_UPGRADE_RANGE_ERROR;
    }

    upgrade->expected_size = image_size;
    upgrade->expected_crc16 = image_crc16 & 0xFFFFU;
    upgrade->received_size = received_size;
    upgrade->running_crc16 = 0xFFFFU;
    upgrade->active = true;
    return YI_MCUBOOT_UPGRADE_OK;
}

yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_finish(
    yi_mcuboot_upgrade_t *upgrade)
{
    yi_mcuboot_raw_header_t header; /**< MCUboot header read back from secondary flash. */
    uint32_t image_end; /**< End of header, payload, and protected TLVs. */
    uint32_t magic_offset; /**< Secondary trailer magic offset. */
    uint32_t swap_info_offset; /**< Secondary trailer test-swap marker offset. */
    uint16_t calculated_crc = 0xFFFFU; /**< CRC recalculated from persistent flash. */
    uint8_t crc_buffer[64]; /**< Small fixed buffer used while verifying flash. */
    uint32_t crc_offset = 0U; /**< Current verification offset. */
    uint8_t swap_info[YI_MCUBOOT_ALIGN_BYTES] = /**< Test swap marker and erased padding. */
    {
        YI_MCUBOOT_SWAP_TYPE_TEST, 0xFFU, 0xFFU, 0xFFU,
        0xFFU, 0xFFU, 0xFFU, 0xFFU
    };

    if(upgrade == NULL)
    {
        return YI_MCUBOOT_UPGRADE_BAD_ARGUMENT;
    }
    if(!upgrade->active ||
       (upgrade->received_size != upgrade->expected_size))
    {
        return YI_MCUBOOT_UPGRADE_BAD_STATE;
    }
    while(crc_offset < upgrade->expected_size)
    {
        uint32_t chunk = upgrade->expected_size - crc_offset; /**< Bytes verified this pass. */
        if(chunk > sizeof(crc_buffer))
        {
            chunk = sizeof(crc_buffer);
        }
        if(yi_partition_read(upgrade->config.secondary, crc_offset,
                             crc_buffer, chunk) != 0)
        {
            return YI_MCUBOOT_UPGRADE_FLASH_ERROR;
        }
        calculated_crc = yi_mcuboot_crc16_update(calculated_crc,
                                                  crc_buffer, chunk);
        crc_offset += chunk;
    }
    if(calculated_crc != (uint16_t)upgrade->expected_crc16)
    {
        return YI_MCUBOOT_UPGRADE_CRC_ERROR;
    }
    if(yi_partition_read(upgrade->config.secondary, 0U,
                          &header, sizeof(header)) != 0)
    {
        return YI_MCUBOOT_UPGRADE_FLASH_ERROR;
    }

    image_end = (uint32_t)header.header_size + header.image_size +
                (uint32_t)header.protected_tlv_size;
    if((header.magic != YI_MCUBOOT_IMAGE_MAGIC) ||
       (header.header_size != upgrade->config.image_header_size) ||
       (header.image_size == 0U) ||
       (image_end > upgrade->expected_size))
    {
        return YI_MCUBOOT_UPGRADE_IMAGE_ERROR;
    }

    magic_offset = upgrade->config.secondary->size -
                   YI_MCUBOOT_MAGIC_BYTES;
    swap_info_offset = magic_offset - (3U * YI_MCUBOOT_ALIGN_BYTES);
    if((yi_partition_write(upgrade->config.secondary, magic_offset,
                           yi_mcuboot_trailer_magic,
                           sizeof(yi_mcuboot_trailer_magic)) != 0) ||
       (yi_partition_write(upgrade->config.secondary, swap_info_offset,
                           swap_info, sizeof(swap_info)) != 0))
    {
        return YI_MCUBOOT_UPGRADE_FLASH_ERROR;
    }

    upgrade->active = false;
    return YI_MCUBOOT_UPGRADE_OK;
}

yi_mcuboot_upgrade_status_t yi_mcuboot_upgrade_confirm(
    const yi_partition_t *primary)
{
    uint32_t image_ok_offset; /**< Primary trailer image-ok marker offset. */
    uint8_t current; /**< Existing marker byte read from flash. */
    uint8_t image_ok[YI_MCUBOOT_ALIGN_BYTES] = /**< Confirm marker and erased padding. */
    {
        1U, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU, 0xFFU
    };

    if(yi_partition_validate(primary) != 0)
    {
        return YI_MCUBOOT_UPGRADE_BAD_ARGUMENT;
    }
    image_ok_offset = primary->size - YI_MCUBOOT_MAGIC_BYTES -
                      YI_MCUBOOT_ALIGN_BYTES;
    if(yi_partition_read(primary, image_ok_offset, &current,
                          sizeof(current)) != 0)
    {
        return YI_MCUBOOT_UPGRADE_FLASH_ERROR;
    }
    if(current == 1U)
    {
        return YI_MCUBOOT_UPGRADE_OK;
    }
    return (yi_partition_write(primary, image_ok_offset,
                                image_ok, sizeof(image_ok)) == 0) ?
           YI_MCUBOOT_UPGRADE_OK : YI_MCUBOOT_UPGRADE_FLASH_ERROR;
}

uint32_t yi_mcuboot_upgrade_received(const yi_mcuboot_upgrade_t *upgrade)
{
    return (upgrade != NULL) ? upgrade->received_size : 0U;
}
