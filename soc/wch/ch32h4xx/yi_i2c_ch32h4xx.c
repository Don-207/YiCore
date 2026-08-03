/**
 * @file yi_i2c_ch32h4xx.c
 * @brief Implement polling master I2C for CH32H417 through YiCore.
 * @author Don
 * @date 2026-08-03
 * @version 1.0.0
 */
#include "yi_i2c_ch32h4xx.h"
#include "system_ch32h417.h"

#include <stddef.h>

/** Error flags checked while polling an I2C transaction. */
#define YI_I2C_ERROR_MASK (I2C_FLAG_AF | I2C_FLAG_TIMEOUT | I2C_FLAG_BERR | \
                           I2C_FLAG_ARLO | I2C_FLAG_OVR)

/** Return the HB2 clock mask associated with a GPIO port. */
static uint32_t yi_i2c_gpio_clock(const GPIO_TypeDef *port)
{
    if(port == GPIOA) { return RCC_HB2Periph_GPIOA; }
    if(port == GPIOB) { return RCC_HB2Periph_GPIOB; }
    if(port == GPIOC) { return RCC_HB2Periph_GPIOC; }
    if(port == GPIOD) { return RCC_HB2Periph_GPIOD; }
    if(port == GPIOE) { return RCC_HB2Periph_GPIOE; }
    if(port == GPIOF) { return RCC_HB2Periph_GPIOF; }
    return 0U;
}

/** Enable the bus clock belonging to one supported I2C instance. */
static int yi_i2c_clock_enable(I2C_TypeDef *instance)
{
    if(instance == I2C1) { RCC_HB1PeriphClockCmd(RCC_HB1Periph_I2C1, ENABLE); }
    else if(instance == I2C2) { RCC_HB1PeriphClockCmd(RCC_HB1Periph_I2C2, ENABLE); }
    else if(instance == I2C3) { RCC_HB1PeriphClockCmd(RCC_HB1Periph_I2C3, ENABLE); }
    else if(instance == I2C4) { RCC_HB2PeriphClockCmd(RCC_HB2Periph_I2C4, ENABLE); }
    else { return -1; }
    return 0;
}

/** Convert active hardware error flags into stable YiCore result values. */
static int yi_i2c_error(I2C_TypeDef *instance)
{
    uint32_t status = instance->STAR1;

    if((status & I2C_FLAG_AF) != 0U) {
        I2C_ClearFlag(instance, I2C_FLAG_AF);
        return YI_I2C_RESULT_NACK;
    }
    if((status & I2C_FLAG_TIMEOUT) != 0U) {
        I2C_ClearFlag(instance, I2C_FLAG_TIMEOUT);
        return YI_I2C_RESULT_TIMEOUT;
    }
    if((status & YI_I2C_ERROR_MASK) != 0U) {
        instance->STAR1 &= ~(I2C_FLAG_BERR | I2C_FLAG_ARLO | I2C_FLAG_OVR);
        return YI_I2C_RESULT_BUS_ERROR;
    }
    return YI_I2C_RESULT_OK;
}

/** Wait for one STAR1 flag while observing errors and a shared poll budget. */
static int yi_i2c_wait(I2C_TypeDef *instance, uint32_t flag,
                       uint32_t *budget)
{
    int result;

    while((instance->STAR1 & flag) == 0U) {
        result = yi_i2c_error(instance);
        if(result != YI_I2C_RESULT_OK) { return result; }
        if((*budget)-- == 0U) { return YI_I2C_RESULT_TIMEOUT; }
    }
    return YI_I2C_RESULT_OK;
}

/** Clear ADDR by performing the mandatory STAR1/STAR2 read sequence. */
static void yi_i2c_clear_address(I2C_TypeDef *instance)
{
    volatile uint32_t discard = instance->STAR1;
    discard = instance->STAR2;
    (void)discard;
}

/** Apply one of the protocol-supported standard bus frequencies. */
static int yi_i2c_ch32h4xx_configure(yi_device_t *dev, uint32_t frequency)
{
    const yi_i2c_ch32h4xx_config_t *cfg;
    yi_i2c_ch32h4xx_data_t *data;
    I2C_InitTypeDef vendor = {0};

    if((dev == NULL) || (dev->config == NULL) || (dev->data == NULL) ||
       ((frequency != 100000U) && (frequency != 400000U) &&
        (frequency != 1000000U))) { return YI_I2C_RESULT_INVALID; }
    cfg = dev->config;
    data = dev->data;
    I2C_Cmd(cfg->instance, DISABLE);
    vendor.I2C_ClockSpeed = frequency;
    vendor.I2C_Mode = I2C_Mode_I2C;
    vendor.I2C_DutyCycle = I2C_DutyCycle_2;
    vendor.I2C_OwnAddress1 = 0U;
    vendor.I2C_Ack = I2C_Ack_Enable;
    vendor.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(cfg->instance, &vendor);
    I2C_Cmd(cfg->instance, ENABLE);
    data->frequency = frequency;
    return YI_I2C_RESULT_OK;
}

/** Start and address one transmitter or receiver phase. */
static int yi_i2c_start(I2C_TypeDef *instance, uint8_t address,
                        uint8_t direction, uint32_t *budget)
{
    int result;

    I2C_GenerateSTART(instance, ENABLE);
    result = yi_i2c_wait(instance, I2C_FLAG_SB, budget);
    if(result != 0) { return result; }
    I2C_Send7bitAddress(instance, (uint8_t)(address << 1U), direction);
    result = yi_i2c_wait(instance, I2C_FLAG_ADDR, budget);
    return result;
}

