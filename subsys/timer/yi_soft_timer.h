#ifndef YI_SOFT_TIMER_H
#define YI_SOFT_TIMER_H

#include <stdbool.h>
#include <stdint.h>

typedef struct yi_soft_timer yi_soft_timer_t;

typedef void (*yi_soft_timer_callback_t)(yi_soft_timer_t *timer,
                                         void *user_data);

struct yi_soft_timer
{
    yi_soft_timer_t *next;
    yi_soft_timer_callback_t callback;
    void *user_data;
    uint32_t deadline_ms;
    uint32_t period_ms;
    bool active;
    bool pending;
    bool registered;
};

/*
 * Software timer callbacks run from yi_soft_timer_process(), not from SysTick.
 * Timer objects must remain allocated after they have been started.
 * These APIs are intended for main-loop context and are not interrupt-safe.
 */
void yi_soft_timer_init(yi_soft_timer_t *timer,
                        yi_soft_timer_callback_t callback,
                        void *user_data);
int yi_soft_timer_start(yi_soft_timer_t *timer,
                        uint32_t delay_ms,
                        uint32_t period_ms);
int yi_soft_timer_stop(yi_soft_timer_t *timer);
bool yi_soft_timer_is_active(const yi_soft_timer_t *timer);
void yi_soft_timer_process(void);

#endif
