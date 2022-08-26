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
    DMABufferPool<Sample> *pool;
    DMABuffer<Sample> *dmabuf[2];
};

static adc_descr_t adc_descr_all[3] = {
    {{ADC1}, {DMA1_Stream1, {DMA_REQUEST_ADC1}}, DMA1_Stream1_IRQn, {TIM1}, ADC_EXTERNALTRIG_T1_TRGO},
    {{ADC2}, {DMA1_Stream2, {DMA_REQUEST_ADC2}}, DMA1_Stream2_IRQn, {TIM2}, ADC_EXTERNALTRIG_T2_TRGO},
    {{ADC3}, {DMA1_Stream3, {DMA_REQUEST_ADC3}}, DMA1_Stream3_IRQn, {TIM3}, ADC_EXTERNALTRIG_T3_TRGO},
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
    for (size_t i=0; i<n_channels; i++) {
        auto _instance = pinmap_peripheral(adc_pins[i], PinMap_ADC);
        if (_instance != instance) {
            return 0;
        }
        // Configure GPIO as analog
        pinmap_pinout(adc_pins[i], PinMap_ADC);
    }

    // Allocate DMA buffer pool.
    descr->pool = new DMABufferPool<Sample>(n_samples, n_channels, n_buffers);
    if (descr->pool == nullptr) {
        return 0;
    }
    descr->callback = callback;
    descr->dmabuf[0] = descr->pool->allocate();
    descr->dmabuf[1] = descr->pool->allocate();

    // Init and config DMA.
    hal_dma_config(&descr->dma, descr->dma_irqn, DMA_PERIPH_TO_MEMORY);

    // Init and config ADC.
    hal_adc_config(&descr->adc, ADC_RES_LUT[resolution], descr->tim_trig, adc_pins, n_channels);

    // Link DMA handle to ADC handle, and start the ADC.
    __HAL_LINKDMA(&descr->adc, DMA_Handle, descr->dma);
    HAL_ADC_Start_DMA(&descr->adc, (uint32_t *) descr->dmabuf[0]->data(), descr->dmabuf[0]->size());

    // Re/enable DMA double buffer mode.
    hal_dma_enable_dbm(&descr->dma, descr->dmabuf[0]->data(), descr->dmabuf[1]->data());

    // Init, config and start the ADC timer.
    hal_tim_config(&descr->tim, sample_rate);
    HAL_TIM_Base_Start(&descr->tim);
    return 1;
}

int AdvancedADC::stop()
{
    if (descr) {
        HAL_TIM_Base_Stop(&descr->tim);
        HAL_ADC_Stop_DMA(&descr->adc);
    }
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
        descr->callback = nullptr;
        for (size_t i=0; i<AN_ARRAY_SIZE(descr->dmabuf); i++) {
            if (descr->dmabuf[i]) {
                descr->dmabuf[i]->release();
                descr->dmabuf[i] = nullptr;
            }
        }
    }
}

extern "C" {
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc) {
    adc_descr_t *descr = adc_descr_get(adc->Instance);

    // NOTE: CT bit is inverted, to get the DMA buffer that's Not currently in use.
    size_t ct = ! hal_dma_get_ct(&descr->dma);

    // Timestamp the buffer. TODO: Should move to timer IRQ.
    descr->dmabuf[ct]->timestamp(HAL_GetTick());

    // Currently, all buffers are interleaved.
    descr->dmabuf[ct]->setflags(DMA_BUFFER_INTRLVD);

    if (descr->pool->writable()) {
        // Make sure any cached data is discarded.
        descr->dmabuf[ct]->invalidate();

        // Move current DMA buffer to ready queue.
        descr->pool->enqueue(descr->dmabuf[ct]);

        // Allocate a new free buffer.
        descr->dmabuf[ct] = descr->pool->allocate();
    } else {
        descr->dmabuf[ct]->setflags(DMA_BUFFER_DISCONT);
    }

    // Update the next DMA target pointer.
    // NOTE: If the pool was empty, the same buffer is reused.
    hal_dma_update_memory(&descr->dma, descr->dmabuf[ct]->data());
}

} // extern C
