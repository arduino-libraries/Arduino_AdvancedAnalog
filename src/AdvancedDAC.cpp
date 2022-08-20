#include "Arduino.h"
#include "HALConfig.h"
#include "AdvancedDAC.h"

struct dac_descr_t {
    DAC_HandleTypeDef *dac;
    uint32_t  channel;
    DMA_HandleTypeDef dma;
    IRQn_Type dma_irqn;
    TIM_HandleTypeDef tim;
    uint32_t tim_trig;
    uint32_t resolution;
    user_callback_t callback;
    DMABuffer<Sample> *dmabuf;
    DMABufferPool<Sample> *pool;
};

// NOTE: Both DAC channel descriptors share the same DAC handle.
static DAC_HandleTypeDef dac = {0};

static dac_descr_t dac_descr_all[] = {
    {&dac, DAC_CHANNEL_1, {DMA1_Stream4, {DMA_REQUEST_DAC1_CH1}}, DMA1_Stream4_IRQn, {TIM4}, DAC_TRIGGER_T4_TRGO, DAC_ALIGN_12B_R},
    {&dac, DAC_CHANNEL_2, {DMA1_Stream5, {DMA_REQUEST_DAC1_CH2}}, DMA1_Stream5_IRQn, {TIM5}, DAC_TRIGGER_T5_TRGO, DAC_ALIGN_12B_R},
};

static uint32_t DAC_RES_LUT[] = {
    DAC_ALIGN_8B_R, DAC_ALIGN_12B_R,
};

extern "C" {

void DMA1_Stream4_IRQHandler() {
    HAL_DMA_IRQHandler(&dac_descr_all[0].dma);
}

void DMA1_Stream5_IRQHandler() {
    HAL_DMA_IRQHandler(&dac_descr_all[1].dma);
}

} // extern C

static dac_descr_t *dac_descr_get(uint32_t channel) {
    if (channel == DAC_CHANNEL_1) {
        return &dac_descr_all[0];
    } else if (channel == DAC_CHANNEL_2) {
        return &dac_descr_all[1];
    }
    return NULL;
}

static int hal_dac_config(DAC_HandleTypeDef *dac, uint32_t channel, uint32_t trigger) {
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

bool AdvancedDAC::available() {
    if (descr != nullptr) {
        return descr->pool->writable();
    }
    return false;
}

DMABuffer<Sample> &AdvancedDAC::dequeue() {
    static DMABuffer<Sample> NULLBUF;
    if (descr != nullptr) {
        return *descr->pool->allocate();
    }
    return NULLBUF;
}

void AdvancedDAC::write(DMABuffer<Sample> &dmabuf) {
    if (descr->dmabuf == nullptr) {
        descr->dmabuf = &dmabuf;
        // Stop trigger timer.
        HAL_TIM_Base_Stop(&descr->tim);

        // Start DAC DMA.
        HAL_DAC_Start_DMA(descr->dac, descr->channel, (uint32_t*) descr->dmabuf->data(), descr->dmabuf->size(), descr->resolution);

        // Re/enable DMA double buffer mode.
        hal_dma_enable_dbm(&descr->dma);

        // Start trigger timer.
        HAL_TIM_Base_Start(&descr->tim);
    } else {
        descr->pool->enqueue(descr->dmabuf);
    }
}

int AdvancedDAC::begin(uint32_t resolution, uint32_t frequency, size_t n_samples, size_t n_buffers, user_callback_t callback) {
    size_t n_channels = dac_pins.size();

    if (n_channels > 1) {
        return 0;
    }

    if (resolution >= _ARRAY_SIZE(DAC_RES_LUT)) {
        return 0;
    }

    // Configure DAC GPIO pins.
    for (auto &pin : dac_pins) {
        // Configure DAC GPIO pin.
        pinmap_pinout(pin, PinMap_DAC);
    }

    uint32_t function = pinmap_function(dac_pins[0], PinMap_DAC);
    descr = dac_descr_get(STM_PIN_CHANNEL(function));
    if (descr == nullptr || descr->pool) {
        return 0;
    }
    // Allocate DMA buffer pool.
    descr->pool = new DMABufferPool<Sample>(n_samples, 1, n_buffers);
    if (descr->pool == nullptr) {
        return 0;
    }
    descr->callback = callback;
    descr->resolution = DAC_RES_LUT[resolution];

    // Init and config DMA.
    hal_dma_config(&descr->dma, descr->dma_irqn, DMA_MEMORY_TO_PERIPH);

    // Init and config DAC.
    hal_dac_config(descr->dac, descr->channel, descr->tim_trig);

    // Link channel's DMA handle to DAC handle
    if (descr->channel == DAC_CHANNEL_1) {
        __HAL_LINKDMA(descr->dac, DMA_Handle1, descr->dma);
    } else {
        __HAL_LINKDMA(descr->dac, DMA_Handle2, descr->dma);
    }

    // Init, config and start the trigger timer.
    hal_tim_config(&descr->tim, frequency);

    return 1;
}

AdvancedDAC::~AdvancedDAC()
{
    stop();
    if (descr) {
        if (descr->pool) {
            delete descr->pool;
        }
        descr->pool = nullptr;
    }
}

int AdvancedDAC::stop()
{
    if (descr) {
        HAL_TIM_Base_Stop(&descr->tim);
        HAL_DAC_Stop_DMA(descr->dac, descr->channel);
    }
    return 1;
}

extern "C" {

void DAC_DMAConvCplt(DMA_HandleTypeDef *dma, uint32_t channel) {
    dac_descr_t *descr = dac_descr_get(channel);

    if (descr->dmabuf) {
        descr->dmabuf->release();
        descr->dmabuf = nullptr;
    }

    if (descr->pool->readable()) {
        // Get next buffer from the pool.
        descr->dmabuf = descr->pool->dequeue();

        // Make sure any cached data is flushed.
        descr->dmabuf->flush();

        // Update the next DMA target pointer.
        hal_dma_swap_memory(&descr->dma, descr->dmabuf->data());
    } else {
        HAL_DAC_Stop_DMA(descr->dac, descr->channel);
    }
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *dac) {
    DAC_DMAConvCplt(&dac_descr_all[0].dma, DAC_CHANNEL_1);
}

void HAL_DAC_ConvCpltCallbackCh2(DAC_HandleTypeDef *dac) {
    DAC_DMAConvCplt(&dac_descr_all[1].dma, DAC_CHANNEL_2);
}

} // extern C
