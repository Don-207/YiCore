/**
 * @file yi_dap_protocol.h
 * @brief Define the independent YiCore CMSIS-DAP v2 command engine.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */
#ifndef YI_DAP_PROTOCOL_H
#define YI_DAP_PROTOCOL_H
#include <stdint.h>
/** Initialize protocol state and target-facing pins. */
void yi_dap_protocol_init(void);
/** Process one 64-byte request and return the response byte count. */
uint16_t yi_dap_protocol_process(const uint8_t *request, uint8_t *response);
#endif
