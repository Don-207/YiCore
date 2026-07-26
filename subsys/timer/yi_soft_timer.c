/**
 * @file yi_soft_timer.c
 * @brief YiCore soft timer implementation.
 * @author Don
 * @date 2026-07-26
 * @version 1.0.0
 */

#include "yi_soft_timer.h"
#include "yi_system.h"
#include <limits.h>
#include <stddef.h>

static yi_soft_timer_t *timer_list;

/**
 * @brief Perform the yi soft timer time reached operation.
 * @param now Now value.
 * @param deadline Deadline value.
 */
static bool yi_soft_timer_time_reached(uint32_t now, uint32_t deadline)
{
    return (int32_t)(now - deadline) >= 0;
}

/**
 * @brief Perform the yi soft timer register operation.
 * @param timer Timer value.
 */
static void yi_soft_timer_register(yi_soft_timer_t *timer)
{
    if(timer->registered)
    {
        return;
    }

    timer->next = timer_list;
    timer_list = timer;
    timer->registered = true;
}

/**
 * @brief Initialize the module.
 * @param timer Timer value.
 * @param callback Callback registration object.
 * @param user_data User data value.
 */
void yi_soft_timer_init(yi_soft_timer_t *timer,
                        yi_soft_timer_callback_t callback,
                        void *user_data)
{
    if(timer == NULL)
    {
        return;
    }

    timer->next = NULL;
    timer->callback = callback;
    timer->user_data = user_data;
    timer->deadline_ms = 0U;
    timer->period_ms = 0U;
    timer->active = false;
    timer->pending = false;
    timer->registered = false;
}

/**
 * @brief Start the module.
 * @param timer Timer value.
 * @param delay_ms Delay ms value.
 * @param period_ms Period ms value.
 */
int yi_soft_timer_start(yi_soft_timer_t *timer,
                        uint32_t delay_ms,
                        uint32_t period_ms)
{
    if((timer == NULL) || (timer->callback == NULL) ||
       (delay_ms > (uint32_t)INT32_MAX) ||
       (period_ms > (uint32_t)INT32_MAX))
    {
        return -1;
    }

    yi_soft_timer_register(timer);
    timer->deadline_ms = yi_system_uptime_ms() + delay_ms;
    timer->period_ms = period_ms;
    timer->pending = false;
    timer->active = true;
    return 0;
}

/**
 * @brief Stop the module.
 * @param timer Timer value.
 */
int yi_soft_timer_stop(yi_soft_timer_t *timer)
{
    if(timer == NULL)
    {
        return -1;
    }

    timer->active = false;
    timer->pending = false;
    return 0;
}

/**
 * @brief Check whether active.
 * @param timer Timer value.
 */
bool yi_soft_timer_is_active(const yi_soft_timer_t *timer)
{
    return (timer != NULL) && timer->active;
}

/**
 * @brief Perform the yi soft timer process operation.
 */
void yi_soft_timer_process(void)
{
    yi_soft_timer_t *timer;
    uint32_t now = yi_system_uptime_ms();

    for(timer = timer_list; timer != NULL; timer = timer->next)
    {
        if(!timer->active ||
           !yi_soft_timer_time_reached(now, timer->deadline_ms))
        {
            continue;
        }

        timer->pending = true;
        if(timer->period_ms == 0U)
        {
            timer->active = false;
        }
        else
        {
            do
            {
                timer->deadline_ms += timer->period_ms;
            } while(yi_soft_timer_time_reached(now, timer->deadline_ms));
        }
    }

    for(timer = timer_list; timer != NULL; timer = timer->next)
    {
        if(timer->pending)
        {
            yi_soft_timer_callback_t callback = timer->callback;
            void *user_data = timer->user_data;

            timer->pending = false;
            if(callback != NULL)
            {
                callback(timer, user_data);
            }
        }
    }
}
