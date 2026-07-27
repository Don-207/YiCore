#include "ecg_protocol.h"

#include "yi_uart.h"

#define ECG_PACKET_LENGTH 15U

static uint16_t ecg_crc16(const uint8_t *data, uint32_t length)
{
    uint16_t crc = 0xFFFFU;
    while(length-- != 0U)
    {
        crc ^= *data++;
        for(uint8_t bit = 0U; bit < 8U; bit++)
        {
            crc = ((crc & 1U) != 0U) ?
                (uint16_t)((crc >> 1) ^ 0xA001U) : (uint16_t)(crc >> 1);
        }
    }
    return crc;
}

static void ecg_put_le16(uint8_t *destination, uint16_t value)
{
    destination[0] = (uint8_t)value;
    destination[1] = (uint8_t)(value >> 8);
}

static uint8_t ecg_lead_status(uint32_t status)
{
    const uint8_t loff_p = (uint8_t)(status >> 12);
    const uint8_t loff_n = (uint8_t)(status >> 4);
    uint8_t result = 0U;
    if((loff_n & 0x03U) != 0U) { result |= (1U << 0); }
    if((loff_p & 0x01U) != 0U) { result |= (1U << 1); }
    if((loff_p & 0x02U) != 0U) { result |= (1U << 2); }
    if((loff_p & 0x04U) != 0U) { result |= (1U << 3); }
    return result;
}

int ecg_protocol_send(yi_device_t *uart,
                      const yi_ads1298_frame_t *sample)
{
    static uint8_t message_index;
    uint8_t packet[ECG_PACKET_LENGTH];
    if((uart == NULL) || (sample == NULL)) { return -1; }

    packet[0] = 0xAAU;
    packet[1] = 0x55U;
    ecg_put_le16(&packet[2], ECG_PACKET_LENGTH);
    packet[4] = 0x84U;
    packet[5] = message_index++;
    ecg_put_le16(&packet[6],
                 (uint16_t)(int16_t)(sample->channel[0] >> 8));
    ecg_put_le16(&packet[8],
                 (uint16_t)(int16_t)(sample->channel[1] >> 8));
    ecg_put_le16(&packet[10],
                 (uint16_t)(int16_t)(sample->channel[2] >> 8));
    packet[12] = ecg_lead_status(sample->status);
    ecg_put_le16(&packet[13],
                 ecg_crc16(packet, ECG_PACKET_LENGTH - 2U));
    return yi_uart_write(uart, packet, sizeof(packet));
}
