/**
 * @file yidap_ch32h417_vendor.h
 * @brief Declare YiDAP vendor command processing for CH32H417 peripherals.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */
#ifndef YIDAP_CH32H417_VENDOR_H
#define YIDAP_CH32H417_VENDOR_H
#include <stdint.h>
/** Process one command in the 0x80 through 0x87 range. */
uint16_t yidap_vendor_process(const uint8_t *request,uint8_t *response);
#endif
