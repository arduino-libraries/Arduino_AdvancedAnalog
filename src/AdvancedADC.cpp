#include "Arduino.h"
#include "HALConfig.h"
#include "AdvancedADC.h"

struct adc_descr_t {
    ADC_HandleTypeDef adc;
    DMA_HandleTypeDef dma;
    IRQn_Type dma_irqn;
    TIM_HandleTypeDef tim;
    uint32_t  tim_trig;
    user_callback_t callback;
    DMABuffer<Sample> *dmabuf;
    DMABufferPool<Sample> *pool;
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

DMABuffer<Sample> &AdvancedADC::read() {
    static DMABuffer<Sample> NULLBUF;
    if (descr != nullptr) {
        return *descr->pool->dequeue();
    }
    return NULLBUF;
}

int AdvancedADC::begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers, user_callback_t callback) {
    size_t n_channels = adc_pins.size();
    ADCName instance = (ADCName) pinmap_peripheral(adc_pins[0], PinMap_ADC);

    // Sanity checks.
    if (resolution >= AN_ARRAY_SIZE(ADC_RES_LUT)) {
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
    descr->pool = new DMABufferPool<Sample>(n_samples, n_channels, n_buffers);
    if (descr->pool == nullptr) {
        return 0;
    }
    descr->callback = callback;
    descr->dmabuf = descr->pool->allocate();

    // Init and config DMA.
    hal_dma_config(&descr->dma, descr->dma_irqn, DMA_PERIPH_TO_MEMORY);

    // Init and config ADC.
    hal_adc_config(&descr->adc, ADC_RES_LUT[resolution], descr->tim_trig, adc_pins);

    // Link DMA handle to ADC handle, and start the ADC.
    __HAL_LINKDMA(&descr->adc, DMA_Handle, descr->dma);
    HAL_ADC_Start_DMA(&descr->adc, (uint32_t *) descr->dmabuf->data(), descr->dmabuf->size());

    // Re/enable DMA double buffer mode.
    hal_dma_enable_dbm(&descr->dma);

    // Init, config and start the ADC timer.
    hal_tim_config(&descr->tim, sample_rate);
    HAL_TIM_Base_Start(&descr->tim);
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
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc) {
    adc_descr_t *descr = adc_descr_get(adc->Instance);

    // Timestamp the buffer.
    // TODO: Should move to timer IRQ.
    descr->dmabuf->timestamp(HAL_GetTick());

    // Currently, all buffers are interleaved.
    descr->dmabuf->setflags(DMA_BUFFER_INTRLVD);

    if (descr->pool->writable()) {
        // Make sure any cached data is discarded.
        descr->dmabuf->invalidate();

        // Move current DMA buffer to ready queue.
        descr->pool->enqueue(descr->dmabuf);

        // Allocate a new free buffer.
        descr->dmabuf = descr->pool->allocate();
    } else {
        descr->dmabuf->setflags(DMA_BUFFER_DISCONT);
    }

    // Update the next DMA target pointer.
    // NOTE: If the pool was empty, the same buffer is reused.
    hal_dma_swap_memory(&descr->dma, descr->dmabuf->data());
}

} // extern C
