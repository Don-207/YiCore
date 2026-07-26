#ifndef YI_MCUBOOT_BOARD_H
#define YI_MCUBOOT_BOARD_H

#include "bootutil/bootutil.h"

int yi_mcuboot_board_flash_map_init(void);
void yi_mcuboot_jump(const struct boot_rsp *response);

#endif
