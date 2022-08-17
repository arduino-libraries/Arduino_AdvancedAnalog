#ifndef __HAL_CONFIG_H__
#define __HAL_CONFIG_H__
#include "Arduino.h"
#include "AdvancedAnalog.h"
int hal_tim_config(TIM_HandleTypeDef *tim, uint32_t t_freq);
int hal_dma_config(DMA_HandleTypeDef *dma, IRQn_Type irqn, uint32_t direction);
void hal_dma_enable_dbm(DMA_HandleTypeDef *dma);
void hal_dma_swap_memory(DMA_HandleTypeDef *dma, void *addr);
#endif  // __HAL_CONFIG_H__
