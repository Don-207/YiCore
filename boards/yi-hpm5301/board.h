/**
 * @file board.h
 * @brief Adapt the HPM5301 EVKLite board baseline to Yi hardware.
 * @author Don
 * @date 2026-07-29
 * @version 1.0.0
 */

#ifndef YI_HPM5301_BOARD_H
#define YI_HPM5301_BOARD_H

/*
 * Define the application UART before importing the common HPM5301 baseline.
 * The upstream header deliberately permits this board-level override.
 */
#define BOARD_APP_UART_BASE       HPM_UART2
#define BOARD_APP_UART_IRQ        IRQn_UART2
#define BOARD_APP_UART_BAUDRATE   (115200UL)
#define BOARD_APP_UART_CLK_NAME   clock_uart2
#define BOARD_APP_UART_RX_DMA_REQ HPM_DMA_SRC_UART2_RX
#define BOARD_APP_UART_TX_DMA_REQ HPM_DMA_SRC_UART2_TX

#include <hpm5301evklite/board.h>

#undef BOARD_APP_I2C_BASE
#undef BOARD_APP_I2C_CLK_NAME
/** External expansion bus controller. */
#define BOARD_APP_I2C_BASE HPM_I2C0
/** External expansion bus source clock identifier. */
#define BOARD_APP_I2C_CLK_NAME clock_i2c0

#undef BOARD_NAME
#define BOARD_NAME "yi-hpm5301"

#undef BOARD_LED_GPIO_NAME
#undef BOARD_LED_GPIO_CTRL
#undef BOARD_LED_GPIO_INDEX
#undef BOARD_LED_GPIO_PIN
#undef BOARD_LED_OFF_LEVEL
#undef BOARD_LED_ON_LEVEL

/** Human-readable identifier for the green connection LED. */
#define BOARD_LED_GPIO_NAME "PB12"
/** GPIO controller that owns both YiLink status LEDs. */
#define BOARD_LED_GPIO_CTRL HPM_GPIO0
/** GPIO port index for the default connection LED. */
#define BOARD_LED_GPIO_INDEX GPIO_DI_GPIOB
/** GPIO pin number for the default connection LED. */
#define BOARD_LED_GPIO_PIN 12
/** Electrical level that turns the active-low connection LED off. */
#define BOARD_LED_OFF_LEVEL 1
/** Electrical level that turns the active-low connection LED on. */
#define BOARD_LED_ON_LEVEL 0

/**
 * @brief Configure and clock UART2 for the USB CDC bridge.
 * @return UART2 source clock frequency in hertz.
 * @note PB08 is UART2_TXD and PB09 is UART2_RXD per the HPM5301 IOMUX.
 * @note Must run before UART2 or its DMA requests are enabled; thread context only.
 */
uint32_t yi_hpm5301_board_init_bridge_uart(void);

/**
 * @brief Configure the external I2C0 bus on PA02/PA03.
 * @return I2C0 source clock frequency in hertz.
 * @note PA02 is SCL and PA03 is SDA; both pins are open-drain with weak pull-ups.
 * @note External pull-ups sized for the bus capacitance are still required.
 */
uint32_t yi_hpm5301_board_init_external_i2c(void);

/**
 * @brief Configure the external SPI1 bus on PA26 through PA29.
 * @return SPI1 source clock frequency in hertz.
 * @note PA26 is active-low CS0, PA27 SCLK, PA28 MISO, and PA29 MOSI.
 */
uint32_t yi_hpm5301_board_init_external_spi(void);

/**
 * @brief Recover a bus held by a slave by generating up to nine SCL pulses.
 * @param ptr I2C controller whose pins are connected to the affected bus.
 * @return None.
 * @note Provided by the reused HPM5301 board support implementation.
 */
void board_i2c_bus_clear(I2C_Type *ptr);

#endif
