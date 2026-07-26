/**
 * @file yi_gpio.h
 * @brief YiCore gpio interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_GPIO_H
#define YI_GPIO_H

#include "yi_device.h"

#define YI_GPIO_PIN(_index) ((uint16_t)(1U << (_index)))

typedef enum
{
    YI_GPIO_LOW = 0,
    YI_GPIO_HIGH = 1
}yi_gpio_value_t;

typedef enum
{
    YI_GPIO_DIRECTION_INPUT = 0,
    YI_GPIO_DIRECTION_OUTPUT
} yi_gpio_direction_t;

typedef enum
{
    YI_GPIO_INTERRUPT_NONE = 0,
    YI_GPIO_INTERRUPT_RISING,
    YI_GPIO_INTERRUPT_FALLING,
    YI_GPIO_INTERRUPT_BOTH
} yi_gpio_interrupt_t;

typedef enum
{
    YI_GPIO_PULL_NONE = 0,
    YI_GPIO_PULL_UP,
    YI_GPIO_PULL_DOWN
} yi_gpio_pull_t;

typedef enum
{
    YI_GPIO_DRIVE_PUSH_PULL = 0,
    YI_GPIO_DRIVE_OPEN_DRAIN
} yi_gpio_drive_t;

struct yi_gpio_callback;
/**
 * @brief Perform the void operation.
 * @param dev Device instance.
 * @param callback Callback registration object.
 * @param pins GPIO pin mask.
 */
typedef void (*yi_gpio_callback_handler_t)(yi_device_t *dev,
                                           struct yi_gpio_callback *callback,
                                           uint16_t pins);

typedef struct yi_gpio_callback
{
    struct yi_gpio_callback *next; /**< Next value. */
    yi_gpio_callback_handler_t handler; /**< Handler value. */
    uint16_t pin_mask; /**< Pin mask value. */} yi_gpio_callback_t;

typedef struct
{
    yi_device_t *self; /**< Self value. */
    void *port; /**< Port value. */
    uint16_t pin; /**< Pin value. */
    yi_device_t *clock; /**< Clock value. */
    yi_gpio_direction_t direction; /**< Direction value. */
    yi_gpio_drive_t drive; /**< Drive value. */
    yi_gpio_pull_t pull; /**< Pull value. */
    yi_gpio_interrupt_t interrupt; /**< Interrupt value. */
    uint8_t irq_priority; /**< Irq priority value. */}yi_gpio_config_t;

typedef struct
{
    yi_gpio_callback_t *callbacks; /**< Callbacks value. */} yi_gpio_data_t;

/*
 * GPIO设备API
 */
typedef struct
{
    int (*set)(yi_device_t *dev, yi_gpio_value_t value);
    int (*get)(yi_device_t *dev);
    int (*toggle)(yi_device_t *dev);
}yi_gpio_api_t;

/*
 * GPIO初始化
 */
int yi_gpio_init(const void *config);

/*
 * GPIO操作
 */
int yi_gpio_set(yi_device_t *dev, yi_gpio_value_t value);

/**
 * @brief Get the module.
 * @param dev Device instance.
 */
int yi_gpio_get(yi_device_t *dev);

/**
 * @brief Perform the yi gpio toggle operation.
 * @param dev Device instance.
 */
int yi_gpio_toggle(yi_device_t *dev);

/**
 * @brief Initialize the module.
 * @param callback Callback registration object.
 * @param handler Callback function to invoke.
 * @param pin_mask GPIO pin mask.
 */
void yi_gpio_callback_init(yi_gpio_callback_t *callback,
                           yi_gpio_callback_handler_t handler,
                           uint16_t pin_mask);
/**
 * @brief Perform the yi gpio add callback operation.
 * @param dev Device instance.
 * @param callback Callback registration object.
 */
int yi_gpio_add_callback(yi_device_t *dev, yi_gpio_callback_t *callback);
/**
 * @brief Perform the yi gpio remove callback operation.
 * @param dev Device instance.
 * @param callback Callback registration object.
 */
int yi_gpio_remove_callback(yi_device_t *dev, yi_gpio_callback_t *callback);
/**
 * @brief Perform the yi gpio irq handler operation.
 * @param pins GPIO pin mask.
 */
void yi_gpio_irq_handler(uint16_t pins);

#define YI_GPIO_DEFINE_LEVEL(_name, _level, _priority, _config, _data) \
    YI_DEVICE_DEFINE_WITH_API(                                        \
        _name, _level, _priority, yi_gpio_init,                       \
        &_config, &_data, NULL                                        \
    )

#endif
