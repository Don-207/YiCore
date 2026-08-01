/** @file yidap_ch32h417_vendor.c @brief Expose CH32H417 I2C, SPI, and FPGA access as CMSIS-DAP vendor commands. @author Don @date 2026-08-01 @version 1.0.0 */
#include "yidap_ch32h417_fpga.h"
#include "yidap_ch32h417_peripherals.h"
#include "yidap_ch32h417_vendor.h"
#define YIDAP_VENDOR_MAX_DATA 48U
#define YIDAP_VENDOR_I2C_INFO 0x80U
#define YIDAP_VENDOR_I2C_CONFIGURE 0x81U
#define YIDAP_VENDOR_I2C_TRANSFER 0x82U
#define YIDAP_VENDOR_SPI_INFO 0x83U
#define YIDAP_VENDOR_SPI_CONFIGURE 0x84U
#define YIDAP_VENDOR_SPI_TRANSFER 0x85U
#define YIDAP_VENDOR_FPGA_INFO 0x86U
#define YIDAP_VENDOR_FPGA_EXCHANGE 0x87U
/** Decode a little-endian word from the request. */
static uint32_t read_u32(const uint8_t *p) { return (uint32_t)p[0] | ((uint32_t)p[1] << 8U) | ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U); }
/** Encode a little-endian word into the response. */
static void write_u32(uint8_t *p, uint32_t v) { p[0]=(uint8_t)v; p[1]=(uint8_t)(v>>8U); p[2]=(uint8_t)(v>>16U); p[3]=(uint8_t)(v>>24U); }
/** Process the YiDAP peripheral and FPGA vendor command family. */
uint16_t yidap_vendor_process(const uint8_t *request, uint8_t *response)
{
    uint8_t cmd=request[0], count; uint16_t consumed=1U, produced=2U; yidap_bus_status_t status=YIDAP_BUS_INVALID;
    response[0]=cmd; response[1]=(uint8_t)status;
    if (cmd == YIDAP_VENDOR_I2C_INFO) { response[1]=0U; response[2]=1U; response[3]=0U; response[4]=YIDAP_VENDOR_MAX_DATA; response[5]=YIDAP_VENDOR_MAX_DATA; write_u32(&response[6],yidap_i2c_speed()); produced=10U; }
    else if (cmd == YIDAP_VENDOR_I2C_CONFIGURE) { consumed=5U; response[1]=(uint8_t)yidap_i2c_configure(read_u32(&request[1])); write_u32(&response[2],yidap_i2c_speed()); produced=6U; }
    else if (cmd == YIDAP_VENDOR_I2C_TRANSFER) { count=request[2]; if (count <= YIDAP_VENDOR_MAX_DATA && request[3] <= YIDAP_VENDOR_MAX_DATA) { consumed=(uint16_t)(4U+count); status=yidap_i2c_transfer(request[1],&request[4],count,&response[3],request[3]); } response[1]=(uint8_t)status; response[2]=(status==YIDAP_BUS_OK)?request[3]:0U; produced=(uint16_t)(3U+response[2]); }
    else if (cmd == YIDAP_VENDOR_SPI_INFO) { response[1]=0U; response[2]=1U; response[3]=0U; response[4]=YIDAP_VENDOR_MAX_DATA; response[5]=yidap_spi_mode(); response[6]=yidap_spi_lsb_first(); write_u32(&response[7],yidap_spi_speed()); produced=11U; }
    else if (cmd == YIDAP_VENDOR_SPI_CONFIGURE) { consumed=7U; response[1]=(uint8_t)yidap_spi_configure(read_u32(&request[1]),request[5],request[6]); write_u32(&response[2],yidap_spi_speed()); response[6]=yidap_spi_mode(); response[7]=yidap_spi_lsb_first(); produced=8U; }
    else if (cmd == YIDAP_VENDOR_SPI_TRANSFER) { count=request[1]; if (count <= YIDAP_VENDOR_MAX_DATA) { consumed=(uint16_t)(2U+count); status=yidap_spi_transfer(&request[2],&response[3],count); } response[1]=(uint8_t)status; response[2]=(status==YIDAP_BUS_OK)?count:0U; produced=(uint16_t)(3U+response[2]); }
    else if (cmd == YIDAP_VENDOR_FPGA_INFO) { response[1]=0U; response[2]=1U; response[3]=0U; response[4]=YIDAP_VENDOR_MAX_DATA; write_u32(&response[5],12000000U); produced=9U; }
    else if (cmd == YIDAP_VENDOR_FPGA_EXCHANGE) { count=request[1]; if (count <= YIDAP_VENDOR_MAX_DATA) { consumed=(uint16_t)(2U+count); status=yidap_fpga_exchange(&request[2],&response[3],count); } response[1]=(uint8_t)status; response[2]=(status==YIDAP_BUS_OK)?count:0U; produced=(uint16_t)(3U+response[2]); }
    (void)consumed;
    return produced;
}
