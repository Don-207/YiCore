/**
 * @file main.c
 * @brief Start the selected STM32 platform and remain in the idle loop.
 * @author Don
 * @date 2026-07-31
 * @version 1.0.0
 */

#include "yi_device.h"
#include "yi_generated.h"
#include "yi_led.h"
#include "yi_system.h"

/**
 * @brief Initialize system and the currently empty bootstrap device table.
 * @return This function does not return during normal operation.
 */
int main(void)
{
    yi_device_t *led;

    if((yi_system_init() != 0) || (yi_device_init_all() != 0))
    {
        yi_system_irq_lock();
    }

    led = YI_DT_GET(LED0);
    if(!yi_device_is_ready(led))
    {
        yi_system_irq_lock();
    }

    while(1)
    {
        (void)yi_led_toggle(led);
        yi_system_delay_ms(500U);
    }
}
