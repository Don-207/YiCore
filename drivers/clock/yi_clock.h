#ifndef YI_CLOCK_H
#define YI_CLOCK_H

#include "yi_device.h"

typedef enum
{
    YI_STM32_CLOCK_GPIOA = 0,
    YI_STM32_CLOCK_GPIOB,
    YI_STM32_CLOCK_GPIOC,
    YI_STM32_CLOCK_GPIOD,
    YI_STM32_CLOCK_GPIOE,
    YI_STM32_CLOCK_USART1,
    YI_STM32_CLOCK_USART2,
    YI_STM32_CLOCK_USART3,
    YI_STM32_CLOCK_SPI1,
    YI_STM32_CLOCK_SPI2,
    YI_STM32_CLOCK_COUNT
} yi_stm32_clock_id_t;

typedef struct
{
    yi_stm32_clock_id_t id;
} yi_clock_config_t;

typedef struct
{
    uint16_t reference_count;
} yi_clock_data_t;

int yi_clock_init(const void *config);
int yi_clock_enable(yi_device_t *dev);
int yi_clock_disable(yi_device_t *dev);
uint32_t yi_clock_get_rate(yi_device_t *dev);

#define YI_CLOCK_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                      \
        _name, _level, _priority, yi_clock_init,                    \
        &_config, &_data, NULL                                     \
    )

#endif
