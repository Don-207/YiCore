/**
 * @file main.c
 * @brief YiCore application entry.
 */

#include "main.h"

#include "yi_device.h"
#include "yi_system.h"

int main(void)
{
    if((yi_system_init() != 0) || (yi_device_init_all() != 0))
    {
        Error_Handler();
    }

    while(1)
    {
    }
}

void Error_Handler(void)
{
    yi_system_irq_lock();
    while(1)
    {
    }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
    (void)file;
    (void)line;
}
#endif
