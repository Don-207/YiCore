/**
 * @file yidap_ch32h417_peripherals.c
 * @brief Implement timeout-bounded CH32H417 I2C2 and SPI3 masters.
 * @author Don
 * @date 2026-08-01
 * @version 1.0.0
 */
#include "yidap_ch32h417_peripherals.h"
#include "ch32h417.h"

/** Maximum status polls before declaring a stalled peripheral. */
#define BUS_TIMEOUT_POLLS 1200000UL
/** Requested I2C frequency retained for host reporting. */
static uint32_t g_i2c_speed;
/** Requested SPI frequency retained for host reporting. */
static uint32_t g_spi_speed;
/** Active standard SPI mode. */
static uint8_t g_spi_mode;
/** Active SPI bit order selector. */
static uint8_t g_spi_lsb;

/** Wait for an I2C event without allowing a disconnected bus to hang USB. */
static int wait_i2c(uint32_t event)
{
    uint32_t polls = BUS_TIMEOUT_POLLS;
    while ((I2C_CheckEvent(I2C2, event) == NoREADY) && (polls-- != 0U)) { }
    return polls != 0U;
}

/** Wait for an SPI flag without allowing a stalled FPGA to hang USB. */
static int wait_spi(uint16_t flag)
{
    uint32_t polls = BUS_TIMEOUT_POLLS;
    while ((SPI_I2S_GetFlagStatus(SPI3, flag) == RESET) && (polls-- != 0U)) { }
    return polls != 0U;
}

/** Initialize both externally visible controllers. */
void yidap_peripherals_init(void)
{
    GPIO_InitTypeDef gpio = {0};
    RCC_HB2PeriphClockCmd(RCC_HB2Periph_AFIO | RCC_HB2Periph_GPIOA |
                         RCC_HB2Periph_GPIOB | RCC_HB2Periph_GPIOC, ENABLE);
    RCC_HB1PeriphClockCmd(RCC_HB1Periph_I2C2 | RCC_HB1Periph_SPI3, ENABLE);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource0, GPIO_AF9);
    GPIO_PinAFConfig(GPIOC, GPIO_PinSource1, GPIO_AF9);
    gpio.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1; gpio.GPIO_Speed = GPIO_Speed_Very_High;
    gpio.GPIO_Mode = GPIO_Mode_AF_OD; GPIO_Init(GPIOC, &gpio);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource14, GPIO_AF1);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource13, GPIO_AF6);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource15, GPIO_AF6);
    gpio.GPIO_Pin = GPIO_Pin_13 | GPIO_Pin_14; gpio.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_15; gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOA, &gpio);
    gpio.GPIO_Pin = GPIO_Pin_12; gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &gpio); GPIO_SetBits(GPIOB, GPIO_Pin_12);
    (void)yidap_i2c_configure(100000U);
    (void)yidap_spi_configure(1000000U, 0U, 0U);
}

/** Reinitialize I2C2 at a supported standard bus rate. */
yidap_bus_status_t yidap_i2c_configure(uint32_t speed_hz)
{
    I2C_InitTypeDef init = {0};
    if ((speed_hz != 100000U) && (speed_hz != 400000U) && (speed_hz != 1000000U)) return YIDAP_BUS_INVALID;
    I2C_Cmd(I2C2, DISABLE);
    init.I2C_ClockSpeed = speed_hz; init.I2C_Mode = I2C_Mode_I2C;
    init.I2C_DutyCycle = I2C_DutyCycle_2; init.I2C_OwnAddress1 = 0U;
    init.I2C_Ack = I2C_Ack_Enable; init.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C2, &init); I2C_Cmd(I2C2, ENABLE); g_i2c_speed = speed_hz;
    return YIDAP_BUS_OK;
}

/** Return the configured I2C rate. */
uint32_t yidap_i2c_speed(void) { return g_i2c_speed; }

