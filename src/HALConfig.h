#ifndef __HAL_CONFIG_H__
#define __HAL_CONFIG_H__
#include "Arduino.h"
#include "AdvancedAnalog.h"
int hal_tim_config(TIM_HandleTypeDef *tim, uint32_t t_freq);
int hal_dma_config(DMA_HandleTypeDef *dma, IRQn_Type irqn, uint32_t direction);
size_t hal_dma_get_ct(DMA_HandleTypeDef *dma);
void hal_dma_enable_dbm(DMA_HandleTypeDef *dma, void *m0 = nullptr, void *m1 = nullptr);
void hal_dma_update_memory(DMA_HandleTypeDef *dma, void *addr);
int hal_dac_config(DAC_HandleTypeDef *dac, uint32_t channel, uint32_t trigger);
int hal_adc_config(ADC_HandleTypeDef *adc, uint32_t resolution, uint32_t trigger, PinName *adc_pins, uint32_t n_channels);
#endif  // __HAL_CONFIG_H__
