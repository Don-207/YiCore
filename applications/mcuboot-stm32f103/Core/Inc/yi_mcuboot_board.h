/**
 * @file yi_mcuboot_board.h
 * @brief YiCore mcuboot board interface.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#ifndef YI_MCUBOOT_BOARD_H
#define YI_MCUBOOT_BOARD_H

#include "bootutil/bootutil.h"

/**
 * @brief Initialize the module.
 */
int yi_mcuboot_board_flash_map_init(void);
/**
 * @brief Perform the yi mcuboot jump operation.
 * @param response Response value.
 */
void yi_mcuboot_jump(const struct boot_rsp *response);

#endif
