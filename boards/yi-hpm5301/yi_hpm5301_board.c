/**
 * @file yi_hpm5301_board.c
 * @brief Implement Yi HPM5301 peripheral initialization.
 * @author Don
 * @date 2026-07-29
 * @version 1.0.0
 */

#include "board.h"

/**
 * @brief Configure and clock the UART2 pins connected to the debug header.
 * @return Effective UART2 source clock frequency in hertz.
 * @note Changes PB08/PB09 pin multiplexing and enables UART2 in clock group 0.
 * @note Call once during startup from thread context, before UART/DMA operation.
 */
uint32_t yi_hpm5301_board_init_bridge_uart(void)
{
    HPM_IOC->PAD[IOC_PAD_PB08].FUNC_CTL = IOC_PB08_FUNC_CTL_UART2_TXD;
    HPM_IOC->PAD[IOC_PAD_PB09].FUNC_CTL = IOC_PB09_FUNC_CTL_UART2_RXD;
    clock_set_source_divider(clock_uart2, clk_src_pll1_clk0, 8U);
    clock_add_to_group(clock_uart2, 0U);
    return clock_get_frequency(clock_uart2);
}

/**
 * @brief Configure the pins and clock used by the external I2C master port.
 * @return Effective I2C0 source clock frequency in hertz.
 * @note Changes PA02/PA03 pin multiplexing and enables I2C0 in clock group 0.
 * @note Call once during startup from thread context.
 */
uint32_t yi_hpm5301_board_init_external_i2c(void)
{
    /** Common electrical configuration for open-drain I2C pins. */
    uint32_t pad_config = IOC_PAD_PAD_CTL_OD_SET(1)
                        | IOC_PAD_PAD_CTL_PE_SET(1)
                        | IOC_PAD_PAD_CTL_PS_SET(1);

    HPM_IOC->PAD[IOC_PAD_PA02].FUNC_CTL =
        IOC_PA02_FUNC_CTL_I2C0_SCL | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
    HPM_IOC->PAD[IOC_PAD_PA02].PAD_CTL = pad_config;
    HPM_IOC->PAD[IOC_PAD_PA03].FUNC_CTL =
        IOC_PA03_FUNC_CTL_I2C0_SDA | IOC_PAD_FUNC_CTL_LOOP_BACK_MASK;
    HPM_IOC->PAD[IOC_PAD_PA03].PAD_CTL = pad_config;
    clock_add_to_group(clock_i2c0, 0U);
    board_i2c_bus_clear(HPM_I2C0);
    return clock_get_frequency(clock_i2c0);
}

/**
 * @brief Configure the pins and clock used by the external SPI master port.
 * @return Effective SPI1 source clock frequency in hertz.
 * @note Changes PA26..PA29 pin multiplexing; thread context only during startup.
 */
uint32_t yi_hpm5301_board_init_external_spi(void)
{
    /** SPI1 source clock frequency after the board clock configuration. */
    uint32_t source_clock_hz = board_init_spi_clock(BOARD_APP_SPI_BASE);

    board_init_spi_pins(BOARD_APP_SPI_BASE);
    return source_clock_hz;
}
