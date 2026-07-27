/**
 * @file ecg_protocol.c
 * @brief ECG serial protocol parsing and response packet generation.
 * @author Don
 * @date 2026-07-27
 * @version 1.1.0
 */

#include "ecg_protocol.h"

#include <string.h>

#include "yi_build_info.h"
#include "yi_uart.h"

#define ECG_PACKET_HEAD_LOW 0xAAU
#define ECG_PACKET_HEAD_HIGH 0x55U
#define ECG_COMMAND_MESSAGE_HEAD 0x04U
#define ECG_RESPONSE_MESSAGE_HEAD 0x84U
#define ECG_ACK_DATA_LENGTH 4U
#define ECG_DATA_DATA_LENGTH 8U
#define ECG_VERSION_DATA_LENGTH 36U
#define ECG_PACKET_OVERHEAD 8U
#define ECG_BOOTLOADER_VERSION "00.00.00"

/**
 * @brief Calculate CRC-16/MODBUS over a packet without its trailing CRC.
 * @param data Packet bytes included in the CRC.
 * @param length Number of bytes to process.
 * @return CRC-16/MODBUS value.
 */
static uint16_t ecg_crc16(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFFU; /**< Running CRC accumulator. */
    while(length-- != 0U)
    {
        crc ^= *data++;
        for(uint8_t bit = 0U; bit < 8U; bit++) /**< Process each input bit. */
        {
            crc = ((crc & 1U) != 0U) ?
                (uint16_t)((crc >> 1) ^ 0xA001U) : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

/**
 * @brief Store a 16-bit value in protocol little-endian order.
 * @param destination Two-byte output location.
 * @param value Value to encode.
 */
static void ecg_put_le16(uint8_t *destination, uint16_t value)
{
    destination[0] = (uint8_t)value;
    destination[1] = (uint8_t)(value >> 8);
}

/**
 * @brief Read a 16-bit value in protocol little-endian order.
 * @param source Two-byte input location.
 * @return Decoded value.
 */
static uint16_t ecg_get_le16(const uint8_t *source)
{
    return (uint16_t)((uint16_t)source[0] |
                      ((uint16_t)source[1] << 8));
}

/**
 * @brief Convert ADS1298 lead-off bits to the compact host status field.
 * @param status Raw ADS1298 status word.
 * @return Compact RA/LA/LL/V/RL status bitmap.
 */
static uint8_t ecg_lead_status(uint32_t status)
{
    const uint8_t loff_p = (uint8_t)(status >> 12); /**< Positive lead-off bits. */
    const uint8_t loff_n = (uint8_t)(status >> 4); /**< Negative lead-off bits. */
    uint8_t result = 0U; /**< Compact lead status returned to the host. */
    if((loff_n & 0x03U) != 0U) { result |= (1U << 0); }
    if((loff_p & 0x01U) != 0U) { result |= (1U << 1); }
    if((loff_p & 0x02U) != 0U) { result |= (1U << 2); }
    if((loff_p & 0x04U) != 0U) { result |= (1U << 3); }
    return result;
}

/**
 * @brief Initialize the common packet header and message fields.
 * @param packet Output packet buffer.
 * @param packet_length Complete packet length including CRC.
 * @param message_index Message sequence index.
 */
static void ecg_packet_begin(uint8_t *packet,
                             uint16_t packet_length,
                             uint8_t message_index)
{
    packet[0] = ECG_PACKET_HEAD_LOW;
    packet[1] = ECG_PACKET_HEAD_HIGH;
    ecg_put_le16(&packet[2], packet_length);
    packet[4] = ECG_RESPONSE_MESSAGE_HEAD;
    packet[5] = message_index;
}

/**
 * @brief Finalize a response CRC and send it over UART.
 * @param uart UART output device.
 * @param packet Complete response buffer.
 * @param packet_length Complete response length including CRC.
 * @return 0 on success, otherwise -1.
 */
static int ecg_packet_send(yi_device_t *uart,
                           uint8_t *packet,
                           uint16_t packet_length)
{
    if((uart == NULL) || (packet == NULL) ||
       (packet_length < ECG_PACKET_OVERHEAD))
    {
        return -1;
    }
    ecg_put_le16(&packet[packet_length - 2U],
                 ecg_crc16(packet, packet_length - 2U));
    return yi_uart_write(uart, packet, packet_length);
}

/**
 * @brief Copy a text field and pad unused bytes with spaces.
 * @param destination Fixed-width protocol field.
 * @param field_length Field width in bytes.
 * @param source Null-terminated source string.
 */
static void ecg_copy_text(uint8_t *destination,
                          uint32_t field_length,
                          const char *source)
{
    uint32_t index = 0U; /**< Current destination byte. */
    memset(destination, ' ', field_length);
    while((index < field_length) && (source[index] != '\0'))
    {
        destination[index] = (uint8_t)source[index];
        index++;
    }
}

/**
 * @brief Convert YYYY-MM-DD build date to the 11-byte "Mon DD YYYY" field.
 * @param destination Eleven-byte protocol date field.
 * @param iso_date Build date generated as YYYY-MM-DD.
 */
static void ecg_format_build_date(uint8_t *destination,
                                  const char *iso_date)
{
    static const char *const months[12] = /**< English protocol month names. */
    {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    uint8_t month = 0U; /**< Parsed numeric month in the range 1..12. */

    memset(destination, ' ', 11U);
    if((iso_date == NULL) || (strlen(iso_date) < 10U))
    {
        return;
    }
    month = (uint8_t)(((uint8_t)(iso_date[5] - '0') * 10U) +
                      (uint8_t)(iso_date[6] - '0'));
    if((month == 0U) || (month > 12U))
    {
        return;
    }
    memcpy(destination, months[month - 1U], 3U);
    destination[3] = ' ';
    destination[4] = (uint8_t)iso_date[8];
    destination[5] = (uint8_t)iso_date[9];
    destination[6] = ' ';
    memcpy(&destination[7], iso_date, 4U);
}

void ecg_protocol_parser_init(ecg_protocol_parser_t *parser)
{
    if(parser != NULL)
    {
        memset(parser, 0, sizeof(*parser));
    }
}

bool ecg_protocol_parse_byte(ecg_protocol_parser_t *parser,
                             uint8_t byte,
                             ecg_protocol_command_t *command)
{
    uint16_t received_crc; /**< CRC value stored in the completed request. */
    uint16_t calculated_crc; /**< CRC calculated over the request body. */

    if((parser == NULL) || (command == NULL))
    {
        return false;
    }
    if((parser->received == 0U) && (byte != ECG_PACKET_HEAD_LOW))
    {
        return false;
    }
    if((parser->received == 1U) && (byte != ECG_PACKET_HEAD_HIGH))
    {
        parser->received = (byte == ECG_PACKET_HEAD_LOW) ? 1U : 0U;
        parser->packet[0] = byte;
        return false;
    }

    parser->packet[parser->received++] = byte;
    if(parser->received == 4U)
    {
        parser->expected = ecg_get_le16(&parser->packet[2]);
        if((parser->expected != ECG_PROTOCOL_COMMAND_LENGTH) ||
           (parser->expected > ECG_PROTOCOL_MAX_PACKET_LENGTH))
        {
            ecg_protocol_parser_init(parser);
            return false;
        }
    }
    if((parser->expected == 0U) || (parser->received < parser->expected))
    {
        return false;
    }

    received_crc = ecg_get_le16(&parser->packet[parser->expected - 2U]);
    calculated_crc = ecg_crc16(parser->packet, parser->expected - 2U);
    if((parser->packet[4] != ECG_COMMAND_MESSAGE_HEAD) ||
       (received_crc != calculated_crc))
    {
        ecg_protocol_parser_init(parser);
        return false;
    }

    command->message_index = parser->packet[5];
    command->type = parser->packet[6];
    command->value = parser->packet[7];
    ecg_protocol_parser_init(parser);
    return (command->type == ECG_PROTOCOL_HEARTBEAT) ||
           (command->type == ECG_PROTOCOL_CONTROL) ||
           (command->type == ECG_PROTOCOL_VERSION);
}

int ecg_protocol_send_ack(yi_device_t *uart,
                          uint8_t message_index,
                          uint8_t response_type)
{
    uint8_t packet[ECG_PACKET_OVERHEAD + ECG_ACK_DATA_LENGTH]; /**< ACK packet. */
    if((response_type != ECG_PROTOCOL_HEARTBEAT) &&
       (response_type != ECG_PROTOCOL_CONTROL))
    {
        return -1;
    }
    ecg_packet_begin(packet, sizeof(packet), message_index);
    packet[6] = response_type;
    memset(&packet[7], 0xFF, 3U);
    return ecg_packet_send(uart, packet, sizeof(packet));
}

int ecg_protocol_send_version(yi_device_t *uart, uint8_t message_index)
{
    uint8_t packet[ECG_PACKET_OVERHEAD + ECG_VERSION_DATA_LENGTH]; /**< Version packet. */
    const yi_build_info_t *build_info = yi_build_info_get(); /**< Valid application build metadata. */

    if(build_info == NULL)
    {
        return -1;
    }
    ecg_packet_begin(packet, sizeof(packet), message_index);
    packet[6] = ECG_PROTOCOL_VERSION;
    ecg_copy_text(&packet[7], 8U, ECG_BOOTLOADER_VERSION);
    ecg_copy_text(&packet[15], 8U, build_info->version);
    ecg_format_build_date(&packet[23], build_info->build_date);
    ecg_copy_text(&packet[34], 8U, build_info->build_time);
    return ecg_packet_send(uart, packet, sizeof(packet));
}

int ecg_protocol_send_data(yi_device_t *uart,
                           const yi_ads1298_frame_t *sample)
{
    static uint8_t message_index; /**< Sequence number for unsolicited ECG frames. */
    uint8_t packet[ECG_PACKET_OVERHEAD + ECG_DATA_DATA_LENGTH]; /**< ECG response packet. */

    if((uart == NULL) || (sample == NULL)) { return -1; }
    ecg_packet_begin(packet, sizeof(packet), message_index++);
    packet[6] = ECG_PROTOCOL_DATA;
    ecg_put_le16(&packet[7],
                 (uint16_t)(int16_t)(sample->channel[0] >> 8));
    ecg_put_le16(&packet[9],
                 (uint16_t)(int16_t)(sample->channel[1] >> 8));
    ecg_put_le16(&packet[11],
                 (uint16_t)(int16_t)(sample->channel[2] >> 8));
    packet[13] = ecg_lead_status(sample->status);
    return ecg_packet_send(uart, packet, sizeof(packet));
}