/** Execute one master transaction with optional repeated START. */
yidap_bus_status_t yidap_i2c_transfer(uint8_t address, const uint8_t *tx, size_t tx_size, uint8_t *rx, size_t rx_size)
{
    size_t i;
    uint32_t polls = BUS_TIMEOUT_POLLS;
    if ((address < 0x08U) || (address > 0x77U) || (tx_size > 48U) || (rx_size > 48U) || ((tx_size == 0U) && (rx_size == 0U)) ||
        ((tx_size != 0U) && (tx == 0)) || ((rx_size != 0U) && (rx == 0))) return YIDAP_BUS_INVALID;
    while ((I2C_GetFlagStatus(I2C2, I2C_FLAG_BUSY) != RESET) && (polls-- != 0U)) { }
    if (polls == 0U) return YIDAP_BUS_TIMEOUT;
    if (tx_size != 0U) {
        I2C_GenerateSTART(I2C2, ENABLE); if (!wait_i2c(I2C_EVENT_MASTER_MODE_SELECT)) goto timeout;
        I2C_Send7bitAddress(I2C2, (uint8_t)(address << 1U), I2C_Direction_Transmitter);
        if (!wait_i2c(I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)) goto nack;
        for (i = 0U; i < tx_size; ++i) { I2C_SendData(I2C2, tx[i]); if (!wait_i2c(I2C_EVENT_MASTER_BYTE_TRANSMITTED)) goto timeout; }
    }
    if (rx_size != 0U) {
        I2C_GenerateSTART(I2C2, ENABLE); if (!wait_i2c(I2C_EVENT_MASTER_MODE_SELECT)) goto timeout;
        I2C_Send7bitAddress(I2C2, (uint8_t)(address << 1U), I2C_Direction_Receiver);
        if (!wait_i2c(I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED)) goto nack;
        for (i = 0U; i < rx_size; ++i) { I2C_AcknowledgeConfig(I2C2, (i + 1U < rx_size) ? ENABLE : DISABLE); if (!wait_i2c(I2C_EVENT_MASTER_BYTE_RECEIVED)) goto timeout; rx[i] = I2C_ReceiveData(I2C2); }
    }
    I2C_GenerateSTOP(I2C2, ENABLE); I2C_AcknowledgeConfig(I2C2, ENABLE); return YIDAP_BUS_OK;
nack: I2C_GenerateSTOP(I2C2, ENABLE); return YIDAP_BUS_NACK;
timeout: I2C_GenerateSTOP(I2C2, ENABLE); return YIDAP_BUS_TIMEOUT;
}

/** Select the nearest hardware divisor not exceeding the requested SPI rate. */
yidap_bus_status_t yidap_spi_configure(uint32_t speed_hz, uint8_t mode, uint8_t lsb_first)
{
    SPI_InitTypeDef init = {0}; uint16_t divisor;
    if ((speed_hz < 100000U) || (speed_hz > 20000000U) || (mode > 3U) || (lsb_first > 1U)) return YIDAP_BUS_INVALID;
    if (speed_hz >= 15000000U) divisor = SPI_BaudRatePrescaler_Mode2;
    else if (speed_hz >= 7500000U) divisor = SPI_BaudRatePrescaler_Mode3;
    else if (speed_hz >= 3750000U) divisor = SPI_BaudRatePrescaler_Mode4;
    else if (speed_hz >= 1875000U) divisor = SPI_BaudRatePrescaler_Mode5;
    else if (speed_hz >= 937500U) divisor = SPI_BaudRatePrescaler_Mode6;
    else if (speed_hz >= 468750U) divisor = SPI_BaudRatePrescaler_Mode7;
    else divisor = SPI_BaudRatePrescaler_Mode7;
    SPI_Cmd(SPI3, DISABLE); init.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
    init.SPI_Mode = SPI_Mode_Master; init.SPI_DataSize = SPI_DataSize_8b;
    init.SPI_CPOL = (mode >= 2U) ? SPI_CPOL_High : SPI_CPOL_Low;
    init.SPI_CPHA = (mode & 1U) ? SPI_CPHA_2Edge : SPI_CPHA_1Edge;
    init.SPI_NSS = SPI_NSS_Soft; init.SPI_BaudRatePrescaler = divisor;
    init.SPI_FirstBit = lsb_first ? SPI_FirstBit_LSB : SPI_FirstBit_MSB; init.SPI_CRCPolynomial = 7U;
    SPI_Init(SPI3, &init); SPI_Cmd(SPI3, ENABLE);
    g_spi_speed = speed_hz; g_spi_mode = mode; g_spi_lsb = lsb_first; return YIDAP_BUS_OK;
}
uint32_t yidap_spi_speed(void) { return g_spi_speed; }
uint8_t yidap_spi_mode(void) { return g_spi_mode; }
uint8_t yidap_spi_lsb_first(void) { return g_spi_lsb; }

/** Exchange bytes with the selected SPI device. */
yidap_bus_status_t yidap_spi_transfer(const uint8_t *tx, uint8_t *rx, size_t size)
{
    size_t i; if ((tx == 0) || (rx == 0) || (size == 0U) || (size > 48U)) return YIDAP_BUS_INVALID;
    GPIO_ResetBits(GPIOB, GPIO_Pin_12);
    for (i = 0U; i < size; ++i) { if (!wait_spi(SPI_I2S_FLAG_TXE)) goto timeout; SPI_I2S_SendData(SPI3, tx[i]); if (!wait_spi(SPI_I2S_FLAG_RXNE)) goto timeout; rx[i] = (uint8_t)SPI_I2S_ReceiveData(SPI3); }
    { uint32_t polls = BUS_TIMEOUT_POLLS; while ((SPI_I2S_GetFlagStatus(SPI3, SPI_I2S_FLAG_BSY) != RESET) && (polls-- != 0U)) { } if (polls == 0U) goto timeout; }
    GPIO_SetBits(GPIOB, GPIO_Pin_12); return YIDAP_BUS_OK;
timeout: GPIO_SetBits(GPIOB, GPIO_Pin_12); return YIDAP_BUS_TIMEOUT;
}
