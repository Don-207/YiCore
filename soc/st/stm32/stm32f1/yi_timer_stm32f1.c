#include "yi_timer_stm32f1.h"

static int yi_timer_calculate_period(const yi_timer_config_t *cfg,
                                     uint32_t *prescaler,
                                     uint32_t *period)
{
    uint64_t timer_clock = yi_stm32_periph_clock_rate(&cfg->clock);
    uint64_t max_period = (cfg->counter_bits == 32U)
                          ? 0x100000000ULL
                          : 0x10000ULL;

    for(uint32_t divider = 1U; divider <= 0x10000U; divider++)
    {
        uint64_t denominator = (uint64_t)cfg->tick_frequency * divider;
        uint64_t candidate;

        if((denominator == 0U) || ((timer_clock % denominator) != 0U))
        {
            continue;
        }
        candidate = timer_clock / denominator;
        if((candidate > 0U) && (candidate <= max_period))
        {
            *prescaler = divider - 1U;
            *period = (uint32_t)(candidate - 1U);
            return 0;
        }
    }

    return -1;
}

int yi_timer_init(const void *config)
{
    const yi_timer_config_t *cfg = (const yi_timer_config_t *)config;
    yi_timer_data_t *data;
    uint32_t prescaler;
    uint32_t period;

    if((cfg == NULL) || (cfg->self == NULL) || (cfg->instance == NULL) ||
       (cfg->clock.enable_mask == 0U) ||
       ((cfg->counter_bits != 16U) && (cfg->counter_bits != 32U)) ||
       (cfg->tick_frequency == 0U) || (cfg->irq_priority > 15U) ||
       (cfg->self->data == NULL))
    {
        return -1;
    }
    if(yi_timer_calculate_period(cfg, &prescaler, &period) != 0)
    {
        return -1;
    }

    data = (yi_timer_data_t *)cfg->self->data;
    if(yi_stm32_periph_clock_enable(&cfg->clock) != 0)
    {
        return -1;
    }

    data->htim.Instance = cfg->instance;
    data->htim.Init.Prescaler = prescaler;
    data->htim.Init.CounterMode = TIM_COUNTERMODE_UP;
    data->htim.Init.Period = period;
    data->htim.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    data->htim.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    data->period_count = 0U;
    data->running = false;

    if(HAL_TIM_Base_Init(&data->htim) != HAL_OK)
    {
        (void)yi_stm32_periph_clock_disable(&cfg->clock);
        return -1;
    }

    HAL_NVIC_SetPriority(cfg->irqn, cfg->irq_priority, 0U);
    HAL_NVIC_EnableIRQ(cfg->irqn);
    if(yi_timer_start(cfg->self) != 0)
    {
        HAL_NVIC_DisableIRQ(cfg->irqn);
        (void)yi_stm32_periph_clock_disable(&cfg->clock);
        return -1;
    }

    return 0;
}

int yi_timer_start(yi_device_t *dev)
{
    yi_timer_data_t *data;

    if((dev == NULL) || (dev->data == NULL))
    {
        return -1;
    }
    data = (yi_timer_data_t *)dev->data;
    if(HAL_TIM_Base_Start_IT(&data->htim) != HAL_OK)
    {
        return -1;
    }
    data->running = true;
    return 0;
}

int yi_timer_stop(yi_device_t *dev)
{
    yi_timer_data_t *data;

    if((dev == NULL) || (dev->data == NULL))
    {
        return -1;
    }
    data = (yi_timer_data_t *)dev->data;
    if(HAL_TIM_Base_Stop_IT(&data->htim) != HAL_OK)
    {
        return -1;
    }
    data->running = false;
    return 0;
}

uint32_t yi_timer_get_period_count(const yi_device_t *dev)
{
    const yi_timer_data_t *data;

    if((dev == NULL) || (dev->data == NULL))
    {
        return 0U;
    }
    data = (const yi_timer_data_t *)dev->data;
    return data->period_count;
}

void yi_timer_irq_handler(yi_device_t *dev)
{
    yi_timer_data_t *data;

    if((dev == NULL) || (dev->data == NULL))
    {
        return;
    }
    data = (yi_timer_data_t *)dev->data;
    if((__HAL_TIM_GET_FLAG(&data->htim, TIM_FLAG_UPDATE) != RESET) &&
       (__HAL_TIM_GET_IT_SOURCE(&data->htim, TIM_IT_UPDATE) != RESET))
    {
        __HAL_TIM_CLEAR_IT(&data->htim, TIM_IT_UPDATE);
        data->period_count++;
    }
}
