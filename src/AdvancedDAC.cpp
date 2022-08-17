#include "Arduino.h"
#include "AdvancedDAC.h"

struct dac_descr_t {
    DAC_HandleTypeDef *dac;
    uint32_t  channel;
    DMA_HandleTypeDef dma;
    IRQn_Type dma_irqn;
    TIM_HandleTypeDef tim;
    uint32_t tim_trig;
    uint32_t resolution;
    DMABufPool<sample_t> *pool;
    DMABuffer<sample_t>  *dmabuf;
};

static DAC_HandleTypeDef dac = {0};
static dac_descr_t dac_descr_all[] = {
    {&dac, DAC_CHANNEL_1, {DMA1_Stream4, {DMA_REQUEST_DAC1_CH1}}, DMA1_Stream4_IRQn, {TIM4}, DAC_TRIGGER_T4_TRGO, DAC_ALIGN_12B_R},
    {&dac, DAC_CHANNEL_2, {DMA1_Stream5, {DMA_REQUEST_DAC1_CH2}}, DMA1_Stream5_IRQn, {TIM5}, DAC_TRIGGER_T5_TRGO, DAC_ALIGN_12B_R},
};

static uint32_t DAC_CHAN_LUT[] = {
    DAC_CHANNEL_1, DAC_CHANNEL_2,
};

static uint32_t DAC_RES_LUT[] = {
    DAC_ALIGN_8B_R, DAC_ALIGN_12B_R,
};

#define DAC_ARRAY_SIZE(a) (sizeof(a)/sizeof(a[0]))

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

    // Init, config and start the timer.
    if ((HAL_TIM_PWM_Init(tim) != HAL_OK)
    || (HAL_TIMEx_MasterConfigSynchronization(tim, &sConfig) != HAL_OK)
    || (HAL_TIM_Base_Start(tim) != HAL_OK)) {
        return -1;
    }

    return 0;
}

static int hal_dma_config(DMA_HandleTypeDef *dma, IRQn_Type irqn, uint32_t dma_mode) {
    // DMA Init
    dma->Init.Mode                  = dma_mode;
    dma->Init.Priority              = DMA_PRIORITY_LOW;
    dma->Init.Direction             = DMA_MEMORY_TO_PERIPH;
    dma->Init.FIFOMode              = DMA_FIFOMODE_ENABLE;
    dma->Init.FIFOThreshold         = DMA_FIFO_THRESHOLD_FULL;
    dma->Init.MemInc                = DMA_MINC_ENABLE;
    dma->Init.PeriphInc             = DMA_PINC_DISABLE;
    dma->Init.MemBurst              = DMA_MBURST_SINGLE;
    dma->Init.PeriphBurst           = DMA_PBURST_SINGLE;
    dma->Init.MemDataAlignment      = DMA_MDATAALIGN_HALFWORD;
    dma->Init.PeriphDataAlignment   = DMA_PDATAALIGN_HALFWORD;

    // Enable DMA clock
    if (dma->Instance == DMA1) {
        __HAL_RCC_DMA1_CLK_ENABLE();
    } else {
        __HAL_RCC_DMA2_CLK_ENABLE();
    }

    if (HAL_DMA_DeInit(dma) != HAL_OK
     || HAL_DMA_Init(dma) != HAL_OK) {
        return -1;
    }

    // NVIC configuration for DMA Input data interrupt.
    HAL_NVIC_SetPriority(irqn, 1, 0);
    HAL_NVIC_EnableIRQ(irqn);

    return 0;
}

static int hal_dac_config(DAC_HandleTypeDef *dac, uint32_t channel, uint32_t trigger) {
    // DAC init
    if (dac->Instance == NULL) {
        // Enable DAC clock
        __HAL_RCC_DAC12_CLK_ENABLE();

        dac->Instance = DAC1;
        if (HAL_DAC_DeInit(&dac) != HAL_OK
         || HAL_DAC_Init(&dac) != HAL_OK) {
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

DMABuffer<sample_t> &AdvancedADC::dequeue() {
    static DMABuffer<sample_t> NULLBUF;
    if (descr != nullptr) {
        return *descr->pool->allocate();
    } else {
        return NULLBUF;
    }
}

void AdvancedDAC::write(DMABuffer<sample_t> *dmabuf) {
    if (descr->dmabuf == nullptr) {
        descr->dmabuf = dmabuf;
        HAL_DAC_Start_DMA(&descr->dac, descr->channel, (uint32_t*) descr->dmabuf->data(), descr->dmabuf->size(), descr->resolution);
    } else {
        descr->queue->push(dmabuf);
}

int AdvancedDAC::begin(uint32_t resolution, uint32_t frequency, size_t n_samples, size_t n_buffers) {
    size_t n_channels = dac_pins.size();

    if (n_channels > 1) {
        return 0;
    }

    if (resolution >= DAC_ARRAY_SIZE(DAC_RES_LUT)) {
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
    descr->pool = new DMABufPool<sample_t>(n_samples, n_buffers);
    if (descr->pool == nullptr) {
        return 0;
    }
    descr->resolution = DAC_RES_LUT[resolution];

    // Init and config DMA.
    hal_dma_config(&descr->dma, descr->dma_irqn, DMA_NORMAL);

    // Init and config DAC.
    hal_dac_config(&descr->dac, descr->channel, descr->tim_trig, dac_pins);

    // Link channel's DMA handle to DAC handle
    if (descr->channel == DAC_CHANNEL_1) {
        __HAL_LINKDMA(&descr->dac, DMA_Handle1, descr->dma);
    } else {
        __HAL_LINKDMA(&descr->dac, DMA_Handle2, descr->dma);
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
        HAL_DAC_Stop_DMA(&descr->dac);
    }
    return 1;
}

extern "C" {
void DAC_DMAConvCplt(DMA_HandleTypeDef *dma, uint32_t channel) {
    dac_descr_t *descr = dac_descr_get(channel);

    if (descr->dmabuf) {
        descr->dmabuf->release();
    }

    if (queue.empty()) {
        descr->dmabuf = nullptr;
        HAL_DAC_Stop_DMA(&descr->dac);
    } else {
        descr->dmabuf = descr->queue.pop();

        // Make sure any cached data is flushed.
        descr->dmabuf->flush();

        // Update the next DMA target pointer. Note if the pool was empty, the same buffer is reused.
        if ((((DMA_Stream_TypeDef *) dma->Instance)->CR & DMA_SxCR_CT) == 0U) {
            HAL_DMAEx_ChangeMemory(dac->DMA_Handle, (uint32_t) descr->dmabuf->data(), MEMORY1);
        } else {                     
            HAL_DMAEx_ChangeMemory(dac->DMA_Handle, (uint32_t) descr->dmabuf->data(), MEMORY0);
        }
    }
}

void DAC_DMAConvCpltCh1(DMA_HandleTypeDef *dma) {
    DAC_DMAConvCplt(dma, DAC_CHANNEL_1);
}

void DAC_DMAConvCpltCh2(DMA_HandleTypeDef *dma) {
    DAC_DMAConvCplt(dma, DAC_CHANNEL_2);
}

} // extern C
