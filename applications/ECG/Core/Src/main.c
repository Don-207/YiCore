/**
 * @file main.c
 * @brief YiCore ECG application entry.
 */

#include "main.h"

#include "../../App/ecg_service.h"
#include "yi_device.h"
#include "yi_generated.h"
#include "yi_system.h"
#include "yi_soft_timer.h"
#include "yi_led.h"

static yi_soft_timer_t blink_timer;
static void blink_timer_expired(yi_soft_timer_t *timer, void *user_data)
{
  (void)timer;
  (void)yi_led_toggle((yi_device_t *)user_data);
}

int main(void)
{
    if((yi_system_init() != 0) ||
       (yi_device_init_all() != 0) ||
       (ecg_service_init() != 0))
    {
        Error_Handler();
    }

    yi_device_t *led = YI_DT_GET(LED0);
    yi_soft_timer_init(&blink_timer, blink_timer_expired, led);
    if(yi_soft_timer_start(&blink_timer, 100U, 100U) != 0)
    {
        Error_Handler();
    }

    while(1)
    {
        yi_soft_timer_process();
        ecg_service_process();
    }
}

void Error_Handler(void)
{
    yi_system_irq_lock();
    while(1) { }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
