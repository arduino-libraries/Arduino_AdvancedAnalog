#include "Arduino.h"
#include "AdvancedADC.h"

struct adc_descr_t {
    DMABuffer<ADCSample>  *dmabuf;
    DMABufPool<ADCSample> *pool;
    IRQn_Type dma_irqn;
    ADC_HandleTypeDef adc;
    DMA_HandleTypeDef dma;
    adc_callback_t callback;
    adc_descr_t(auto irqn, auto stream, auto request) {
        dma_irqn = irqn;
        dma.Instance = stream;
        dma.Init.Request = request;
    }
};

static uint32_t ADC_INDEX_TO_RANK[] = {
    ADC_REGULAR_RANK_1,
    ADC_REGULAR_RANK_2,
    ADC_REGULAR_RANK_3,
    ADC_REGULAR_RANK_4,
    ADC_REGULAR_RANK_5
};

static adc_descr_t adc_descr_all[3] = {
    adc_descr_t(DMA1_Stream1_IRQn, DMA1_Stream1, DMA_REQUEST_ADC1),
    adc_descr_t(DMA1_Stream2_IRQn, DMA1_Stream2, DMA_REQUEST_ADC2),
    adc_descr_t(DMA1_Stream3_IRQn, DMA1_Stream3, DMA_REQUEST_ADC3),
};

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

bool AdvancedADC::available() {
    if (adc_descr != nullptr) {
        return adc_descr->pool->readable();
    }
    return false;
}

DMABuffer<ADCSample> &AdvancedADC::dequeue() {
    static DMABuffer<ADCSample> NULLBUF;
    if (adc_descr != nullptr) {
        return *adc_descr->pool->dequeue();
    } else {
        return NULLBUF;
    }
}

static int adc_tim_config(uint32_t frequency) {
    static TIM_HandleTypeDef htim;
    TIM_MasterConfigTypeDef sConfig = {0};

    uint32_t tclock = HAL_RCC_GetPCLK2Freq(); // This should be * 2
    uint32_t presc  = tclock / 1000000;
    uint32_t period = tclock / presc / frequency;

    htim.Instance                = TIM1;
    htim.Init.Period             = period - 1;
    htim.Init.Prescaler          = presc - 1;
    htim.Init.CounterMode        = TIM_COUNTERMODE_UP;
    htim.Init.ClockDivision      = TIM_CLOCKDIVISION_DIV1;
    htim.Init.RepetitionCounter  = 0;
    htim.Init.AutoReloadPreload  = TIM_AUTORELOAD_PRELOAD_ENABLE;

    sConfig.MasterOutputTrigger  = TIM_TRGO_UPDATE;
    sConfig.MasterOutputTrigger2 = TIM_TRGO2_RESET;
    sConfig.MasterSlaveMode      = TIM_MASTERSLAVEMODE_DISABLE;

    __HAL_RCC_TIM1_CLK_ENABLE();

    __HAL_TIM_CLEAR_FLAG(&htim, TIM_FLAG_UPDATE);

    // Init, config and start the timer.
    if ((HAL_TIM_PWM_Init(&htim) != HAL_OK)
    || (HAL_TIMEx_MasterConfigSynchronization(&htim, &sConfig) != HAL_OK)
    || (HAL_TIM_Base_Start(&htim) != HAL_OK)) {
        return -1;
    }
    return 0;
}

