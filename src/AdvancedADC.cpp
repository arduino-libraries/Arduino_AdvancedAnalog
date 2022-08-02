#include "Arduino.h"
#include "AdvancedADC.h"

static struct adc_descr_t {
    ADC_HandleTypeDef adc;
    DMA_HandleTypeDef dma;
    DMABuffer<uint16_t> buf;
    DMABufferPool<uint16_t> *pool;
    adc_callback_t callback;
} adc_descr_all[3];

static uint32_t index_to_rank(int index)
{
    switch (index) {
        case 0:
            return ADC_REGULAR_RANK_1;
        case 1:
            return ADC_REGULAR_RANK_2;
        case 2:
            return ADC_REGULAR_RANK_3;
        case 3:
            return ADC_REGULAR_RANK_4;
        case 4:
            return ADC_REGULAR_RANK_5;
        default:
            return 0xFFFFFFFF;
    }
}

static adc_descr_t *adc_get_descr(ADC_HandleTypeDef* adc)
{
    adc_descr_t *adc_descr = NULL;
    if (adc == &adc_descr_all[0].adc) {
        adc_descr = &adc_descr_all[0];
    } else if (adc == &adc_descr_all[1].adc) {
        adc_descr = &adc_descr_all[1];
    } else {
        adc_descr = &adc_descr_all[2];
    }
    return adc_descr;
}

#if 0
static void *adc_get_next_buffer(ADC_HandleTypeDef* adc)
{
    adc_descr_t *adc_descr = adc_get_descr(adc);
    adc_descr->pool->enqueue(&adc_descr->buf);
    adc_descr->buf = adc_descr->pool->allocate();
    return adc_descr->buf.data();
}
#endif

bool AdvancedADC::available()
{
    if (adc_descr != nullptr) {
        return adc_descr->pool->available();
    }
    return false;
}

ADCBuffer AdvancedADC::dequeue()
{
    ADCBuffer buf;
    if (adc_descr != nullptr) {
        buf = adc_descr->pool->dequeue();
    }
    return buf;
}

void AdvancedADC::release(ADCBuffer buf)
{
    if (adc_descr != nullptr) {
        return adc_descr->pool->release(buf);
    }
}

