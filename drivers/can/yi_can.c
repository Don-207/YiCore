#include "yi_can.h"
#include "yi_pinmux.h"

static int yi_can_timing(const yi_can_config_t *cfg, CAN_InitTypeDef *init)
{
    uint32_t clock = yi_stm32_periph_clock_rate(&cfg->clock);
    uint32_t best_error = UINT32_MAX;
    uint32_t best_bs1 = 0U, best_bs2 = 0U, best_prescaler = 0U;

    for(uint32_t bs1 = 1U; bs1 <= 16U; bs1++)
    {
        for(uint32_t bs2 = 1U; bs2 <= 8U; bs2++)
        {
            uint32_t tq = 1U + bs1 + bs2;
            uint64_t divisor = (uint64_t)cfg->bitrate * tq;
            uint32_t prescaler;
            uint32_t sample;
            uint32_t error;
            if((divisor == 0U) || ((clock % divisor) != 0U)) { continue; }
            prescaler = (uint32_t)(clock / divisor);
            if((prescaler == 0U) || (prescaler > 1024U)) { continue; }
            sample = ((1U + bs1) * 1000U) / tq;
            error = (sample > cfg->sample_point) ?
                    (sample - cfg->sample_point) : (cfg->sample_point - sample);
            if(error < best_error)
            {
                best_error = error; best_bs1 = bs1; best_bs2 = bs2;
                best_prescaler = prescaler;
            }
        }
    }
    if(best_prescaler == 0U) { return -1; }
    init->Prescaler = best_prescaler;
    init->SyncJumpWidth = CAN_SJW_1TQ;
    init->TimeSeg1 = (best_bs1 - 1U) << CAN_BTR_TS1_Pos;
    init->TimeSeg2 = (best_bs2 - 1U) << CAN_BTR_TS2_Pos;
    return 0;
}

int yi_can_init(const void *config)
{
    const yi_can_config_t *cfg = (const yi_can_config_t *)config;
    yi_can_data_t *data;
    CAN_FilterTypeDef filter = {0};
    if((cfg == NULL) || (cfg->self == NULL) || (cfg->self->data == NULL) ||
       !yi_device_is_ready(cfg->tx_pin) || !yi_device_is_ready(cfg->rx_pin) ||
       (cfg->bitrate == 0U) || (cfg->sample_point > 1000U) ||
       (cfg->irq_priority > 15U)) { return -1; }
    if((yi_stm32_periph_clock_enable(&cfg->clock) != 0) ||
       (yi_pinmux_apply(cfg->tx_pin) != 0) ||
       (yi_pinmux_apply(cfg->rx_pin) != 0)) { return -1; }
    data = (yi_can_data_t *)cfg->self->data;
    data->hcan.Instance = cfg->instance;
    data->hcan.Init.Mode = CAN_MODE_NORMAL;
    data->hcan.Init.TimeTriggeredMode = DISABLE;
    data->hcan.Init.AutoBusOff = ENABLE;
    data->hcan.Init.AutoWakeUp = DISABLE;
    data->hcan.Init.AutoRetransmission = ENABLE;
    data->hcan.Init.ReceiveFifoLocked = DISABLE;
    data->hcan.Init.TransmitFifoPriority = DISABLE;
    if((yi_can_timing(cfg, &data->hcan.Init) != 0) ||
       (HAL_CAN_Init(&data->hcan) != HAL_OK)) { return -1; }

    filter.FilterBank = 0U;
    filter.FilterMode = CAN_FILTERMODE_IDMASK;
    filter.FilterScale = CAN_FILTERSCALE_32BIT;
    filter.FilterIdHigh = 0U; filter.FilterIdLow = 0U;
    filter.FilterMaskIdHigh = 0U; filter.FilterMaskIdLow = 0U;
    filter.FilterFIFOAssignment = CAN_RX_FIFO0;
    filter.FilterActivation = ENABLE;
    filter.SlaveStartFilterBank = 14U;
    if((HAL_CAN_ConfigFilter(&data->hcan, &filter) != HAL_OK) ||
       (HAL_CAN_Start(&data->hcan) != HAL_OK) ||
       (HAL_CAN_ActivateNotification(&data->hcan,
            CAN_IT_TX_MAILBOX_EMPTY | CAN_IT_RX_FIFO0_MSG_PENDING |
            CAN_IT_RX_FIFO1_MSG_PENDING | CAN_IT_BUSOFF | CAN_IT_ERROR) != HAL_OK))
    {
        return -1;
    }

    HAL_NVIC_SetPriority(cfg->tx_irqn, cfg->irq_priority, 0U);
    HAL_NVIC_SetPriority(cfg->rx0_irqn, cfg->irq_priority, 0U);
    HAL_NVIC_SetPriority(cfg->rx1_irqn, cfg->irq_priority, 0U);
    HAL_NVIC_SetPriority(cfg->sce_irqn, cfg->irq_priority, 0U);
    HAL_NVIC_EnableIRQ(cfg->tx_irqn); HAL_NVIC_EnableIRQ(cfg->rx0_irqn);
    HAL_NVIC_EnableIRQ(cfg->rx1_irqn); HAL_NVIC_EnableIRQ(cfg->sce_irqn);
    return 0;
}

int yi_can_send(yi_device_t *dev, const yi_can_frame_t *frame, uint32_t *mailbox)
{
    CAN_TxHeaderTypeDef header = {0};
    if(!yi_device_is_ready(dev) || (dev->data == NULL) || (frame == NULL) ||
       (mailbox == NULL) || (frame->length > 8U)) { return -1; }
    header.IDE = frame->extended ? CAN_ID_EXT : CAN_ID_STD;
    header.ExtId = frame->id; header.StdId = frame->id;
    header.RTR = frame->remote ? CAN_RTR_REMOTE : CAN_RTR_DATA;
    header.DLC = frame->length;
    return (HAL_CAN_AddTxMessage(&((yi_can_data_t *)dev->data)->hcan, &header,
            (uint8_t *)frame->data, mailbox) == HAL_OK) ? 0 : -1;
}

int yi_can_receive(yi_device_t *dev, uint32_t fifo, yi_can_frame_t *frame)
{
    CAN_RxHeaderTypeDef header;
    if(!yi_device_is_ready(dev) || (dev->data == NULL) || (frame == NULL) ||
       ((fifo != CAN_RX_FIFO0) && (fifo != CAN_RX_FIFO1))) { return -1; }
    if(HAL_CAN_GetRxMessage(&((yi_can_data_t *)dev->data)->hcan, fifo,
                             &header, frame->data) != HAL_OK) { return -1; }
    frame->extended = (header.IDE == CAN_ID_EXT);
    frame->remote = (header.RTR == CAN_RTR_REMOTE);
    frame->id = frame->extended ? header.ExtId : header.StdId;
    frame->length = (uint8_t)header.DLC;
    return 0;
}

void yi_can_irq_handler(yi_device_t *dev)
{
    if((dev != NULL) && (dev->data != NULL))
        HAL_CAN_IRQHandler(&((yi_can_data_t *)dev->data)->hcan);
}