int AdvancedADC::begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers, adc_callback_t cb) {
    size_t n_channels = adc_pins.size();

    // Max of 5 channels can be read in sequence.
    if (n_channels > 5 ) {
        return -1;
    }

    // All channels must share the same instance; if not, bail out
    ADCName instance = (ADCName) pinmap_peripheral(adc_pins[0], PinMap_ADC);
    for (auto &pin : adc_pins) {
        auto _instance = pinmap_peripheral(pin, PinMap_ADC);
        if (_instance != instance) {
            return -1;
        }
        // Configure GPIO as analog
        pinmap_pinout(pin, PinMap_ADC);
    }

    adc_descr = adc_descr_get((ADC_TypeDef *) instance);
    adc_descr->callback = cb;

    // Set ADC clock source.
    __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);

    // Enable ADC clock
    if (instance == ADC_1) {
        __HAL_RCC_ADC12_CLK_ENABLE();
    } else if (instance == ADC_2) {
        __HAL_RCC_ADC12_CLK_ENABLE();
    } else if (instance == ADC_3) {
        __HAL_RCC_ADC3_CLK_ENABLE();
    }

    // ADC init
    adc_descr->adc.Instance                   = (ADC_TypeDef *) instance;
    adc_descr->adc.Init.ClockPrescaler        = ADC_CLOCK_ASYNC_DIV1;
    adc_descr->adc.Init.Resolution            = ADC_RESOLUTION_12B; // TODO fix
    adc_descr->adc.Init.ScanConvMode          = ADC_SCAN_ENABLE;
    adc_descr->adc.Init.EOCSelection          = ADC_EOC_SEQ_CONV;
    adc_descr->adc.Init.LowPowerAutoWait      = DISABLE;
    adc_descr->adc.Init.ContinuousConvMode    = DISABLE;
    adc_descr->adc.Init.DiscontinuousConvMode = DISABLE;
    adc_descr->adc.Init.NbrOfConversion       = n_channels;
    adc_descr->adc.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    adc_descr->adc.Init.LeftBitShift          = ADC_LEFTBITSHIFT_NONE;
    adc_descr->adc.Init.OversamplingMode      = DISABLE;
    adc_descr->adc.Init.ExternalTrigConv      = ADC_EXTERNALTRIG_T1_TRGO;
    adc_descr->adc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_RISING;
    adc_descr->adc.Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;

    HAL_ADC_Init(&adc_descr->adc);
    HAL_ADCEx_Calibration_Start(&adc_descr->adc, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);

    if (instance == ADC_1) {
        ADC_MultiModeTypeDef multimode = {0};
        multimode.Mode = ADC_MODE_INDEPENDENT;
        HAL_ADCEx_MultiModeConfigChannel(&adc_descr->adc, &multimode);
    }

    ADC_ChannelConfTypeDef sConfig;
    sConfig.Offset       = 0;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.SingleDiff   = ADC_SINGLE_ENDED;
    sConfig.SamplingTime = ADC_SAMPLETIME_8CYCLES_5;

    int rank = 0;
    for (auto &pin : adc_pins) {
        uint32_t function = pinmap_function(pin, PinMap_ADC);
        uint32_t channel  = STM_PIN_CHANNEL(function);
        sConfig.Rank      = ADC_INDEX_TO_RANK[rank++];
        sConfig.Channel   = __HAL_ADC_DECIMAL_NB_TO_CHANNEL(channel);
        HAL_ADC_ConfigChannel(&adc_descr->adc, &sConfig);
    }

    // Enable DMA clock
    __HAL_RCC_DMA1_CLK_ENABLE();

    // DMA Init
    adc_descr->dma.Init.Mode                  = DMA_DOUBLE_BUFFER_M0;
    adc_descr->dma.Init.Priority              = DMA_PRIORITY_LOW;
    adc_descr->dma.Init.Direction             = DMA_PERIPH_TO_MEMORY;
    adc_descr->dma.Init.FIFOMode              = DMA_FIFOMODE_ENABLE;
    adc_descr->dma.Init.FIFOThreshold         = DMA_FIFO_THRESHOLD_FULL;
    adc_descr->dma.Init.MemInc                = DMA_MINC_ENABLE;
    adc_descr->dma.Init.PeriphInc             = DMA_PINC_DISABLE;
    adc_descr->dma.Init.MemBurst              = DMA_MBURST_SINGLE;
    adc_descr->dma.Init.PeriphBurst           = DMA_PBURST_SINGLE;
    adc_descr->dma.Init.MemDataAlignment      = DMA_MDATAALIGN_HALFWORD;
    adc_descr->dma.Init.PeriphDataAlignment   = DMA_PDATAALIGN_HALFWORD;

    HAL_DMA_DeInit(&adc_descr->dma);
    HAL_DMA_Init(&adc_descr->dma);

    // Associate the DMA handle
    __HAL_LINKDMA(&adc_descr->adc, DMA_Handle, adc_descr->dma);

    // NVIC configuration for DMA Input data interrupt.
    HAL_NVIC_SetPriority(adc_descr->dma_irqn, 1, 0);
    HAL_NVIC_EnableIRQ(adc_descr->dma_irqn);

    adc_descr->pool = new DMABufPool<ADCSample>(n_samples * n_channels, n_buffers);
    adc_descr->dmabuf = adc_descr->pool->allocate();
    HAL_ADC_Start_DMA(&adc_descr->adc, (uint32_t *) adc_descr->dmabuf->data(), adc_descr->dmabuf->size());

    __HAL_DMA_DISABLE(&adc_descr->dma);
    ((DMA_Stream_TypeDef *) adc_descr->dma.Instance)->CR |= DMA_SxCR_DBM;
    __HAL_DMA_ENABLE(&adc_descr->dma);

    adc_tim_config(sample_rate);
    return 1;
}

int AdvancedADC::stop()
{
    return 1;
}

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

//TODO: Use HAL_ADC_RegisterCallback
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc) {
    adc_descr_t *adc_descr = adc_descr_get(adc->Instance);

    // Timestamp the buffer. TODO: Should move to timer IRQ.
    adc_descr->dmabuf->timestamp(HAL_GetTick());

    if (adc_descr->pool->writable()) {
        // Make sure any cached data is discarded.
        adc_descr->dmabuf->flush();

        // Move current DMA buffer to ready queue.
        adc_descr->pool->enqueue(adc_descr->dmabuf);

        // Allocate a new free buffer.
        adc_descr->dmabuf = adc_descr->pool->allocate();
    }

    // Update the next DMA target pointer. Note if the pool was empty, the same buffer is reused.
    if ((((DMA_Stream_TypeDef *) adc->DMA_Handle->Instance)->CR & DMA_SxCR_CT) == 0U) {
        HAL_DMAEx_ChangeMemory(adc->DMA_Handle, (uint32_t) adc_descr->dmabuf->data(), MEMORY1);
    } else {
        HAL_DMAEx_ChangeMemory(adc->DMA_Handle, (uint32_t) adc_descr->dmabuf->data(), MEMORY0);
    }
}

} //extern C