/** Send one message and optionally retain the bus for a repeated START. */
static int yi_i2c_write(I2C_TypeDef *instance, yi_i2c_msg_t *message,
                        uint32_t *budget)
{
    uint16_t index;
    int result;

    yi_i2c_clear_address(instance);
    for(index = 0U; index < message->length; ++index) {
        result = yi_i2c_wait(instance, I2C_FLAG_TXE, budget);
        if(result != 0) { return result; }
        I2C_SendData(instance, message->buffer[index]);
    }
    result = yi_i2c_wait(instance, I2C_FLAG_BTF, budget);
    if((result == 0) && ((message->flags & YI_I2C_MSG_STOP) != 0U)) {
        I2C_GenerateSTOP(instance, ENABLE);
    }
    return result;
}

/** Receive one message with correct ACK/NACK handling for its final byte. */
static int yi_i2c_read(I2C_TypeDef *instance, yi_i2c_msg_t *message,
                       uint32_t *budget)
{
    uint16_t index;
    int result;

    if(message->length == 1U) {
        I2C_AcknowledgeConfig(instance, DISABLE);
        yi_i2c_clear_address(instance);
        I2C_GenerateSTOP(instance, ENABLE);
        result = yi_i2c_wait(instance, I2C_FLAG_RXNE, budget);
        if(result == 0) { message->buffer[0] = I2C_ReceiveData(instance); }
    } else {
        I2C_AcknowledgeConfig(instance, ENABLE);
        yi_i2c_clear_address(instance);
        result = YI_I2C_RESULT_OK;
        for(index = 0U; index < message->length; ++index) {
            if(index == (uint16_t)(message->length - 1U)) {
                I2C_AcknowledgeConfig(instance, DISABLE);
                I2C_GenerateSTOP(instance, ENABLE);
            }
            result = yi_i2c_wait(instance, I2C_FLAG_RXNE, budget);
            if(result != 0) { break; }
            message->buffer[index] = I2C_ReceiveData(instance);
        }
    }
    I2C_AcknowledgeConfig(instance, ENABLE);
    return result;
}

/** Execute one plain or combined YiCore I2C transaction by polling. */
static int yi_i2c_ch32h4xx_transfer(yi_device_t *dev, uint8_t address,
                                    yi_i2c_msg_t *messages,
                                    uint8_t message_count,
                                    uint32_t timeout_ms)
{
    const yi_i2c_ch32h4xx_config_t *cfg = dev->config;
    uint32_t budget = (HCLKClock / 1000U) * timeout_ms;
    uint8_t index;
    int result = YI_I2C_RESULT_OK;

    if((message_count > 2U) || (timeout_ms == 0U)) {
        return YI_I2C_RESULT_INVALID;
    }
    while((cfg->instance->STAR2 & (I2C_FLAG_BUSY >> 16U)) != 0U) {
        if(budget-- == 0U) { return YI_I2C_RESULT_TIMEOUT; }
    }
    for(index = 0U; index < message_count; ++index) {
        uint8_t direction = ((messages[index].flags & YI_I2C_MSG_READ) != 0U)
            ? I2C_Direction_Receiver : I2C_Direction_Transmitter;
        result = yi_i2c_start(cfg->instance, address, direction, &budget);
        if(result != 0) { break; }
        result = (direction == I2C_Direction_Receiver)
            ? yi_i2c_read(cfg->instance, &messages[index], &budget)
            : yi_i2c_write(cfg->instance, &messages[index], &budget);
        if(result != 0) { break; }
    }
    if(result != 0) { I2C_GenerateSTOP(cfg->instance, ENABLE); }
    return result;
}

/** Return the active I2C frequency cached by the CH32H4xx backend. */
static uint32_t yi_i2c_ch32h4xx_get_frequency(yi_device_t *dev)
{
    const yi_i2c_ch32h4xx_data_t *data = dev->data;
    return (data != NULL) ? data->frequency : 0U;
}

/** Initialize I2C pins, alternate functions, and the initial bus speed. */
int yi_i2c_ch32h4xx_init(const void *config)
{
    const yi_i2c_ch32h4xx_config_t *cfg = config;
    GPIO_InitTypeDef gpio = {0};
    uint32_t gpio_clock;

    if((cfg == NULL) || (cfg->instance == NULL) ||
       (cfg->scl_port == NULL) || (cfg->sda_port == NULL)) { return -1; }
    gpio_clock = yi_i2c_gpio_clock(cfg->scl_port) |
                 yi_i2c_gpio_clock(cfg->sda_port);
    if((gpio_clock == 0U) || (yi_i2c_clock_enable(cfg->instance) != 0)) {
        return -1;
    }
    RCC_HB2PeriphClockCmd(gpio_clock, ENABLE);
    GPIO_PinAFConfig(cfg->scl_port, cfg->scl_source, cfg->alternate);
    GPIO_PinAFConfig(cfg->sda_port, cfg->sda_source, cfg->alternate);
    gpio.GPIO_Pin = cfg->scl_pin;
    gpio.GPIO_Speed = GPIO_Speed_Very_High;
    gpio.GPIO_Mode = GPIO_Mode_AF_OD;
    GPIO_Init(cfg->scl_port, &gpio);
    gpio.GPIO_Pin = cfg->sda_pin;
    GPIO_Init(cfg->sda_port, &gpio);
    return 0;
}

/** YiCore I2C dispatch table backed by the WCH standard peripheral library. */
const yi_i2c_api_t yi_i2c_ch32h4xx_api = {
    .configure = yi_i2c_ch32h4xx_configure,
    .transfer = yi_i2c_ch32h4xx_transfer,
    .get_frequency = yi_i2c_ch32h4xx_get_frequency
};
