/**
 * @file yi_device.h
 * @brief YiCore device interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

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

/**
 * @brief Perform the int operation.
 * @param config Device configuration.
 */
typedef int (*yi_device_init_t)(const void *config);

struct device;

typedef struct
{
    /*
     * 打开设备
     */
    int (*open)(struct device *dev);
    /*
     * 关闭设备
     */
    int (*close)(struct device *dev);
    /*
     * 流式设备读写接口；不支持时置为NULL
     */
    int (*write)(struct device *dev, const uint8_t *buf, uint32_t len);
    int (*read)(struct device *dev, uint8_t *buf, uint32_t len);
}yi_device_api_t;

typedef struct device
{
    const char *name; /**< Name value. */
    yi_device_init_t init; /**< Init value. */
    const void *config; /**< Config value. */
    void *data; /**< Data value. */
    const yi_device_api_t *api; /**< Api value. */
    yi_init_level_t init_level; /**< Init level value. */
    uint8_t init_priority; /**< Init priority value. */
    uint16_t init_order; /**< Stable declaration order within one priority. */
    yi_device_state_t state; /**< State value. */}yi_device_t;

/** Zephyr-style typedef for code that does not use the struct tag. */
typedef struct device device_t;

#define YI_DEVICE_MAX_NUM 32

/**
 * @brief Initialize all.
 */
int yi_device_init_all(void);

/**
 * @brief Initialize level.
 * @param level Initialization level.
 */
int yi_device_init_level(yi_init_level_t level);

/**
 * @brief Check whether ready.
 * @param dev Device instance.
 */
bool yi_device_is_ready(const yi_device_t *dev);

/** Return device readiness through the Zephyr-compatible public name. */
static inline bool device_is_ready(const struct device *dev)
{
    return yi_device_is_ready(dev);
}

/**
 * @brief Get the module.
 * @param name Registered device name.
 */
yi_device_t *yi_device_get(const char *name);

/*
 * 自动注册设备
 */
#if defined(YI_DEVICE_USE_AUTO_SECTION)
#define YI_DEVICE_SECTION_ATTRIBUTE __attribute__((used, section("yi_device")))
#else
#define YI_DEVICE_SECTION_ATTRIBUTE __attribute__((used, section(".yi_device")))
#endif

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
    .init_order = (uint16_t)__COUNTER__,                             \
    .state = YI_DEVICE_STATE_UNINITIALIZED                          \
};                                                                   \
                                                                     \
YI_DEVICE_SECTION_ATTRIBUTE                                          \
static yi_device_t * const _name##_ptr = &_name;

#define YI_DEVICE_DEFINE(_name, _init, _config)                       \
    YI_DEVICE_DEFINE_WITH_API(                                       \
        _name, YI_INIT_APPLICATION, YI_INIT_PRIORITY_DEFAULT,        \
        _init, _config, 0, 0)

/* Initialization-level names retain Zephyr spelling without kernel meaning. */
#define EARLY YI_INIT_PRE_KERNEL
#define PRE_KERNEL_1 YI_INIT_PRE_KERNEL
#define PRE_KERNEL_2 YI_INIT_PRE_KERNEL
#define POST_KERNEL YI_INIT_POST_KERNEL
#define APPLICATION YI_INIT_APPLICATION

/* Define a device using the common Zephyr driver declaration signature. */
#define DEVICE_DEFINE(_name, _init, _pm, _data, _config, _level,      \
                      _priority, _api, ...)                            \
    static int _name##_yi_init_adapter(const void *unused);           \
    YI_DEVICE_DEFINE_WITH_API(                                        \
        _name, _level, _priority, _name##_yi_init_adapter,            \
        _config, _data, (const yi_device_api_t *)(_api))              \
    static int _name##_yi_init_adapter(const void *unused)            \
    {                                                                 \
        (void)unused;                                                  \
        (void)(_pm);                                                   \
        return (_init != NULL) ? _init(&_name) : 0;                   \
    }

#define DEVICE_DT_DEFINE(_node_id, _init, _pm, _data, _config,       \
                         _level, _priority, _api, ...)                 \
    DEVICE_DEFINE(_node_id, _init, _pm, _data, _config, _level,      \
                  _priority, _api, __VA_ARGS__)

#define DEVICE_DT_INST_DEFINE(_inst, _init, _pm, _data, _config,     \
                              _level, _priority, _api, ...)            \
    DEVICE_DT_DEFINE(DT_INST(_inst, DT_DRV_COMPAT), _init, _pm,      \
                     _data, _config, _level, _priority, _api,         \
                     __VA_ARGS__)

#endif
