/** @file yidap_ch32h417_fpga.h @brief Define the CH32H417 raw SPI bridge to an YiLink FPGA endpoint. @author Don @date 2026-08-01 @version 1.0.0 */
#ifndef YIDAP_CH32H417_FPGA_H
#define YIDAP_CH32H417_FPGA_H
#include <stddef.h>
#include <stdint.h>
#include "yidap_ch32h417_peripherals.h"
/** Exchange one bounded full-duplex FPGA transaction on SPI3. */
yidap_bus_status_t yidap_fpga_exchange(const uint8_t *tx, uint8_t *rx, size_t size);
#endif
