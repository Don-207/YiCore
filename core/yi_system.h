#ifndef YI_SYSTEM_H
#define YI_SYSTEM_H

#include <stdint.h>

int yi_system_init(void);
uint32_t yi_system_uptime_ms(void);
uint32_t yi_system_uptime_us(void);
void yi_system_delay_ms(uint32_t delay_ms);
void yi_system_delay_us(uint32_t delay_us);
void yi_system_irq_lock(void);

#endif
