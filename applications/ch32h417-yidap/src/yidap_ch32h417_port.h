/**
 * @file yidap_ch32h417_port.h
 * @brief Bind the independent YiDAP signal engine to CH32H417 GPIO pins.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */
#ifndef YIDAP_CH32H417_PORT_H
#define YIDAP_CH32H417_PORT_H
#include "ch32h417.h"
/** V3F clock used to derive software debug clock delays. */
#define YIDAP_CPU_CLOCK_HZ 120000000UL
/** Target debug GPIO bank. */
#define YIDAP_PORT GPIOA
/** JTAG TDO input. */
#define YIDAP_TDO GPIO_Pin_4
/** JTAG TDI output. */
#define YIDAP_TDI GPIO_Pin_5
/** SWCLK/JTAG TCK output. */
#define YIDAP_TCK GPIO_Pin_6
/** SWDIO/JTAG TMS bidirectional signal. */
#define YIDAP_TMS GPIO_Pin_7
/** Active-low target reset output. */
#define YIDAP_RESET GPIO_Pin_8
/** Configure all target pins for JTAG. */
void yidap_port_jtag_setup(void);
/** Configure target pins for two-wire SWD. */
void yidap_port_swd_setup(void);
/** Return all target pins to high-impedance inputs. */
void yidap_port_off(void);
/** Initialize the reset pin released and keep the debug connector disconnected. */
void yidap_port_init(void);
/** Drive debug clock low. */
void yidap_clock_low(void);
/** Drive debug clock high. */
void yidap_clock_high(void);
/** Sample SWDIO/TMS. */
uint32_t yidap_data_read(void);
/** Drive SWDIO/TMS. */
void yidap_data_write(uint32_t bit);
/** Sample JTAG TDO. */
uint32_t yidap_tdo_read(void);
/** Drive JTAG TDI. */
void yidap_tdi_write(uint32_t bit);
/** Sample active-low target reset. */
uint32_t yidap_reset_read(void);
/** Drive active-low target reset. */
void yidap_reset_write(uint32_t bit);
/** Switch SWDIO to push-pull output mode. */
void yidap_data_output(void);
/** Switch SWDIO to floating input mode. */
void yidap_data_input(void);
#endif
