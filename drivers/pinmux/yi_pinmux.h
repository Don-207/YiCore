#ifndef YI_PINMUX_H
#define YI_PINMUX_H

#include "yi_device.h"

typedef enum
{
    YI_PINMUX_FUNCTION_GPIO = 0,
    YI_PINMUX_FUNCTION_UART_TX,
    YI_PINMUX_FUNCTION_UART_RX,
    YI_PINMUX_FUNCTION_SPI_CLOCK,
    YI_PINMUX_FUNCTION_SPI_MOSI,
    YI_PINMUX_FUNCTION_SPI_MISO,
    YI_PINMUX_FUNCTION_I2C_SCL,
    YI_PINMUX_FUNCTION_I2C_SDA,
    YI_PINMUX_FUNCTION_CAN_TX,
    YI_PINMUX_FUNCTION_CAN_RX,
    YI_PINMUX_FUNCTION_PWM,
    YI_PINMUX_FUNCTION_ADC
} yi_pinmux_function_t;

typedef enum
{
    YI_PINMUX_MODE_INPUT = 0,
    YI_PINMUX_MODE_OUTPUT_PUSH_PULL,
    YI_PINMUX_MODE_OUTPUT_OPEN_DRAIN,
    YI_PINMUX_MODE_ALTERNATE_PUSH_PULL,
    YI_PINMUX_MODE_ALTERNATE_OPEN_DRAIN,
    YI_PINMUX_MODE_ANALOG
} yi_pinmux_mode_t;

typedef enum
{
    YI_PINMUX_PULL_NONE = 0,
    YI_PINMUX_PULL_UP,
    YI_PINMUX_PULL_DOWN
} yi_pinmux_pull_t;

typedef enum
{
    YI_PINMUX_SPEED_LOW = 0,
    YI_PINMUX_SPEED_MEDIUM,
    YI_PINMUX_SPEED_HIGH
} yi_pinmux_speed_t;

typedef struct
{
    void *port;
    uint16_t pin;
    yi_pinmux_mode_t mode;
    yi_pinmux_pull_t pull;
    yi_pinmux_speed_t speed;
    yi_pinmux_function_t function;
    yi_device_t *clock;
} yi_pinmux_config_t;

int yi_pinmux_init(const void *config);
int yi_pinmux_apply(yi_device_t *dev);
int yi_pinmux_release(yi_device_t *dev);

#define YI_PINMUX_DEFINE_LEVEL(_name, _level, _priority, _config) \
    YI_DEVICE_DEFINE_WITH_API(                                  \
        _name, _level, _priority, yi_pinmux_init,                \
        &_config, NULL, NULL                                    \
    )

#define YI_PINMUX_DEFINE(_name, _config)                         \
    YI_PINMUX_DEFINE_LEVEL(_name, YI_INIT_PRE_KERNEL,            \
                           YI_INIT_PRIORITY_DEFAULT, _config)

#endif