int AdvancedADC::begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers, adc_callback_t cb)
{
    ADC_HandleTypeDef *adc = nullptr;
    DMA_HandleTypeDef *dma = nullptr;
    size_t n_channels = adc_pins.size();

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
    __HAL_RCC_ADC_CONFIG(RCC_ADCCLKSOURCE_CLKP);

    if (instance == ADC_1) {
        adc_descr = &adc_descr_all[0];
        dma = &adc_descr->dma;
        dma->Instance = DMA1_Stream1;
        dma->Init.Request = DMA_REQUEST_ADC1;
        __HAL_RCC_ADC12_CLK_ENABLE();
    }
    if (instance == ADC_2) {
        adc_descr = &adc_descr_all[1];
        dma = &adc_descr->dma;
        dma->Instance = DMA1_Stream2;
        dma->Init.Request = DMA_REQUEST_ADC2;
        __HAL_RCC_ADC12_CLK_ENABLE();
    }
    if (instance == ADC_3) {
        adc_descr = &adc_descr_all[2];
        dma = &adc_descr->dma;
        dma->Instance = DMA1_Stream3;
        dma->Init.Request = DMA_REQUEST_ADC3;
        __HAL_RCC_ADC3_CLK_ENABLE();
    }

    adc = &adc_descr->adc;
    adc_descr->callback = cb;

    // Enable DMA clock
    __HAL_RCC_DMA1_CLK_ENABLE();

    // DMA Init
    dma->Init.Direction = DMA_PERIPH_TO_MEMORY;
    dma->Init.PeriphInc = DMA_PINC_DISABLE;
    dma->Init.MemInc = DMA_MINC_ENABLE;
    dma->Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    dma->Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
//    dma->Init.Mode = DMA_CIRCULAR;
    dma->Init.Mode = DMA_DOUBLE_BUFFER_M0;
    dma->Init.Priority = DMA_PRIORITY_LOW;

    HAL_DMA_DeInit(dma);
    HAL_DMA_Init(dma);

    // Associate the DMA handle
    __HAL_LINKDMA(adc, DMA_Handle, *dma);

    if (instance == ADC_1) {
        // NVIC configuration for DMA Input data interrupt.
        HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
    }
    if (instance == ADC_2) {
        // NVIC configuration for DMA Input data interrupt.
        HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
    }
    if (instance == ADC_3) {
        // NVIC configuration for DMA Input data interrupt.
        HAL_NVIC_SetPriority(DMA1_Stream3_IRQn, 1, 0);
        HAL_NVIC_EnableIRQ(DMA1_Stream3_IRQn);
    }

    adc->Instance = (ADC_TypeDef*)instance;
    adc->Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV128;
    adc->Init.Resolution = ADC_RESOLUTION_12B; // TODO fix
    adc->Init.ScanConvMode = ADC_SCAN_ENABLE;
    adc->Init.EOCSelection = ADC_EOC_SEQ_CONV;
    //adc->Init.ScanConvMode = DISABLE;
    //adc->Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    adc->Init.LowPowerAutoWait = DISABLE;
    adc->Init.ContinuousConvMode = ENABLE;
    adc->Init.NbrOfConversion = n_channels;
    adc->Init.DiscontinuousConvMode = DISABLE;
    adc->Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc->Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc->Init.ConversionDataManagement = ADC_CONVERSIONDATA_DMA_CIRCULAR;
    adc->Init.Overrun = ADC_OVR_DATA_OVERWRITTEN;
    adc->Init.LeftBitShift = ADC_LEFTBITSHIFT_NONE;
    adc->Init.OversamplingMode = DISABLE;

    //hadc1.Init.OversamplingMode = ENABLE;
    //hadc1.Init.Oversampling.Ratio = 8;
    //hadc1.Init.Oversampling.RightBitShift = ADC_RIGHTBITSHIFT_3;
    //hadc1.Init.Oversampling.TriggeredMode = ADC_TRIGGEREDMODE_SINGLE_TRIGGER;
    //hadc1.Init.Oversampling.OversamplingStopReset = ADC_REGOVERSAMPLING_CONTINUED_MODE;

    HAL_ADC_Init(adc);
    HAL_ADCEx_Calibration_Start(adc, ADC_CALIB_OFFSET, ADC_SINGLE_ENDED);

    if (instance == ADC_1) {
        ADC_MultiModeTypeDef multimode = {0};
        multimode.Mode = ADC_MODE_INDEPENDENT;
        HAL_ADCEx_MultiModeConfigChannel(adc, &multimode);
    }

    ADC_ChannelConfTypeDef sConfig;
    sConfig.SamplingTime = ADC_SAMPLETIME_64CYCLES_5;
    sConfig.SingleDiff = ADC_SINGLE_ENDED;
    sConfig.OffsetNumber = ADC_OFFSET_NONE;
    sConfig.Offset = 0;

    int rank = 0;
    for (auto &pin : adc_pins) {
        uint32_t function = pinmap_function(pin, PinMap_ADC);
        uint32_t channel = STM_PIN_CHANNEL(function);
        sConfig.Rank = index_to_rank(rank++);
        sConfig.Channel = __HAL_ADC_DECIMAL_NB_TO_CHANNEL(channel);
        HAL_ADC_ConfigChannel(adc, &sConfig);
    }

    //HAL_ADC_RegisterCallback(&hadc1, HAL_ADC_CONVERSION_HALF_CB_ID, );
    //HAL_ADC_RegisterCallback(&hadc1, HAL_ADC_CONVERSION_COMPLETE_CB_ID, );

    adc_descr->pool = new DMABufferPool<uint16_t>(n_buffers, n_samples);
    adc_descr->buf = adc_descr->pool->allocate();
//    for (int i =0; i<32; i++) {
//        HAL_ADC_ConvCpltCallback(&adc_descr->adc);
//    }
    HAL_ADC_Start_DMA(adc, (uint32_t *) adc_descr->buf.data(), adc_descr->buf.size());
    return 1;
}

int AdvancedADC::stop()
{
    return 1;
}

extern "C" {
void DMA1_Stream1_IRQHandler()
{
    HAL_DMA_IRQHandler(adc_descr_all[0].adc.DMA_Handle);
}

void DMA1_Stream2_IRQHandler()
{
    HAL_DMA_IRQHandler(adc_descr_all[1].adc.DMA_Handle);
}

void DMA1_Stream3_IRQHandler()
{
    HAL_DMA_IRQHandler(adc_descr_all[2].adc.DMA_Handle);
}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef* adc)
{
    adc_descr_t *adc_descr = adc_get_descr(adc);
    ADCBuffer buf = adc_descr->pool->allocate();
    // If there's a free DMA buffer available, move the current buffer to the ready queue,
    // and use the free buffer for the next target, otherwise keep holding the current buffer.
    if (buf.data() != nullptr) {
        // Move to ready queue.
        adc_descr->pool->enqueue(adc_descr->buf);
        if (((DMA_Stream_TypeDef *)(adc->DMA_Handle->Instance))->CR & DMA_SxCR_CT) {
            // Update memory 0 address.
            HAL_DMAEx_ChangeMemory(adc->DMA_Handle, (uint32_t) buf.data(), MEMORY0);
        } else {
            // Update memory 1 address.
            HAL_DMAEx_ChangeMemory(adc->DMA_Handle, (uint32_t) buf.data(), MEMORY1);
        }
        // Set current buffer handle.
        adc_descr->buf = buf;
    }
}
} //extern C
