#include "HALConfig.h"

static uint32_t hal_tim_freq(TIM_HandleTypeDef *tim) {
    // NOTE: If a APB1/2 prescaler is set, respective timers clock should
    // be doubled, however, it seems the right timer clock is not doubled.
    if (((uint32_t) tim->Instance & (~0xFFFFUL)) ==  D2_APB1PERIPH_BASE) {
        return HAL_RCC_GetPCLK1Freq() * ((RCC->D2CFGR & RCC_D2CFGR_D2PPRE1) ? 1 : 1);
    } else {
        return HAL_RCC_GetPCLK2Freq() * ((RCC->D2CFGR & RCC_D2CFGR_D2PPRE2) ? 1 : 1);
    }
}

int hal_tim_config(TIM_HandleTypeDef *tim, uint32_t t_freq) {
    uint32_t t_div = 64000; // TODO: This divider will only allow frequencies up to 64KHz
    uint32_t t_clk = hal_tim_freq(tim);

    tim->Init.Period             = (t_div / t_freq) - 1;
    tim->Init.Prescaler          = (t_clk / t_div ) - 1;
    tim->Init.CounterMode        = TIM_COUNTERMODE_UP;
    tim->Init.ClockDivision      = TIM_CLOCKDIVISION_DIV1;
    tim->Init.RepetitionCounter  = 0;
    tim->Init.AutoReloadPreload  = TIM_AUTORELOAD_PRELOAD_ENABLE;

    TIM_MasterConfigTypeDef sConfig = {0};
    sConfig.MasterOutputTrigger  = TIM_TRGO_UPDATE;
    sConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    sConfig.MasterSlaveMode      = TIM_MASTERSLAVEMODE_ENABLE;

    if (tim->Instance == TIM1) {
        __HAL_RCC_TIM1_CLK_ENABLE();
    } else if (tim->Instance == TIM2) {
        __HAL_RCC_TIM2_CLK_ENABLE();
    } else if (tim->Instance == TIM3) {
        __HAL_RCC_TIM3_CLK_ENABLE();
    } else if (tim->Instance == TIM4) {
        __HAL_RCC_TIM4_CLK_ENABLE();
    } else if (tim->Instance == TIM5) {
        __HAL_RCC_TIM5_CLK_ENABLE();
    }

    __HAL_TIM_CLEAR_FLAG(tim, TIM_FLAG_UPDATE);

    // Init and config the timer.
    if ((HAL_TIM_PWM_Init(tim) != HAL_OK)
    || (HAL_TIMEx_MasterConfigSynchronization(tim, &sConfig) != HAL_OK)) {
        return -1;
    }

    return 0;
}

int hal_dma_config(DMA_HandleTypeDef *dma, IRQn_Type irqn, uint32_t direction) {
    // Enable DMA clock
    __HAL_RCC_DMA1_CLK_ENABLE();

    // DMA Init
    dma->Init.Mode                  = DMA_DOUBLE_BUFFER_M0;
    dma->Init.Priority              = DMA_PRIORITY_LOW;
    dma->Init.Direction             = direction;
    dma->Init.FIFOMode              = DMA_FIFOMODE_ENABLE;
    dma->Init.FIFOThreshold         = DMA_FIFO_THRESHOLD_FULL;
    dma->Init.MemInc                = DMA_MINC_ENABLE;
    dma->Init.PeriphInc             = DMA_PINC_DISABLE;
    dma->Init.MemBurst              = DMA_MBURST_SINGLE;
    dma->Init.PeriphBurst           = DMA_PBURST_SINGLE;
    dma->Init.MemDataAlignment      = DMA_MDATAALIGN_HALFWORD;
    dma->Init.PeriphDataAlignment   = DMA_PDATAALIGN_HALFWORD;

    if (HAL_DMA_DeInit(dma) != HAL_OK
     || HAL_DMA_Init(dma) != HAL_OK) {
        return -1;
    }

    // NVIC configuration for DMA Input data interrupt.
    HAL_NVIC_SetPriority(irqn, 1, 0);
    HAL_NVIC_EnableIRQ(irqn);

    return 0;
}

void hal_dma_enable_dbm(DMA_HandleTypeDef *dma, void *m0, void *m1)
{
    // NOTE: This is a workaround for the ADC/DAC HAL driver lacking a function to start DMA
    // in double/multi buffer mode. The HAL_x_DMA_Start function clears the double buffer bit,
    // so we disable the stream, re-set the DMB bit, and re-enable the stream. This should be
    // safe to do, assuming the ADC/DAC trigger timer is Not running.
    __HAL_DMA_DISABLE(dma);

    // Clear all interrupt flags
    volatile uint32_t *ifc;
    ifc = (uint32_t *)((uint32_t)(dma->StreamBaseAddress + 8U));
    *ifc = 0x3FUL << (dma->StreamIndex & 0x1FU);

    // Set double buffer mode bit.
    ((DMA_Stream_TypeDef *) dma->Instance)->CR  |= DMA_SxCR_DBM;

    // Set M0, and M1 if provided.
    ((DMA_Stream_TypeDef *) dma->Instance)->M0AR = (uint32_t) m0;
    ((DMA_Stream_TypeDef *) dma->Instance)->M1AR = (uint32_t) m1;

    // Set the second buffer transfer complete callback.
    dma->XferM1CpltCallback = dma->XferCpltCallback;

    __HAL_DMA_ENABLE(dma);
}

size_t hal_dma_get_ct(DMA_HandleTypeDef *dma)
{
    // Returns 0 if CT==0, and 1 if CT==1.
    return !!(((DMA_Stream_TypeDef *) dma->Instance)->CR & DMA_SxCR_CT);
}

void hal_dma_update_memory(DMA_HandleTypeDef *dma, void *addr)
{
    // Update the next DMA target pointer.
    if (((DMA_Stream_TypeDef *) dma->Instance)->CR & DMA_SxCR_CT) {
        HAL_DMAEx_ChangeMemory(dma, (uint32_t) addr, MEMORY0);
    } else {
        HAL_DMAEx_ChangeMemory(dma, (uint32_t) addr, MEMORY1);
    }
}
