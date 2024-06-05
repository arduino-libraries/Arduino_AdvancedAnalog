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
    uint32_t dmaudr_flag;
    DMAPool<Sample> *pool;
    DMABuffer<Sample> *dmabuf[2];
    bool loop_mode;
};

// NOTE: Both DAC channel descriptors share the same DAC handle.
static DAC_HandleTypeDef dac = {0};

static dac_descr_t dac_descr_all[] = {
    {&dac, DAC_CHANNEL_1, {DMA1_Stream4, {DMA_REQUEST_DAC1_CH1}}, DMA1_Stream4_IRQn, {TIM4},
        DAC_TRIGGER_T4_TRGO, DAC_ALIGN_12B_R, DAC_FLAG_DMAUDR1, nullptr, {nullptr, nullptr}, false},
    {&dac, DAC_CHANNEL_2, {DMA1_Stream5, {DMA_REQUEST_DAC1_CH2}}, DMA1_Stream5_IRQn, {TIM5},
        DAC_TRIGGER_T5_TRGO, DAC_ALIGN_12B_R, DAC_FLAG_DMAUDR2, nullptr, {nullptr, nullptr}, false},
};

static uint32_t DAC_RES_LUT[] = {
    DAC_ALIGN_8B_R, DAC_ALIGN_12B_R, DAC_ALIGN_12B_R
};

static uint32_t DAC_CHAN_LUT[] = {
    DAC_CHANNEL_1, DAC_CHANNEL_2,
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

static void dac_descr_deinit(dac_descr_t *descr, bool dealloc_pool) {
    if (descr != nullptr) {
        HAL_TIM_Base_Stop(&descr->tim);
        HAL_DAC_Stop_DMA(descr->dac, descr->channel);

        __HAL_DAC_CLEAR_FLAG(descr->dac, descr->dmaudr_flag);

        for (size_t i=0; i<AN_ARRAY_SIZE(descr->dmabuf); i++) {
            if (descr->dmabuf[i]) {
                descr->dmabuf[i]->release();
                descr->dmabuf[i] = nullptr;
            }
        }

        if (dealloc_pool) {
            if (descr->pool) {
                delete descr->pool;
            }
            descr->pool = nullptr;
        } else {
            descr->pool->flush();
        }
    }
}

bool AdvancedDAC::available() {
    if (descr != nullptr) {
        if (__HAL_DAC_GET_FLAG(descr->dac, descr->dmaudr_flag)) {
            dac_descr_deinit(descr, false);
        }
        return descr->pool->writable();
    }
    return false;
}

DMABuffer<Sample> &AdvancedDAC::dequeue() {
    static DMABuffer<Sample> NULLBUF;
    if (descr != nullptr) {
        while (!available()) {
            __WFI();
        }
        return *descr->pool->alloc(DMA_BUFFER_WRITE);
    }
    return NULLBUF;
}

void AdvancedDAC::write(DMABuffer<Sample> &dmabuf) {
    static uint32_t buf_count = 0;

    if (descr == nullptr) {
        return;
    }

    // Make sure any cached data is flushed.
    dmabuf.flush();
    dmabuf.release();

    if (!descr->dmabuf[0] &&
       ((descr->loop_mode && !descr->pool->writable()) ||
       (!descr->loop_mode && (++buf_count % 3 == 0)))) {
        descr->dmabuf[0] = descr->pool->alloc(DMA_BUFFER_READ);
        descr->dmabuf[1] = descr->pool->alloc(DMA_BUFFER_READ);

        // Start DAC DMA.
        HAL_DAC_Start_DMA(descr->dac, descr->channel,
        (uint32_t *) descr->dmabuf[0]->data(), descr->dmabuf[0]->size(), descr->resolution);

        // Re/enable DMA double buffer mode.
        HAL_NVIC_DisableIRQ(descr->dma_irqn);
        hal_dma_enable_dbm(&descr->dma, descr->dmabuf[0]->data(), descr->dmabuf[1]->data());
        HAL_NVIC_EnableIRQ(descr->dma_irqn);

        // Start trigger timer.
        HAL_TIM_Base_Start(&descr->tim);
    }
}

int AdvancedDAC::begin(uint32_t resolution, uint32_t frequency, size_t n_samples, size_t n_buffers, bool loop) {
    // Sanity checks.
    if (resolution >= AN_ARRAY_SIZE(DAC_RES_LUT) || descr != nullptr) {
        return 0;
    }

    // Configure DAC GPIO pins.
    for (size_t i=0; i<n_channels; i++) {
        // Configure DAC GPIO pin.
        pinmap_pinout(dac_pins[i], PinMap_DAC);
    }

    uint32_t function = pinmap_function(dac_pins[0], PinMap_DAC);
    descr = dac_descr_get(DAC_CHAN_LUT[STM_PIN_CHANNEL(function) - 1]);
    if (descr == nullptr) {
        return 0;
    }

    // Allocate DMA buffer pool.
    descr->pool = new DMAPool<Sample>(n_samples, n_channels, n_buffers);
    if (descr->pool == nullptr) {
        descr = nullptr;
        return 0;
    }

    descr->loop_mode = loop;
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

    // Init and config the trigger timer.
    hal_tim_config(&descr->tim, frequency);
    return 1;
}

int AdvancedDAC::stop() {
    if (descr != nullptr) {
        dac_descr_deinit(descr, true);
        descr = nullptr;
    }
    return 1;
}

int AdvancedDAC::frequency(uint32_t const frequency) {
    if (descr != nullptr) {
        // Reconfigure the trigger timer.
        dac_descr_deinit(descr, false);
        hal_tim_config(&descr->tim, frequency);
    }
}

AdvancedDAC::~AdvancedDAC() {
    dac_descr_deinit(descr, true);
}

extern "C" {

void DAC_DMAConvCplt(DMA_HandleTypeDef *dma, uint32_t channel) {
    dac_descr_t *descr = dac_descr_get(channel);

    // Release the DMA buffer that was just done, allocate a new one,
    // and update the next DMA memory address target.
    if (descr && descr->pool->readable()) {
        // NOTE: CT bit is inverted, to get the DMA buffer that's Not currently in use.
        size_t ct = ! hal_dma_get_ct(dma);
        descr->dmabuf[ct]->release();
        descr->dmabuf[ct] = descr->pool->alloc(DMA_BUFFER_READ);
        if (descr->loop_mode) {
            // Move a buffer from the write queue to the read queue.
            descr->pool->alloc(DMA_BUFFER_WRITE)->release();
        }
        hal_dma_update_memory(dma, descr->dmabuf[ct]->data());
    } else {
        dac_descr_deinit(descr, false);
    }
}

void HAL_DAC_ConvCpltCallbackCh1(DAC_HandleTypeDef *dac) {
    DAC_DMAConvCplt(&dac_descr_all[0].dma, DAC_CHANNEL_1);
}

void HAL_DACEx_ConvCpltCallbackCh2(DAC_HandleTypeDef *dac) {
    DAC_DMAConvCplt(&dac_descr_all[1].dma, DAC_CHANNEL_2);
}

} // extern C
