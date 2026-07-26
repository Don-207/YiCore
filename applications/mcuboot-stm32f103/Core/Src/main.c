#include "bootutil/bootutil.h"
#include "bootutil/fault_injection_hardening.h"
#include "yi_device.h"
#include "yi_mcuboot_board.h"
#include "yi_system.h"

static void boot_failure(void)
{
    yi_system_irq_lock();
    for(;;)
    {
    }
}

int main(void)
{
    struct boot_rsp response;
    fih_ret result = FIH_FAILURE;

    if((yi_system_init() != 0) ||
       (yi_device_init_all() != 0) ||
       (yi_mcuboot_board_flash_map_init() != 0))
    {
        boot_failure();
    }

    FIH_CALL(boot_go, result, &response);
    if(FIH_NOT_EQ(result, FIH_SUCCESS))
    {
        boot_failure();
    }

    yi_mcuboot_jump(&response);
    boot_failure();
    return 0;
}
