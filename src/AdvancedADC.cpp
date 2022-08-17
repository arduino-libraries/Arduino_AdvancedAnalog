#include "Arduino.h"
#include "AdvancedADC.h"

struct adc_descr_t {
    ADC_HandleTypeDef adc;
    DMA_HandleTypeDef dma;
    IRQn_Type dma_irqn;
    TIM_HandleTypeDef tim;
    uint32_t  tim_trig;
    adc_callback_t callback;
    DMABufPool<sample_t> *pool;
    DMABuffer<sample_t>  *dmabuf;
};

static adc_descr_t adc_descr_all[3] = {
    {{ADC1}, {DMA1_Stream1, {DMA_REQUEST_ADC1}}, DMA1_Stream1_IRQn, {TIM1}, ADC_EXTERNALTRIG_T1_TRGO},
    {{ADC2}, {DMA1_Stream2, {DMA_REQUEST_ADC2}}, DMA1_Stream2_IRQn, {TIM2}, ADC_EXTERNALTRIG_T2_TRGO},
    {{ADC3}, {DMA1_Stream3, {DMA_REQUEST_ADC3}}, DMA1_Stream3_IRQn, {TIM3}, ADC_EXTERNALTRIG_T3_TRGO},
};

static uint32_t ADC_RANK_LUT[] = {
    ADC_REGULAR_RANK_1, ADC_REGULAR_RANK_2, ADC_REGULAR_RANK_3, ADC_REGULAR_RANK_4, ADC_REGULAR_RANK_5
};

static uint32_t ADC_RES_LUT[] = {
    ADC_RESOLUTION_8B, ADC_RESOLUTION_10B, ADC_RESOLUTION_12B, ADC_RESOLUTION_14B, ADC_RESOLUTION_16B,
};

#define ADC_ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

extern "C" {

void DMA1_Stream1_IRQHandler() {
    HAL_DMA_IRQHandler(adc_descr_all[0].adc.DMA_Handle);
}

void DMA1_Stream2_IRQHandler() {
    HAL_DMA_IRQHandler(adc_descr_all[1].adc.DMA_Handle);
}

void DMA1_Stream3_IRQHandler() {
    HAL_DMA_IRQHandler(adc_descr_all[2].adc.DMA_Handle);
}

} // extern C

static adc_descr_t *adc_descr_get(ADC_TypeDef *adc) {
    if (adc == ADC1) {
        return &adc_descr_all[0];
    } else if (adc == ADC2) {
        return &adc_descr_all[1];
    } else if (adc == ADC3) {
        return &adc_descr_all[2];
    }
    return NULL;
}

static uint32_t hal_tim_freq(TIM_HandleTypeDef *tim) {
    // NOTE: If a APB1/2 prescaler is set, respective timers clock should
    // be doubled, however, it seems the right timer clock is not doubled.
    if (((uint32_t) tim->Instance & (~0xFFFFUL)) ==  D2_APB1PERIPH_BASE) {
        return HAL_RCC_GetPCLK1Freq() * ((RCC->D2CFGR & RCC_D2CFGR_D2PPRE1) ? 1 : 1);
    } else {
        return HAL_RCC_GetPCLK2Freq() * ((RCC->D2CFGR & RCC_D2CFGR_D2PPRE2) ? 1 : 1);
    }
}

static int hal_tim_config(TIM_HandleTypeDef *tim, uint32_t t_freq) {
    uint32_t t_div = 64000;
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
    }

    __HAL_TIM_CLEAR_FLAG(tim, TIM_FLAG_UPDATE);

    // Init, config and start the timer.
    if ((HAL_TIM_PWM_Init(tim) != HAL_OK)
    || (HAL_TIMEx_MasterConfigSynchronization(tim, &sConfig) != HAL_OK)
    || (HAL_TIM_Base_Start(tim) != HAL_OK)) {
        return -1;
    }

    return 0;
}

static int hal_dma_config(DMA_HandleTypeDef *dma, IRQn_Type irqn) {
    // Enable DMA clock
    __HAL_RCC_DMA1_CLK_ENABLE();

    // DMA Init
    dma->Init.Mode                  = DMA_DOUBLE_BUFFER_M0;
    dma->Init.Priority              = DMA_PRIORITY_LOW;
    dma->Init.Direction             = DMA_PERIPH_TO_MEMORY;
    dma->Init.FIFOMode              = DMA_FIFOMODE_ENABLE;
    dma->Init.FIFOThreshold         = DMA_FIFO_THRESHOLD_FULL;
    dma->Init.MemInc                = DMA_MINC_ENABLE;
    dma->Init.PeriphInc             = DMA_PINC_DISABLE;
    dma->Init.MemBurst              = DMA_MBURST_SINGLE;
    dma->Init.PeriphBurst           = DMA_PBURST_SINGLE;
    dma->Init.MemDataAlignment      = DMA_MDATAALIGN_HALFWORD;
    dma->Init.PeriphDataAlignment   = DMA_PDATAALIGN_HALFWORD;

    HAL_DMA_DeInit(dma);
    HAL_DMA_Init(dma);

    // NVIC configuration for DMA Input data interrupt.
    HAL_NVIC_SetPriority(irqn, 1, 0);
    HAL_NVIC_EnableIRQ(irqn);

    return 0;
}

