/**
 * @file ecg_service.h
 * @brief ECG acquisition, command processing, and upload service interface.
 * @author Don
 * @date 2026-07-27
 * @version 1.1.0
 */

#ifndef ECG_SERVICE_H
#define ECG_SERVICE_H

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint32_t acquired_frames; /**< ADS1298 frames read successfully. */
    uint32_t uploaded_frames; /**< ECG data responses sent successfully. */
    uint32_t received_commands; /**< Valid host commands received. */
    uint32_t protocol_errors; /**< Unsupported command values or RX overruns. */
    uint32_t read_errors; /**< ADS1298 read failures. */
    uint32_t uart_errors; /**< UART response failures. */
} ecg_service_stats_t;

/**
 * @brief Initialize ECG acquisition and host-command reception.
 * @return 0 on success, otherwise -1.
 */
int ecg_service_init(void);

/**
 * @brief Process pending host commands and one available ECG sample.
 */
void ecg_service_process(void);

/**
 * @brief Return whether unsolicited ECG upload is enabled.
 * @return true after a valid control-start command; false after reset or stop.
 */
bool ecg_service_upload_enabled(void);

/**
 * @brief Return read-only ECG service counters.
 * @return Pointer to persistent service statistics.
 */
const ecg_service_stats_t *ecg_service_stats(void);

#endif
