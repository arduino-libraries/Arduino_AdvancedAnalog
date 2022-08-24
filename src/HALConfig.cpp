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
    __HAL_DMA_DISABLE_IT(dma, (DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_HT));
    __HAL_DMA_DISABLE(dma);
    ((DMA_Stream_TypeDef *) dma->Instance)->CR  |= DMA_SxCR_DBM;
    ((DMA_Stream_TypeDef *) dma->Instance)->M0AR = (uint32_t) m0;
    ((DMA_Stream_TypeDef *) dma->Instance)->M1AR = (uint32_t) m1;
    __HAL_DMA_ENABLE_IT(dma, (DMA_IT_TC | DMA_IT_TE | DMA_IT_DME | DMA_IT_HT));
    __HAL_DMA_ENABLE(dma);
}

size_t hal_dma_get_ct(DMA_HandleTypeDef *dma)
{
    // Returns 0 if CT==0, and 1 if CT==1.
    return !!(((DMA_Stream_TypeDef *) dma->Instance)->CR & DMA_SxCR_CT);
}

void hal_dma_swap_memory(DMA_HandleTypeDef *dma, void *addr)
{
    // Update the next DMA target pointer.
    if (((DMA_Stream_TypeDef *) dma->Instance)->CR & DMA_SxCR_CT) {
        HAL_DMAEx_ChangeMemory(dma, (uint32_t) addr, MEMORY0);
    } else {
        HAL_DMAEx_ChangeMemory(dma, (uint32_t) addr, MEMORY1);
    }
}

// Copy of DAC_Start_DMA modified to start multibuffers.
HAL_StatusTypeDef HAL_DAC_Start_DMA_MB(DAC_HandleTypeDef *hdac, uint32_t Channel,
        uint32_t *pData, uint32_t *pData2, uint32_t Length, uint32_t Alignment)
{
    HAL_StatusTypeDef status;
    uint32_t tmpreg = 0U;

    /* Check the parameters */
    assert_param(IS_DAC_CHANNEL(Channel));
    assert_param(IS_DAC_ALIGN(Alignment));

    /* Process locked */
    __HAL_LOCK(hdac);

    /* Change DAC state */
    hdac->State = HAL_DAC_STATE_BUSY;

    if (Channel == DAC_CHANNEL_1) {
        /* Set the DMA transfer complete callback for channel1 */
        hdac->DMA_Handle1->XferCpltCallback = DAC_DMAConvCpltCh1;

        /* Set the DMA half transfer complete callback for channel1 */
        hdac->DMA_Handle1->XferHalfCpltCallback = DAC_DMAHalfConvCpltCh1;

        /* Set the DMA error callback for channel1 */
        hdac->DMA_Handle1->XferErrorCallback = DAC_DMAErrorCh1;

        /* Enable the selected DAC channel1 DMA request */
        SET_BIT(hdac->Instance->CR, DAC_CR_DMAEN1);

        /* Case of use of channel 1 */
        switch (Alignment) {
            case DAC_ALIGN_12B_R:
                /* Get DHR12R1 address */
                tmpreg = (uint32_t)&hdac->Instance->DHR12R1;
                break;
            case DAC_ALIGN_12B_L:
                /* Get DHR12L1 address */
                tmpreg = (uint32_t)&hdac->Instance->DHR12L1;
                break;
            case DAC_ALIGN_8B_R:
                /* Get DHR8R1 address */
                tmpreg = (uint32_t)&hdac->Instance->DHR8R1;
                break;
            default:
                break;
        }
    } else {
        /* Set the DMA transfer complete callback for channel2 */
        hdac->DMA_Handle2->XferCpltCallback = DAC_DMAConvCpltCh2;

        /* Set the DMA half transfer complete callback for channel2 */
        hdac->DMA_Handle2->XferHalfCpltCallback = DAC_DMAHalfConvCpltCh2;

        /* Set the DMA error callback for channel2 */
        hdac->DMA_Handle2->XferErrorCallback = DAC_DMAErrorCh2;

        /* Enable the selected DAC channel2 DMA request */
        SET_BIT(hdac->Instance->CR, DAC_CR_DMAEN2);

        /* Case of use of channel 2 */
        switch (Alignment) {
            case DAC_ALIGN_12B_R:
                /* Get DHR12R2 address */
                tmpreg = (uint32_t)&hdac->Instance->DHR12R2;
                break;
            case DAC_ALIGN_12B_L:
                /* Get DHR12L2 address */
                tmpreg = (uint32_t)&hdac->Instance->DHR12L2;
                break;
            case DAC_ALIGN_8B_R:
                /* Get DHR8R2 address */
                tmpreg = (uint32_t)&hdac->Instance->DHR8R2;
                break;
            default:
                break;
        }
    }

    /* Enable the DMA Stream */
    if (Channel == DAC_CHANNEL_1) {
        /* Enable the DAC DMA underrun interrupt */
        __HAL_DAC_ENABLE_IT(hdac, DAC_IT_DMAUDR1);

        /* Enable the DMA Stream */
        status = HAL_DMAEx_MultiBufferStart_IT(hdac->DMA_Handle1, (uint32_t)pData, tmpreg, (uint32_t)pData2, Length);
    } else {
        /* Enable the DAC DMA underrun interrupt */
        __HAL_DAC_ENABLE_IT(hdac, DAC_IT_DMAUDR2);

        /* Enable the DMA Stream */
        status = HAL_DMAEx_MultiBufferStart_IT(hdac->DMA_Handle2, (uint32_t)pData, tmpreg, (uint32_t)pData2, Length);
    }

    /* Process Unlocked */
    __HAL_UNLOCK(hdac);

    if (status == HAL_OK) {
        /* Enable the Peripheral */
        __HAL_DAC_ENABLE(hdac, Channel);
    } else {
        hdac->ErrorCode |= HAL_DAC_ERROR_DMA;
    }

    /* Return function status */
    return status;
}
