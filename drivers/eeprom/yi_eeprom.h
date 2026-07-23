#ifndef YI_EEPROM_H
#define YI_EEPROM_H

#include "yi_device.h"

typedef struct
{
    uint32_t size;
    uint32_t page_size;
} yi_eeprom_config_t;

typedef struct
{
    int (*read)(yi_device_t *dev, uint32_t offset,
                void *data, uint32_t length);
    int (*write)(yi_device_t *dev, uint32_t offset,
                 const void *data, uint32_t length);
} yi_eeprom_api_t;

int yi_eeprom_read(yi_device_t *dev, uint32_t offset,
                   void *data, uint32_t length);
int yi_eeprom_write(yi_device_t *dev, uint32_t offset,
                    const void *data, uint32_t length);
uint32_t yi_eeprom_get_size(const yi_device_t *dev);
uint32_t yi_eeprom_get_page_size(const yi_device_t *dev);

#endif
