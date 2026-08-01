/**
 * @file main.c
 * @brief Validate CH32H417 V3F startup, clock setup, and GPIO output.
 * @author Don
 * @date 2026-07-31
 * @version 1.0.0
 */

#include "ch32h417.h"
#include "yi_system.h"

/** Heartbeat half-period in milliseconds. */
#define YI_CH32H417_HEARTBEAT_HALF_PERIOD_MS (250U)

/**
 * @brief Initialize PB1 and toggle it forever as the first V3F bench test.
 * @return Never returns during normal operation.
 * @note Runs on V3F only and changes the global clock tree and GPIOB pin 1.
 */
int main(void)
{
    /** Vendor GPIO configuration for the active-high PB1 heartbeat output. */
    GPIO_InitTypeDef heartbeat_gpio = {0};

    (void)yi_system_init();
    RCC_HB2PeriphClockCmd(RCC_HB2Periph_GPIOB, ENABLE);
    heartbeat_gpio.GPIO_Pin = GPIO_Pin_1;
    heartbeat_gpio.GPIO_Speed = GPIO_Speed_Very_High;
    heartbeat_gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_Init(GPIOB, &heartbeat_gpio);

    for (;;) {
        GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_SET);
        yi_system_delay_ms(YI_CH32H417_HEARTBEAT_HALF_PERIOD_MS);
        GPIO_WriteBit(GPIOB, GPIO_Pin_1, Bit_RESET);
        yi_system_delay_ms(YI_CH32H417_HEARTBEAT_HALF_PERIOD_MS);
    }
}
