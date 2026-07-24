#ifndef YI_DEVICE_H
#define YI_DEVICE_H


#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#define YI_INIT_PRIORITY_HIGHEST  0
#define YI_INIT_PRIORITY_DEFAULT  50
#define YI_INIT_PRIORITY_LOWEST   99

typedef enum
{
    YI_INIT_PRE_KERNEL = 0,
    YI_INIT_POST_KERNEL,
    YI_INIT_APPLICATION,
    YI_INIT_LEVEL_COUNT
} yi_init_level_t;

typedef enum
{
    YI_DEVICE_STATE_UNINITIALIZED = 0,
    YI_DEVICE_STATE_INITIALIZING,
    YI_DEVICE_STATE_READY,
    YI_DEVICE_STATE_FAILED
} yi_device_state_t;

typedef int (*yi_device_init_t)(const void *config);

struct yi_device;

typedef struct
{
    /*
     * 打开设备
     */
    int (*open)( struct yi_device *dev);
    /*
     * 关闭设备
     */
    int (*close)(struct yi_device *dev);
    /*
     * 流式设备读写接口；不支持时置为NULL
     */
    int (*write)(struct yi_device *dev, const uint8_t *buf, uint32_t len);
    int (*read)(struct yi_device *dev, uint8_t *buf, uint32_t len);
}yi_device_api_t;

typedef struct yi_device
{
    const char *name;
    yi_device_init_t init;
    const void *config;
    void *data;
    const yi_device_api_t *api;
    yi_init_level_t init_level;
    uint8_t init_priority;
    yi_device_state_t state;
}yi_device_t;

#define YI_DEVICE_MAX_NUM 32

int yi_device_init_all(void);

int yi_device_init_level(yi_init_level_t level);

bool yi_device_is_ready(const yi_device_t *dev);

yi_device_t *yi_device_get(const char *name);

/*
 * 自动注册设备
 */
#define YI_DEVICE_DEFINE_WITH_API(_name, _level, _priority, _init, _config, _data, _api) \
                                                                     \
static yi_device_t _name =                                           \
{                                                                    \
    .name = #_name,                                                  \
    .init = _init,                                                   \
    .config = _config,                                               \
    .data = _data,                                                   \
    .api = _api,                                                     \
    .init_level = (_level),                                         \
    .init_priority = (_priority),                                   \
    .state = YI_DEVICE_STATE_UNINITIALIZED                          \
};                                                                   \
                                                                     \
__attribute__((used, section(".yi_device")))                         \
static yi_device_t * const _name##_ptr = &_name;

#define YI_DEVICE_DEFINE(_name, _init, _config)                       \
    YI_DEVICE_DEFINE_WITH_API(                                       \
        _name, YI_INIT_APPLICATION, YI_INIT_PRIORITY_DEFAULT,        \
        _init, _config, 0, 0)

#endif
