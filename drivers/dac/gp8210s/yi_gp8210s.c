/** @file yi_gp8210s.c @brief YiCore GP8210S implementation. */
#include "yi_gp8210s.h"
#include <string.h>
#define GP8210S_REG_RANGE 0x01U
#define GP8210S_REG_CHANNEL0 0x02U
#define GP8210S_REG_CHANNEL1 0x04U
#define GP8210S_RANGE_10V_VALUE 0x11U
/** @brief Validate a GP8210S device object. */
static bool gp8210s_valid(yi_device_t *dev)
{ return dev != NULL && dev->config != NULL && dev->data != NULL; }
/** @brief Write bytes to a GP8210S register. */
static int gp8210s_write(const yi_gp8210s_config_t *cfg, uint8_t reg,
                         const uint8_t *data, uint8_t length)
{
    uint8_t tx[5]; if(length > 4U) return -1; tx[0] = reg;
    for(uint8_t i=0U;i<length;i++) tx[i+1U]=data[i];
    return yi_i2c_master_write(cfg->i2c, cfg->address, tx,
                               (uint16_t)length+1U, cfg->transfer_timeout_ms);
}
/** @brief Select the 5 V or 10 V output range. */
int yi_gp8210s_set_range(yi_device_t *dev, yi_gp8210s_range_t range)
{
    const yi_gp8210s_config_t *cfg; uint8_t value;
    if(!gp8210s_valid(dev) || range > YI_GP8210S_RANGE_10V) return -1;
    cfg=dev->config; value = range == YI_GP8210S_RANGE_10V ? GP8210S_RANGE_10V_VALUE : 0U;
    return gp8210s_write(cfg, GP8210S_REG_RANGE, &value, 1U);
}
/** @brief Write one 15-bit channel code. */
int yi_gp8210s_write_channel(yi_device_t *dev, uint8_t channel, uint16_t value)
{
    const yi_gp8210s_config_t *cfg; yi_gp8210s_data_t *state; uint16_t encoded;
    uint8_t data[2];
    if(!gp8210s_valid(dev) || channel >= 2U || value > YI_GP8210S_MAX_CODE) return -1;
    cfg=dev->config; state=dev->data; encoded=(uint16_t)(value << 1);
    data[0]=(uint8_t)encoded; data[1]=(uint8_t)(encoded>>8);
    if(gp8210s_write(cfg, channel == 0U ? GP8210S_REG_CHANNEL0 : GP8210S_REG_CHANNEL1,
                      data, 2U) != 0) { state->error_count++; return -1; }
    state->value[channel]=value; state->write_count++; return 0;
}
/** @brief Adapt the default channel to the generic DAC API. */
static int gp8210s_dac_write(yi_device_t *dev, uint16_t value)
{ const yi_gp8210s_config_t *cfg;
  if(!gp8210s_valid(dev)) return -1; cfg=dev->config;
  return yi_gp8210s_write_channel(dev, cfg->default_channel, value); }
/** @brief Convert millivolts and write one channel. */
int yi_gp8210s_write_channel_mv(yi_device_t *dev, uint8_t channel, uint16_t mv)
{
    const yi_gp8210s_config_t *cfg; uint32_t full_scale, code;
    if(!gp8210s_valid(dev) || channel >= 2U) return -1; cfg=dev->config;
    full_scale = cfg->range == YI_GP8210S_RANGE_10V ? 10000U : 5000U;
    if(mv > full_scale) return -1;
    code=((uint32_t)mv*YI_GP8210S_MAX_CODE+full_scale/2U)/full_scale;
    return yi_gp8210s_write_channel(dev, channel, (uint16_t)code);
}
/** @brief Update both channels in one I2C transaction. */
int yi_gp8210s_write_all(yi_device_t *dev, uint16_t channel0, uint16_t channel1)
{
    const yi_gp8210s_config_t *cfg; yi_gp8210s_data_t *state; uint16_t a,b; uint8_t data[4];
    if(!gp8210s_valid(dev) || channel0>32767U || channel1>32767U) return -1;
    cfg=dev->config; state=dev->data; a=(uint16_t)(channel0<<1); b=(uint16_t)(channel1<<1);
    data[0]=(uint8_t)a; data[1]=(uint8_t)(a>>8); data[2]=(uint8_t)b; data[3]=(uint8_t)(b>>8);
    if(gp8210s_write(cfg, GP8210S_REG_CHANNEL0, data, 4U)!=0) return -1;
    state->value[0]=channel0; state->value[1]=channel1; state->write_count++; return 0;
}
/** @brief Initialize range and default channel values. */
int yi_gp8210s_init(const void *config)
{
    const yi_gp8210s_config_t *cfg=config; yi_gp8210s_data_t *data;
    if(cfg==NULL || cfg->self==NULL || cfg->self->data==NULL ||
       !yi_device_is_ready(cfg->i2c) || cfg->address>0x7FU || cfg->default_channel>=2U ||
       cfg->range>YI_GP8210S_RANGE_10V || cfg->default_value[0]>32767U ||
       cfg->default_value[1]>32767U || cfg->transfer_timeout_ms==0U) return -1;
    data=cfg->self->data; memset(data,0,sizeof(*data));
    if(yi_gp8210s_set_range(cfg->self,cfg->range)!=0) return -1;
    return yi_gp8210s_write_all(cfg->self,cfg->default_value[0],cfg->default_value[1]);
}
const yi_dac_api_t yi_gp8210s_api={.write=gp8210s_dac_write};
