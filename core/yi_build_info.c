/**
 * @file yi_build_info.c
 * @brief YiCore build info implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_build_info.h"

/**
 * @brief Validate a build information record.
 * @param info Build information record.
 * @return Valid build information, or NULL when the record is invalid.
 */
static const yi_build_info_t *yi_build_info_validate(
    const yi_build_info_t *info)
{
    if((info == 0) ||
       (info->magic != YI_BUILD_INFO_MAGIC) ||
       (info->format_version != YI_BUILD_INFO_FORMAT_VERSION) ||
       (info->size != sizeof(yi_build_info_t)))
    {
        return 0;
    }

    return info;
}

/**
 * @brief Get the module.
 */
const yi_build_info_t *yi_build_info_get(void)
{
    return yi_build_info_validate(&yi_build_info);
}

/**
 * @brief Get and validate build information at a fixed memory address.
 */
const yi_build_info_t *yi_build_info_at(uintptr_t address)
{
    if((address == 0U) ||
       ((address & ((uintptr_t)sizeof(uint32_t) - 1U)) != 0U))
    {
        return 0;
    }

    return yi_build_info_validate((const yi_build_info_t *)address);
}