static int hal_adc_config(ADC_HandleTypeDef *adc, uint32_t resolution, uint32_t timer_trigger, auto adc_pins) {
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
    adc->Init.NbrOfConversion          = adc_pins.size();
    adc->Init.Overrun                  = ADC_OVR_DATA_OVERWRITTEN;
    adc->Init.LeftBitShift             = ADC_LEFTBITSHIFT_NONE;
    adc->Init.OversamplingMode         = DISABLE;
    adc->Init.ExternalTrigConv         = timer_trigger;
    adc->Init.ExternalTrigConvEdge     = ADC_EXTERNALTRIGCONVEDGE_RISINGFALLING;
    adc->Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;

    HAL_ADC_Init(adc);
    HAL_ADCEx_Calibration_Start(adc, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Offset       = 0;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_8CYCLES_5;

    for (size_t rank=0; rank<adc_pins.size(); rank++) {
        uint32_t function = pinmap_function(adc_pins[rank], PinMap_ADC);
        uint32_t channel = STM_PIN_CHANNEL(function);
        sConfig.Rank     = ADC_RANK_LUT[rank];
        sConfig.Channel  = __HAL_ADC_DECIMAL_NB_TO_CHANNEL(channel);
        HAL_ADC_ConfigChannel(adc, &sConfig);
    }

    return 0;
}

bool AdvancedADC::available() {
    if (descr != nullptr) {
        return descr->pool->readable();
    }
    return false;
}

DMABuffer<sample_t> &AdvancedADC::read() {
    static DMABuffer<sample_t> NULLBUF;
    if (descr != nullptr) {
        return *descr->pool->dequeue();
    } else {
        return NULLBUF;
    }
}

int AdvancedADC::begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers, adc_callback_t cb) {
    size_t n_channels = adc_pins.size();
    ADCName instance = (ADCName) pinmap_peripheral(adc_pins[0], PinMap_ADC);

    // Max of 5 channels can be read in sequence.
    if (n_channels > ADC_ARRAY_SIZE(ADC_RANK_LUT)) {
        return 0;
    }

    if (resolution >= ADC_ARRAY_SIZE(ADC_RES_LUT)) {
        return 0;
    }

    // Check if this ADC/descriptor is already in use.
    descr = adc_descr_get((ADC_TypeDef *) instance);
    if (descr->pool != nullptr) {
        descr = nullptr;
        return 0;
    }

    // All channels must share the same instance; if not, bail out
    for (auto &pin : adc_pins) {
        auto _instance = pinmap_peripheral(pin, PinMap_ADC);
        if (_instance != instance) {
            return 0;
        }
        // Configure GPIO as analog
        pinmap_pinout(pin, PinMap_ADC);
    }

    // Allocate DMA buffer pool.
    descr->pool = new DMABufPool<sample_t>(n_samples * n_channels, n_buffers);
    if (descr->pool == nullptr) {
        return 0;
    }
    descr->callback = cb;
    descr->dmabuf = descr->pool->allocate();

    // Init and config DMA.
    hal_dma_config(&descr->dma, descr->dma_irqn);

    // Init and config ADC.
    hal_adc_config(&descr->adc, ADC_RES_LUT[resolution], descr->tim_trig, adc_pins);

    // Link DMA handle to ADC handle, and start the ADC.
    __HAL_LINKDMA(&descr->adc, DMA_Handle, descr->dma);
    HAL_ADC_Start_DMA(&descr->adc, (uint32_t *) descr->dmabuf->data(), descr->dmabuf->size());

    // NOTE: This is a workaround for the ADC HAL driver lacking a function to start DMA in
    // double/multi buffer mode. The normal HAL_DMA_Start function clears the double buffer
    // bit, so we disable the stream, re-set the DMB bit, and re-enable the stream. This is
    // safe to do, since the timer that triggers the ADC is Not started yet.
    __HAL_DMA_DISABLE(&descr->dma);
    ((DMA_Stream_TypeDef *) descr->dma.Instance)->CR |= DMA_SxCR_DBM;
    __HAL_DMA_ENABLE(&descr->dma);

    // Init, config and start the ADC timer.
    hal_tim_config(&descr->tim, sample_rate);

    return 1;
}

AdvancedADC::~AdvancedADC()
{
    stop();
    if (descr) {
        if (descr->pool) {
            delete descr->pool;
        }
        descr->pool = nullptr;
        descr->dmabuf= nullptr;
        descr->callback = nullptr;
    }
}

int AdvancedADC::stop()
{
    if (descr) {
        HAL_TIM_Base_Stop(&descr->tim);
        HAL_ADC_Stop_DMA(&descr->adc);
    }
    return 1;
}

extern "C" {
//TODO: Use HAL_ADC_RegisterCallback
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc) {
    adc_descr_t *descr = adc_descr_get(adc->Instance);

    // Timestamp the buffer. TODO: Should move to timer IRQ.
    descr->dmabuf->timestamp(HAL_GetTick());

    if (descr->pool->writable()) {
        // Make sure any cached data is discarded.
        descr->dmabuf->invalidate();

        // Move current DMA buffer to ready queue.
        descr->pool->enqueue(descr->dmabuf);

        // Allocate a new free buffer.
        descr->dmabuf = descr->pool->allocate();
    }

    // Update the next DMA target pointer. Note if the pool was empty, the same buffer is reused.
    if ((((DMA_Stream_TypeDef *) adc->DMA_Handle->Instance)->CR & DMA_SxCR_CT) == 0U) {
        HAL_DMAEx_ChangeMemory(adc->DMA_Handle, (uint32_t) descr->dmabuf->data(), MEMORY1);
    } else {
        HAL_DMAEx_ChangeMemory(adc->DMA_Handle, (uint32_t) descr->dmabuf->data(), MEMORY0);
    }
}

} // extern C
