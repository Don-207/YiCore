/**
 * @file ecg_service.c
 * @brief ECG acquisition, command parsing, and controlled upload service.
 * @author Don
 * @date 2026-07-27
 * @version 1.1.0
 */

#include "ecg_service.h"

#include <stdbool.h>
#include <string.h>

#include "ecg_protocol.h"
#include "yi_ads1298.h"
#include "yi_device.h"
#include "yi_uart_dma_lwrb.h"

#define ECG_DECIMATION 5U
#define ECG_RX_DMA_BUFFER_SIZE 64U
#define ECG_RX_RING_BUFFER_SIZE 128U
#define ECG_RX_PROCESS_CHUNK_SIZE 32U

static yi_device_t *ecg_ads1298; /**< ADS1298 device used for acquisition. */
static yi_device_t *ecg_uart; /**< USART device shared by commands and responses. */
static uint8_t ecg_decimation; /**< Number of samples accumulated toward 100 Hz upload. */
static bool ecg_upload_enabled; /**< Host-controlled unsolicited upload state. */
static ecg_service_stats_t ecg_stats; /**< Persistent service diagnostic counters. */
static yi_uart_dma_lwrb_t ecg_rx; /**< DMA-to-ring UART receive context. */
static uint8_t ecg_rx_dma_buffer[ECG_RX_DMA_BUFFER_SIZE]; /**< Circular DMA storage. */
static uint8_t ecg_rx_ring_buffer[ECG_RX_RING_BUFFER_SIZE]; /**< Parsed-command staging ring. */
static ecg_protocol_parser_t ecg_parser; /**< Streaming host packet parser state. */

/**
 * @brief Execute one validated host command and transmit its response.
 * @param command Decoded command received from the host.
 */
static void ecg_service_handle_command(const ecg_protocol_command_t *command)
{
    int result = -1; /**< UART response result for statistics. */

    if(command == NULL)
    {
        return;
    }
    ecg_stats.received_commands++;
    switch(command->type)
    {
        case ECG_PROTOCOL_HEARTBEAT:
            result = ecg_protocol_send_ack(ecg_uart,
                                           command->message_index,
                                           ECG_PROTOCOL_HEARTBEAT);
            break;

        case ECG_PROTOCOL_CONTROL:
            if(command->value > 1U)
            {
                ecg_stats.protocol_errors++;
                return;
            }
            ecg_upload_enabled = command->value == 1U;
            ecg_decimation = 0U;
            result = ecg_protocol_send_ack(ecg_uart,
                                           command->message_index,
                                           ECG_PROTOCOL_CONTROL);
            break;

        case ECG_PROTOCOL_VERSION:
            result = ecg_protocol_send_version(ecg_uart,
                                               command->message_index);
            break;

        default:
            ecg_stats.protocol_errors++;
            return;
    }
    if(result != 0)
    {
        ecg_stats.uart_errors++;
    }
}

/**
 * @brief Drain UART RX bytes and dispatch all complete commands.
 */
static void ecg_service_process_commands(void)
{
    uint8_t bytes[ECG_RX_PROCESS_CHUNK_SIZE]; /**< Current ring-buffer read chunk. */
    uint32_t count; /**< Number of valid bytes in the current chunk. */

    if(yi_uart_dma_lwrb_overrun(&ecg_rx, true) != 0U)
    {
        ecg_stats.protocol_errors++;
        ecg_protocol_parser_init(&ecg_parser);
    }
    do
    {
        count = yi_uart_dma_lwrb_read(&ecg_rx, bytes, sizeof(bytes));
        for(uint32_t index = 0U; index < count; index++) /**< Feed each received byte. */
        {
            ecg_protocol_command_t command; /**< Command produced by the streaming parser. */
            if(ecg_protocol_parse_byte(&ecg_parser, bytes[index], &command))
            {
                ecg_service_handle_command(&command);
            }
        }
    } while(count == sizeof(bytes));
}

int ecg_service_init(void)
{
    ecg_ads1298 = yi_device_get("ads1298");
    ecg_uart = yi_device_get("usart1");
    ecg_decimation = 0U;
    ecg_upload_enabled = false;
    memset(&ecg_stats, 0, sizeof(ecg_stats));
    ecg_protocol_parser_init(&ecg_parser);

    if(!yi_device_is_ready(ecg_ads1298) ||
       !yi_device_is_ready(ecg_uart) ||
       (yi_uart_dma_lwrb_start(&ecg_rx,
                               ecg_uart,
                               ecg_rx_dma_buffer,
                               sizeof(ecg_rx_dma_buffer),
                               ecg_rx_ring_buffer,
                               sizeof(ecg_rx_ring_buffer)) != 0) ||
       (yi_ads1298_start(ecg_ads1298) != 0) ||
       (yi_ads1298_set_continuous(ecg_ads1298, true) != 0))
    {
        return -1;
    }
    return 0;
}

void ecg_service_process(void)
{
    yi_ads1298_frame_t sample; /**< Most recently acquired raw ADS1298 frame. */
    bool ready; /**< Whether DRDY reported a pending sample. */

    ecg_service_process_commands();

    if(yi_ads1298_data_ready(ecg_ads1298, &ready) != 0)
    {
        ecg_stats.read_errors++;
        return;
    }
    if(!ready) { return; }
    if(yi_ads1298_read_frame(ecg_ads1298, &sample) != 0)
    {
        ecg_stats.read_errors++;
        return;
    }
    ecg_stats.acquired_frames++;

    if(!ecg_upload_enabled)
    {
        ecg_decimation = 0U;
        return;
    }
    if(++ecg_decimation < ECG_DECIMATION) { return; }
    ecg_decimation = 0U;
    if(ecg_protocol_send_data(ecg_uart, &sample) != 0)
    {
        ecg_stats.uart_errors++;
        return;
    }
    ecg_stats.uploaded_frames++;
}

bool ecg_service_upload_enabled(void)
{
    return ecg_upload_enabled;
}

const ecg_service_stats_t *ecg_service_stats(void)
{
    return &ecg_stats;
}
