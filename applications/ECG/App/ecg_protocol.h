/**
 * @file ecg_protocol.h
 * @brief ECG serial protocol parser and response interface.
 * @author Don
 * @date 2026-07-27
 * @version 1.1.0
 */

#ifndef ECG_PROTOCOL_H
#define ECG_PROTOCOL_H

#include <stdbool.h>
#include <stdint.h>

#include "yi_ads1298.h"
#include "yi_device.h"

#define ECG_PROTOCOL_COMMAND_LENGTH 12U
#define ECG_PROTOCOL_MAX_PACKET_LENGTH 50U

typedef enum
{
    ECG_PROTOCOL_HEARTBEAT = 0x00U, /**< Heartbeat request/response type. */
    ECG_PROTOCOL_CONTROL = 0x01U, /**< ECG upload control request/response type. */
    ECG_PROTOCOL_DATA = 0x02U, /**< Unsolicited ECG sample response type. */
    ECG_PROTOCOL_VERSION = 0xFFU /**< Firmware version request/response type. */
} ecg_protocol_type_t;

typedef struct
{
    uint8_t type; /**< Decoded command type. */
    uint8_t value; /**< Control value at data offset 1; zero for other commands. */
    uint8_t message_index; /**< Request index echoed by the response. */
} ecg_protocol_command_t;

typedef struct
{
    uint8_t packet[ECG_PROTOCOL_MAX_PACKET_LENGTH]; /**< In-progress packet bytes. */
    uint16_t received; /**< Number of valid bytes accumulated. */
    uint16_t expected; /**< Total packet length decoded from the header. */
} ecg_protocol_parser_t;

/**
 * @brief Reset a streaming ECG protocol parser.
 * @param parser Parser state to initialize.
 */
void ecg_protocol_parser_init(ecg_protocol_parser_t *parser);

/**
 * @brief Consume one serial byte and return a validated host command.
 * @param parser Parser state.
 * @param byte Next received byte.
 * @param command Decoded command when the function returns true.
 * @return true when a complete valid command was decoded.
 */
bool ecg_protocol_parse_byte(ecg_protocol_parser_t *parser,
                             uint8_t byte,
                             ecg_protocol_command_t *command);

/**
 * @brief Send a heartbeat or ECG-control acknowledgement.
 * @param uart UART device used for the response.
 * @param message_index Request index to echo.
 * @param response_type ECG_PROTOCOL_HEARTBEAT or ECG_PROTOCOL_CONTROL.
 * @return 0 on success, otherwise -1.
 */
int ecg_protocol_send_ack(yi_device_t *uart,
                          uint8_t message_index,
                          uint8_t response_type);

/**
 * @brief Send the fixed-width application and bootloader version response.
 * @param uart UART device used for the response.
 * @param message_index Request index to echo.
 * @return 0 on success, otherwise -1.
 */
int ecg_protocol_send_version(yi_device_t *uart, uint8_t message_index);

/**
 * @brief Send one unsolicited ECG data response.
 * @param uart UART device used for the response.
 * @param sample Raw ADS1298 sample.
 * @return 0 on success, otherwise -1.
 */
int ecg_protocol_send_data(yi_device_t *uart,
                           const yi_ads1298_frame_t *sample);

#endif
