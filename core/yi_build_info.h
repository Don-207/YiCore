#ifndef YI_BUILD_INFO_H
#define YI_BUILD_INFO_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define YI_BUILD_INFO_MAGIC 0x59494249U
#define YI_BUILD_INFO_FORMAT_VERSION 1U

typedef struct
{
    uint32_t magic;
    uint16_t format_version;
    uint16_t size;
    char image[16];
    char version[32];
    char build_date[11];
    char build_time[9];
} yi_build_info_t;

extern const yi_build_info_t yi_build_info;

const yi_build_info_t *yi_build_info_get(void);

#ifdef __cplusplus
}
#endif

#endif
