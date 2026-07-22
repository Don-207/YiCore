#include "yi_device.h"
#include <string.h>

#if defined(__ARMCC_VERSION)
extern yi_device_t * const Image$$yi_device$$Base[];
extern yi_device_t * const Image$$yi_device$$Limit[];
#define YI_DEVICE_SECTION_BEGIN Image$$yi_device$$Base
#define YI_DEVICE_SECTION_END   Image$$yi_device$$Limit
#elif defined(__GNUC__)
extern yi_device_t * const __yi_device_start[];
extern yi_device_t * const __yi_device_end[];
#define YI_DEVICE_SECTION_BEGIN __yi_device_start
#define YI_DEVICE_SECTION_END   __yi_device_end
#else
#error "Unsupported compiler: yi_device supports ARMCLANG and GCC"
#endif

static uint32_t device_count;
static yi_device_t *device_table[YI_DEVICE_MAX_NUM];
static uint8_t device_table_ready;

static int yi_device_collect(void)
{
    yi_device_t * const *dev;

    if(device_table_ready != 0U)
    {
        return 0;
    }

    device_count = 0U;

    for(dev = YI_DEVICE_SECTION_BEGIN; dev < YI_DEVICE_SECTION_END; dev++)
    {
        if((device_count >= YI_DEVICE_MAX_NUM) || (*dev == NULL))
        {
            device_count = 0U;
            return -1;
        }

        if(((*dev)->init_level >= YI_INIT_LEVEL_COUNT) ||
           ((*dev)->init_priority > YI_INIT_PRIORITY_LOWEST) ||
           ((*dev)->name == NULL))
        {
            device_count = 0U;
            return -1;
        }

        for(uint32_t i = 0U; i < device_count; i++)
        {
            if(strcmp(device_table[i]->name, (*dev)->name) == 0)
            {
                device_count = 0U;
                return -1;
            }
        }

        device_table[device_count++] = *dev;
    }

    device_table_ready = 1U;
    return 0;
}

static void yi_device_sort(void)
{
    for(uint32_t i = 1U; i < device_count; i++)
    {
        yi_device_t *item = device_table[i];
        uint32_t j = i;

        while(j > 0U)
        {
            yi_device_t *previous = device_table[j - 1U];
            bool item_before_previous =
                (item->init_level < previous->init_level) ||
                ((item->init_level == previous->init_level) &&
                 (item->init_priority < previous->init_priority));

            if(!item_before_previous)
            {
                break;
            }

            device_table[j] = previous;
            j--;
        }

        device_table[j] = item;
    }
}

int yi_device_init_level(yi_init_level_t level)
{
    int result = 0;

    if((level < YI_INIT_PRE_KERNEL) || (level >= YI_INIT_LEVEL_COUNT))
    {
        return -1;
    }

    if(yi_device_collect() != 0)
    {
        return -1;
    }

    yi_device_sort();

    for(uint32_t i = 0; i < device_count; i++)
    {
        yi_device_t *dev = device_table[i];

        if(dev->init_level != level)
        {
            continue;
        }

        if(dev->state == YI_DEVICE_STATE_READY)
        {
            continue;
        }
        if(dev->state != YI_DEVICE_STATE_UNINITIALIZED)
        {
            result = -1;
            continue;
        }

        dev->state = YI_DEVICE_STATE_INITIALIZING;

        if(dev->init)
        {
            if(dev->init(dev->config) != 0)
            {
                dev->state = YI_DEVICE_STATE_FAILED;
                result = -1;
                continue;
            }
        }

        dev->state = YI_DEVICE_STATE_READY;
    }

    return result;
}

int yi_device_init_all(void)
{
    int result = 0;

    for(yi_init_level_t level = YI_INIT_PRE_KERNEL;
        level < YI_INIT_LEVEL_COUNT;
        level++)
    {
        if(yi_device_init_level(level) != 0)
        {
            result = -1;
        }
    }

    return result;
}

bool yi_device_is_ready(const yi_device_t *dev)
{
    return (dev != NULL) && (dev->state == YI_DEVICE_STATE_READY);
}

yi_device_t *yi_device_get(const char *name)
{
    if((name == NULL) || (yi_device_collect() != 0))
    {
        return NULL;
    }

    for(uint32_t i = 0; i < device_count; i++)
    {
        if(strcmp(device_table[i]->name, name) == 0)
        {
            return device_table[i];
        }

    }

    return NULL;
}
