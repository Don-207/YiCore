/** @file yidap_ch32h417_fpga.c @brief Implement the CH32H417 to FPGA SPI exchange entry point. @author Don @date 2026-08-01 @version 1.0.0 */
#include "yidap_ch32h417_fpga.h"
/** Exchange an FPGA protocol fragment using mode 0, MSB-first SPI3. */
yidap_bus_status_t yidap_fpga_exchange(const uint8_t *tx, uint8_t *rx, size_t size)
{
    yidap_bus_status_t status = yidap_spi_configure(12000000U, 0U, 0U);
    return (status == YIDAP_BUS_OK) ? yidap_spi_transfer(tx, rx, size) : status;
}
