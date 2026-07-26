/**
 * @file yi_build_info.h
 * @brief YiCore build info interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

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
    uint32_t magic; /**< Magic value. */
    uint16_t format_version; /**< Format version value. */
    uint16_t size; /**< Size value. */
    char image[16]; /**< Image value. */
    char version[32]; /**< Version value. */
    char build_date[11]; /**< Build date value. */
    char build_time[9]; /**< Build time value. */} yi_build_info_t;

extern const yi_build_info_t yi_build_info;

/**
 * @brief Get the module.
 */
const yi_build_info_t *yi_build_info_get(void);

#ifdef __cplusplus
}
#endif

#endif
