/**
 * @file yi_build_info.c
 * @brief YiCore build info implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_build_info.h"

/**
 * @brief Get the module.
 */
const yi_build_info_t *yi_build_info_get(void)
{
    if((yi_build_info.magic != YI_BUILD_INFO_MAGIC) ||
       (yi_build_info.format_version != YI_BUILD_INFO_FORMAT_VERSION) ||
       (yi_build_info.size != sizeof(yi_build_info_t)))
    {
        return 0;
    }

    return &yi_build_info;
}
