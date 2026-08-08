/**
 * @file yi_dap_protocol_port.h
 * @brief Define hardware and product hooks required by the YiDAP protocol core.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */
#ifndef YI_DAP_PROTOCOL_PORT_H
#define YI_DAP_PROTOCOL_PORT_H
#include <stdint.h>
/** Return the processor frequency used for debug-clock delay calculation. */
uint32_t yidap_port_cpu_clock_hz(void);
void yidap_port_init(void); /**< Initialize target pins in a safe state. */
void yidap_port_jtag_setup(void); /**< Select JTAG electrical modes. */
void yidap_port_swd_setup(void); /**< Select SWD electrical modes. */
void yidap_port_off(void); /**< Disconnect target-facing outputs. */
void yidap_clock_low(void); /**< Drive SWCLK/TCK low. */
void yidap_clock_high(void); /**< Drive SWCLK/TCK high. */
uint32_t yidap_data_read(void); /**< Sample SWDIO/TMS. */
void yidap_data_write(uint32_t bit); /**< Drive SWDIO/TMS. */
uint32_t yidap_tdo_read(void); /**< Sample JTAG TDO. */
void yidap_tdi_write(uint32_t bit); /**< Drive JTAG TDI. */
uint32_t yidap_reset_read(void); /**< Sample target reset. */
void yidap_reset_write(uint32_t bit); /**< Drive target reset. */
void yidap_data_output(void); /**< Switch SWDIO to output. */
void yidap_data_input(void); /**< Switch SWDIO to input. */
/** Dispatch a product vendor command and return response length. */
uint16_t yidap_vendor_process(const uint8_t *request,uint8_t *response);
#endif
