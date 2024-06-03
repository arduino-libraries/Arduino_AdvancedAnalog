/*
  This file is part of the Arduino_AdvancedAnalog library.
  Copyright (c) 2023 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "HALConfig.h"

static uint32_t hal_tim_freq(TIM_HandleTypeDef *tim) {
    // NOTE: If a APB1/2 prescaler is set, respective timers clock should
    // be doubled, however, it seems the right timer clock is not doubled.
    if (((uint32_t) tim->Instance & (~0xFFFFUL)) ==  D2_APB1PERIPH_BASE) {
        return HAL_RCC_GetPCLK1Freq() * ((RCC->D2CFGR & RCC_D2CFGR_D2PPRE1) ? 2 : 1);
    } else {
        return HAL_RCC_GetPCLK2Freq() * ((RCC->D2CFGR & RCC_D2CFGR_D2PPRE2) ? 2 : 1);
    }
}

int hal_tim_config(TIM_HandleTypeDef *tim, uint32_t t_freq) {
    uint32_t t_clk = hal_tim_freq(tim);
    uint32_t t_div = ((t_clk / t_freq) > 0xFFFF) ? 64000 : (t_freq * 2);

    tim->Init.Period                = (t_div / t_freq) - 1;
    tim->Init.Prescaler             = (t_clk / t_div ) - 1;
    tim->Init.CounterMode           = TIM_COUNTERMODE_UP;
    tim->Init.ClockDivision         = TIM_CLOCKDIVISION_DIV1;
    tim->Init.RepetitionCounter     = 0;
    tim->Init.AutoReloadPreload     = TIM_AUTORELOAD_PRELOAD_ENABLE;

    TIM_MasterConfigTypeDef sConfig = {0};
    sConfig.MasterOutputTrigger     = TIM_TRGO_UPDATE;
    sConfig.MasterOutputTrigger2    = TIM_TRGO2_RESET;
    sConfig.MasterSlaveMode         = TIM_MASTERSLAVEMODE_ENABLE;

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
    } else if (tim->Instance == TIM6) {
        __HAL_RCC_TIM6_CLK_ENABLE();
    }

    // Init and config the timer.
    __HAL_TIM_CLEAR_FLAG(tim, TIM_FLAG_UPDATE);
    if ((HAL_TIM_PWM_Init(tim) != HAL_OK)
    || (HAL_TIMEx_MasterConfigSynchronization(tim, &sConfig) != HAL_OK)) {
        return -1;
    }
    return 0;
}

int hal_dma_config(DMA_HandleTypeDef *dma, IRQn_Type irqn, uint32_t direction) {
    // Enable DMA clock
    __HAL_RCC_DMA1_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    // DMA Init
    dma->Init.Mode                  = DMA_DOUBLE_BUFFER_M0;
    dma->Init.Priority              = DMA_PRIORITY_VERY_HIGH;
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
    HAL_NVIC_SetPriority(irqn, 0, 0);
    HAL_NVIC_EnableIRQ(irqn);

    return 0;
}

void hal_dma_enable_dbm(DMA_HandleTypeDef *dma, void *m0, void *m1) {
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

size_t hal_dma_get_ct(DMA_HandleTypeDef *dma) {
    // Returns 0 if CT==0, and 1 if CT==1.
    return !!(((DMA_Stream_TypeDef *) dma->Instance)->CR & DMA_SxCR_CT);
}

void hal_dma_update_memory(DMA_HandleTypeDef *dma, void *addr) {
    // Update the next DMA target pointer.
    if (((DMA_Stream_TypeDef *) dma->Instance)->CR & DMA_SxCR_CT) {
        HAL_DMAEx_ChangeMemory(dma, (uint32_t) addr, MEMORY0);
    } else {
        HAL_DMAEx_ChangeMemory(dma, (uint32_t) addr, MEMORY1);
    }
}

int hal_dac_config(DAC_HandleTypeDef *dac, uint32_t channel, uint32_t trigger) {
    // DAC init
    if (dac->Instance == NULL) {
        // Enable DAC clock
        __HAL_RCC_DAC12_CLK_ENABLE();

        dac->Instance = DAC1;
        if (HAL_DAC_DeInit(dac) != HAL_OK
         || HAL_DAC_Init(dac) != HAL_OK) {
            return -1;
        }
    }

    DAC_ChannelConfTypeDef sConfig = {0};
    sConfig.DAC_Trigger         = trigger;
    sConfig.DAC_OutputBuffer    = DAC_OUTPUTBUFFER_DISABLE;
    sConfig.DAC_UserTrimming    = DAC_TRIMMING_FACTORY;
    sConfig.DAC_SampleAndHold   = DAC_SAMPLEANDHOLD_DISABLE;
    sConfig.DAC_ConnectOnChipPeripheral = DAC_CHIPCONNECT_DISABLE;
    if (HAL_DAC_ConfigChannel(dac, &sConfig, channel) != HAL_OK) {
        return -1;
    }

    return 0;
}

static uint32_t ADC_RANK_LUT[] = {
    ADC_REGULAR_RANK_1, ADC_REGULAR_RANK_2, ADC_REGULAR_RANK_3, ADC_REGULAR_RANK_4,
    ADC_REGULAR_RANK_5, ADC_REGULAR_RANK_6, ADC_REGULAR_RANK_7, ADC_REGULAR_RANK_8,
    ADC_REGULAR_RANK_9, ADC_REGULAR_RANK_10, ADC_REGULAR_RANK_11, ADC_REGULAR_RANK_12,
    ADC_REGULAR_RANK_13, ADC_REGULAR_RANK_14, ADC_REGULAR_RANK_15, ADC_REGULAR_RANK_16
};

int hal_adc_config(ADC_HandleTypeDef *adc, uint32_t resolution, uint32_t trigger,
                   PinName *adc_pins, uint32_t n_channels, uint32_t sample_time) {
    // Set ADC clock source.
    __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);

    // Enable ADC clock
    if (adc->Instance == ADC1) {
        __HAL_RCC_ADC12_CLK_ENABLE();
    } else if (adc->Instance == ADC2) {
        __HAL_RCC_ADC12_CLK_ENABLE();
    } else if (adc->Instance == ADC3) {
        __HAL_RCC_ADC3_CLK_ENABLE();
    }

    // ADC init
    adc->Init.Resolution               = resolution;
    adc->Init.ClockPrescaler           = ADC_CLOCK_ASYNC_DIV1;
    adc->Init.ScanConvMode             = ADC_SCAN_ENABLE;
    adc->Init.EOCSelection             = ADC_EOC_SEQ_CONV;
    adc->Init.LowPowerAutoWait         = DISABLE;
    adc->Init.ContinuousConvMode       = DISABLE;
    adc->Init.DiscontinuousConvMode    = DISABLE;
    adc->Init.NbrOfConversion          = n_channels;
    adc->Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;
    adc->Init.LeftBitShift             = ADC_LEFTBITSHIFT_NONE;
    adc->Init.OversamplingMode         = DISABLE;
    adc->Init.ExternalTrigConv         = trigger;
    adc->Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_RISING;
    adc->Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;

    if (HAL_ADC_Init(adc) != HAL_OK
        || HAL_ADCEx_Calibration_Start(adc, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED) != HAL_OK) {
            return -1;
    }

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Offset       = 0;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = sample_time;

    for (size_t rank=0; rank<n_channels; rank++) {
        uint32_t function = pinmap_function(adc_pins[rank], PinMap_ADC);
        uint32_t channel = STM_PIN_CHANNEL(function);
        sConfig.Rank     = ADC_RANK_LUT[rank];
        sConfig.Channel  = __HAL_ADC_DECIMAL_NB_TO_CHANNEL(channel);
        if (HAL_ADC_ConfigChannel(adc, &sConfig) != HAL_OK) {
            return -1;
        }
    }

    return 0;
}

int hal_adc_enable_dual_mode(bool enable) {
    if (enable) {
        LL_ADC_SetMultimode(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_MULTI_DUAL_REG_SIMULT);
    } else {
        LL_ADC_SetMultimode(__LL_ADC_COMMON_INSTANCE(ADC1), LL_ADC_MULTI_INDEPENDENT);
    }
    return 0;
}

int hal_i2s_config(I2S_HandleTypeDef *i2s, uint32_t sample_rate, uint32_t mode, bool mck_enable) {
    // Set I2S clock source.
    RCC_PeriphCLKInitTypeDef pclk_init = {0};
    pclk_init.PLL3.PLL3M = 16;
    pclk_init.PLL3.PLL3N = 344;
    pclk_init.PLL3.PLL3P = 7;
    pclk_init.PLL3.PLL3Q = 5;
    pclk_init.PLL3.PLL3R = 5;
    pclk_init.PLL3.PLL3FRACN = 0;
    pclk_init.PLL3.PLL3RGE = RCC_PLL3VCIRANGE_0;
    pclk_init.PLL3.PLL3VCOSEL = RCC_PLL3VCOMEDIUM;
    pclk_init.PeriphClockSelection |= RCC_PERIPHCLK_SPI123;
    pclk_init.Spi123ClockSelection  = RCC_SPI123CLKSOURCE_PLL3;

    if (HAL_RCCEx_PeriphCLKConfig(&pclk_init) != HAL_OK) {
        return -1;
    }

    // Enable I2S clock
    if (i2s->Instance == SPI1) {
        __HAL_RCC_SPI1_CLK_ENABLE();
    } else if (i2s->Instance == SPI2) {
        __HAL_RCC_SPI2_CLK_ENABLE();
    } else if (i2s->Instance == SPI3) {
        __HAL_RCC_SPI3_CLK_ENABLE();
    }

    i2s->Init.Mode = mode;
    i2s->Init.Standard = I2S_STANDARD_PHILIPS;
    i2s->Init.DataFormat = I2S_DATAFORMAT_16B_EXTENDED;
    i2s->Init.MCLKOutput = mck_enable ? I2S_MCLKOUTPUT_ENABLE : I2S_MCLKOUTPUT_DISABLE;
    i2s->Init.AudioFreq = sample_rate;
    i2s->Init.CPOL = I2S_CPOL_LOW;
    i2s->Init.FirstBit = I2S_FIRSTBIT_MSB;
    i2s->Init.WSInversion = I2S_WS_INVERSION_DISABLE;
    i2s->Init.Data24BitAlignment = I2S_DATA_24BIT_ALIGNMENT_RIGHT;
    i2s->Init.MasterKeepIOState = I2S_MASTER_KEEP_IO_STATE_DISABLE;

    HAL_I2S_DeInit(i2s);
    if (HAL_I2S_Init(i2s) != HAL_OK) {
        return -1;
    }
    return 0;
}
