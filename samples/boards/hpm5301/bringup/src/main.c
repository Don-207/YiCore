/**
 * @file main.c
 * @brief Exercise HPM5301 startup, clock, UART and RISC-V IRQ primitives.
 * @author Don
 * @date 2026-07-29
 * @version 1.0.0
 */

#include <stdint.h>
#include <stdio.h>

#include "board.h"
#include "yi_riscv_irq.h"

/** Delay between bring-up heartbeat messages, in milliseconds. */
#define YI_HPM5301_HEARTBEAT_MS (1000U)

/**
 * @brief Start the official board runtime and emit a polling UART heartbeat.
 * @return Never returns during normal operation.
 * @note Runs in machine mode; UART is initialized by board_init().
 */
int main(void)
{
    /** Saved machine interrupt state used to verify nested-safe restoration. */
    uint32_t irq_key;
    /** Monotonic heartbeat sequence used during bench validation. */
    uint32_t heartbeat = 0U;

    board_init();

    irq_key = yi_riscv_irq_lock();
    yi_riscv_memory_barrier();
    yi_riscv_irq_unlock(irq_key);

    printf("YiCore HPM5301 architecture bring-up ready\r\n");
    for (;;) {
        printf("heartbeat %lu\r\n", (unsigned long)heartbeat);
        heartbeat++;
        board_delay_ms(YI_HPM5301_HEARTBEAT_MS);
    }
}
