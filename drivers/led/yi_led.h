#ifndef YI_LED_H
#define YI_LED_H


#include "yi_device.h"


#ifdef __cplusplus
extern "C" {
#endif

/*
 * LED配置结构
 *
 * 来自 board/device tree
 */
typedef struct
{
    yi_device_t *gpio;
    uint8_t active_low;
}yi_led_config_t;

/*
 * LED运行数据
 */
typedef struct
{
    /*
     * 当前状态
     */
    uint8_t state;
}yi_led_data_t;

/*
 * LED专用API。device必须是首成员，以兼容yi_device_t::api。
 */
typedef struct
{
    yi_device_api_t device;
    int (*on)(yi_device_t *dev);
    int (*off)(yi_device_t *dev);
    int (*toggle)(yi_device_t *dev);
}yi_led_api_t;

static inline const yi_led_api_t *yi_led_get_api(const yi_device_t *dev)
{
    return (dev != NULL) ? (const yi_led_api_t *)dev->api : NULL;
}

static inline int yi_led_on(yi_device_t *dev)
{
    const yi_led_api_t *api = yi_led_get_api(dev);
    return (yi_device_is_ready(dev) && (api != NULL) && (api->on != NULL))
           ? api->on(dev) : -1;
}

static inline int yi_led_off(yi_device_t *dev)
{
    const yi_led_api_t *api = yi_led_get_api(dev);
    return (yi_device_is_ready(dev) && (api != NULL) && (api->off != NULL))
           ? api->off(dev) : -1;
}

static inline int yi_led_toggle(yi_device_t *dev)
{
    const yi_led_api_t *api = yi_led_get_api(dev);
    return (yi_device_is_ready(dev) && (api != NULL) && (api->toggle != NULL))
           ? api->toggle(dev) : -1;
}

/*
 * LED初始化
 */
int yi_led_init(const void *config);

/*
 * 打开LED
 */
extern const yi_led_api_t yi_led_driver_api;


/*
 * 创建LED设备
 *
 * 示例：
 *
 * static yi_led_config_t led0_cfg;
 *
 * YI_LED_DEFINE(
 *      led0,
 *      led0_cfg
 * );
 *
 */
#define YI_LED_DEFINE_LEVEL(_name, _level, _priority, _config) \
                                                    \
static yi_led_data_t _name##_data;                  \
                                                    \
YI_DEVICE_DEFINE_WITH_API(                          \
        _name,                                      \
        _level,                                     \
        _priority,                                  \
        yi_led_init,                                \
        &_config,                                   \
        &_name##_data,                              \
        &yi_led_driver_api.device                   \
);                                                 

#define YI_LED_DEFINE(_name, _config)                         \
    YI_LED_DEFINE_LEVEL(_name, YI_INIT_APPLICATION,           \
                        YI_INIT_PRIORITY_DEFAULT, _config)


#ifdef __cplusplus
}
#endif


#endif
