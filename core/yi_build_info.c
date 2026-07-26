#include "yi_build_info.h"

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
