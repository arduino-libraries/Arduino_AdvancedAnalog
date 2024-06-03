/*
  This file is part of the Arduino_AdvancedAnalog library.
  Copyright (c) 2024 Arduino SA. All rights reserved.

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
#include "AdvancedI2S.h"

struct i2s_descr_t {
    I2S_HandleTypeDef i2s;
    DMA_HandleTypeDef dmatx;
    IRQn_Type dmatx_irqn;
    DMAPool<Sample> *dmatx_pool;
    DMABuffer<Sample> *dmatx_buf[2];
    DMA_HandleTypeDef dmarx;
    IRQn_Type dmarx_irqn;
    DMAPool<Sample> *dmarx_pool;
    DMABuffer<Sample> *dmarx_buf[2];
};

static i2s_descr_t i2s_descr_all[] = {
    {
        {SPI1},
        {DMA2_Stream1, {DMA_REQUEST_SPI1_TX}}, DMA2_Stream1_IRQn, nullptr, {nullptr, nullptr},
        {DMA2_Stream2, {DMA_REQUEST_SPI1_RX}}, DMA2_Stream2_IRQn, nullptr, {nullptr, nullptr},
    },
    {
        {SPI2},
        {DMA2_Stream3, {DMA_REQUEST_SPI2_TX}}, DMA2_Stream3_IRQn, nullptr, {nullptr, nullptr},
        {DMA2_Stream4, {DMA_REQUEST_SPI2_RX}}, DMA2_Stream4_IRQn, nullptr, {nullptr, nullptr},
    },
    {
        {SPI3},
        {DMA2_Stream5, {DMA_REQUEST_SPI3_TX}}, DMA2_Stream5_IRQn, nullptr, {nullptr, nullptr},
        {DMA2_Stream6, {DMA_REQUEST_SPI3_RX}}, DMA2_Stream6_IRQn, nullptr, {nullptr, nullptr},
    },
};

static const PinMap PinMap_SPI_MCK[] = {
    {PC_4, SPI_1, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_NOPULL, GPIO_AF5_SPI1)},
    {PC_6, SPI_2, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_NOPULL, GPIO_AF5_SPI2)},
    {PC_7, SPI_3, STM_PIN_DATA(STM_MODE_AF_PP, GPIO_NOPULL, GPIO_AF6_SPI3)},
    {NC, NC, 0}
};

extern "C" {

void DMA2_Stream1_IRQHandler() {
    HAL_DMA_IRQHandler(&i2s_descr_all[0].dmatx);
}

void DMA2_Stream2_IRQHandler() {
    HAL_DMA_IRQHandler(&i2s_descr_all[0].dmarx);
}

void DMA2_Stream3_IRQHandler() {
    HAL_DMA_IRQHandler(&i2s_descr_all[1].dmatx);
}

void DMA2_Stream4_IRQHandler() {
    HAL_DMA_IRQHandler(&i2s_descr_all[1].dmarx);
}

void DMA2_Stream5_IRQHandler() {
    HAL_DMA_IRQHandler(&i2s_descr_all[2].dmatx);
}

void DMA2_Stream6_IRQHandler() {
    HAL_DMA_IRQHandler(&i2s_descr_all[2].dmarx);
}

} // extern C

static uint32_t i2s_hal_mode(i2s_mode_t i2s_mode) {
    if (i2s_mode == AN_I2S_MODE_OUT) {
        return I2S_MODE_MASTER_TX;
    } else if (i2s_mode == AN_I2S_MODE_IN) {
        return I2S_MODE_MASTER_RX;
    } else {
        return I2S_MODE_MASTER_FULLDUPLEX;
    }
}

static i2s_descr_t *i2s_descr_get(SPI_TypeDef *i2s) {
    if (i2s == SPI1) {
        return &i2s_descr_all[0];
    } else if (i2s == SPI2) {
        return &i2s_descr_all[1];
    } else if (i2s == SPI3) {
        return &i2s_descr_all[2];
    }
    return NULL;
}

static void i2s_descr_deinit(i2s_descr_t *descr, bool dealloc_pool) {
    if (descr != nullptr) {
        HAL_I2S_DMAStop(&descr->i2s);

        for (size_t i=0; i<AN_ARRAY_SIZE(descr->dmatx_buf); i++) {
            if (descr->dmatx_buf[i]) {
                descr->dmatx_buf[i]->release();
                descr->dmatx_buf[i] = nullptr;
            }
        }

        for (size_t i=0; i<AN_ARRAY_SIZE(descr->dmarx_buf); i++) {
            if (descr->dmatx_buf[i]) {
                descr->dmatx_buf[i]->release();
                descr->dmatx_buf[i] = nullptr;
            }
        }

        if (dealloc_pool) {
            if (descr->dmarx_pool) {
                delete descr->dmarx_pool;
            }
            descr->dmarx_pool = nullptr;

            if (descr->dmatx_pool) {
                delete descr->dmatx_pool;
            }
            descr->dmatx_pool = nullptr;
        } else {
            if (descr->dmarx_pool) {
                descr->dmarx_pool->flush();
            }

            if (descr->dmatx_pool) {
                descr->dmatx_pool->flush();
            }
        }
    }
}

static int i2s_start_dma_transfer(i2s_descr_t *descr, i2s_mode_t i2s_mode) {
    uint16_t *tx_buf = NULL;
    uint16_t *rx_buf = NULL;
    uint16_t buf_size = 0;

    if (i2s_mode & AN_I2S_MODE_IN) {
        // Start I2S DMA.
        descr->dmarx_buf[0] = descr->dmarx_pool->alloc(DMA_BUFFER_WRITE);
        descr->dmarx_buf[1] = descr->dmarx_pool->alloc(DMA_BUFFER_WRITE);
        rx_buf = (uint16_t *) descr->dmarx_buf[0]->data();
        buf_size = descr->dmarx_buf[0]->size();
        HAL_NVIC_DisableIRQ(descr->dmarx_irqn);
    }

    if (i2s_mode & AN_I2S_MODE_OUT) {
        descr->dmatx_buf[0] = descr->dmatx_pool->alloc(DMA_BUFFER_READ);
        descr->dmatx_buf[1] = descr->dmatx_pool->alloc(DMA_BUFFER_READ);
        tx_buf = (uint16_t *) descr->dmatx_buf[0]->data();
        buf_size = descr->dmatx_buf[0]->size();
        HAL_NVIC_DisableIRQ(descr->dmatx_irqn);
    }

    // Start I2S DMA.
    if (i2s_mode == AN_I2S_MODE_IN) {
        if (HAL_I2S_Receive_DMA(&descr->i2s, rx_buf, buf_size) != HAL_OK) {
            return 0;
        }
    } else if (i2s_mode == AN_I2S_MODE_OUT) {
        if (HAL_I2S_Transmit_DMA(&descr->i2s, tx_buf, buf_size) != HAL_OK) {
            return 0;
        }
    } else {
        if (HAL_I2SEx_TransmitReceive_DMA(&descr->i2s, tx_buf, rx_buf, buf_size) != HAL_OK) {
            return 0;
        }
    }
    HAL_I2S_DMAPause(&descr->i2s);
    // Re/enable DMA double buffer mode.
    if (i2s_mode & AN_I2S_MODE_IN) {
        hal_dma_enable_dbm(&descr->dmarx, descr->dmarx_buf[0]->data(), descr->dmarx_buf[1]->data());
        HAL_NVIC_EnableIRQ(descr->dmarx_irqn);
    }

    if (i2s_mode & AN_I2S_MODE_OUT) {
        hal_dma_enable_dbm(&descr->dmatx, descr->dmatx_buf[0]->data(), descr->dmatx_buf[1]->data());
        HAL_NVIC_EnableIRQ(descr->dmatx_irqn);
    }
    HAL_I2S_DMAResume(&descr->i2s);
    return 1;
}

bool AdvancedI2S::available() {
    if (descr != nullptr) {
        if (i2s_mode == AN_I2S_MODE_IN && descr->dmarx_pool) {
            return descr->dmarx_pool->readable();
        } else if (i2s_mode == AN_I2S_MODE_OUT && descr->dmatx_pool) {
            return descr->dmatx_pool->writable();
        } else if (descr->dmatx_pool && descr->dmarx_pool) {
            return descr->dmarx_pool->readable() && descr->dmatx_pool->writable();
        }
    }
    return false;
}

DMABuffer<Sample> &AdvancedI2S::read() {
    static DMABuffer<Sample> NULLBUF;
    if (descr && descr->dmarx_pool) {
        while (!descr->dmarx_pool->readable()) {
            __WFI();
        }
        return *descr->dmarx_pool->alloc(DMA_BUFFER_READ);
    }
    return NULLBUF;
}

DMABuffer<Sample> &AdvancedI2S::dequeue() {
    static DMABuffer<Sample> NULLBUF;
    if (descr && descr->dmatx_pool) {
        while (!descr->dmatx_pool->writable()) {
            __WFI();
        }
        return *descr->dmatx_pool->alloc(DMA_BUFFER_WRITE);
    }
    return NULLBUF;
}

void AdvancedI2S::write(DMABuffer<Sample> &dmabuf) {
    static uint32_t buf_count = 0;

    if (descr == nullptr) {
        return;
    }

    // Make sure any cached data is flushed.
    dmabuf.flush();
    dmabuf.release();

    if (descr->dmatx_buf[0] == nullptr && (++buf_count % 3) == 0) {
        i2s_start_dma_transfer(descr, i2s_mode);
    }
}

int AdvancedI2S::begin(i2s_mode_t i2s_mode, uint32_t sample_rate, size_t n_samples, size_t n_buffers) {
    this->i2s_mode = i2s_mode;

    // Sanity checks.
    if (sample_rate < 8000 || sample_rate > 192000 || descr != nullptr) {
        return 0;
    }

    // Configure I2S pins.
    uint32_t i2s = NC;
    const PinMap *i2s_pins_map[] = {
        PinMap_SPI_SSEL, PinMap_SPI_SCLK, PinMap_SPI_MISO, PinMap_SPI_MOSI, PinMap_SPI_MCK
    }; 

    for (size_t i=0; i<AN_ARRAY_SIZE(i2s_pins); i++) {
        uint32_t per;
        if (i2s_pins[i] == NC) {
            continue;
        }
        per = pinmap_find_peripheral(i2s_pins[i], i2s_pins_map[i]);
        if (per == NC) {
            return 0;
        } else if (i2s == NC) {
            i2s = per;
        } else if (i2s != per) {
            return 0;
        }
        pinmap_pinout(i2s_pins[i], i2s_pins_map[i]);
    }

    descr = i2s_descr_get((SPI_TypeDef *) i2s);
    if (descr == nullptr) {
        return 0;
    }

    if (i2s_mode & AN_I2S_MODE_IN) {
        // Allocate DMA buffer pool.
        descr->dmarx_pool = new DMAPool<Sample>(n_samples, 2, n_buffers);
        if (descr->dmarx_pool == nullptr) {
            descr = nullptr;
            return 0;
        }
        // Init and config DMA.
        if (hal_dma_config(&descr->dmarx, descr->dmarx_irqn, DMA_PERIPH_TO_MEMORY) != 0) {
            return 0;
        }
        __HAL_LINKDMA(&descr->i2s, hdmarx, descr->dmarx);
    }

    if (i2s_mode & AN_I2S_MODE_OUT) {
        // Allocate DMA buffer pool.
        descr->dmatx_pool = new DMAPool<Sample>(n_samples, 2, n_buffers);
        if (descr->dmatx_pool == nullptr) {
            descr = nullptr;
            return 0;
        }
        // Init and config DMA.
        if (hal_dma_config(&descr->dmatx, descr->dmatx_irqn, DMA_MEMORY_TO_PERIPH) != 0) {
            return 0;
        }
        __HAL_LINKDMA(&descr->i2s, hdmatx, descr->dmatx);
    }

    // Init and config I2S.
    if (hal_i2s_config(&descr->i2s, sample_rate, i2s_hal_mode(i2s_mode), i2s_pins[4] != NC) != 0) {
        return 0;
    }

    if (i2s_mode == AN_I2S_MODE_IN) {
        return i2s_start_dma_transfer(descr, i2s_mode);
    }

    if (i2s_mode == AN_I2S_MODE_INOUT) {
        // The transmit pool has to be primed with a few buffers first, before the
        // DMA can be started in full-duplex mode.
        for (int i=0; i<3; i++) {
            SampleBuffer outbuf = dequeue();
            memset(outbuf.data(), 0, outbuf.bytes());
            write(outbuf);
        }
    }
    return 1;
}

int AdvancedI2S::stop() {
    i2s_descr_deinit(descr, true);
    descr = nullptr;
    return 1;
}

AdvancedI2S::~AdvancedI2S() {
    i2s_descr_deinit(descr, true);
}

extern "C" {

void HAL_I2S_TxCpltCallback(I2S_HandleTypeDef *i2s) {
    i2s_descr_t *descr = i2s_descr_get(i2s->Instance);
    
    if (descr == nullptr) {
        return;
    }

    // NOTE: CT bit is inverted, to get the DMA buffer that's Not currently in use.
    size_t ct = ! hal_dma_get_ct(&descr->dmatx);

    // Release the DMA buffer that was just used, dequeue the next one, and update
    // the next DMA memory address target.
    if (descr->dmatx_pool->readable()) {
        descr->dmatx_buf[ct]->release();
        descr->dmatx_buf[ct] = descr->dmatx_pool->alloc(DMA_BUFFER_READ);
        hal_dma_update_memory(&descr->dmatx, descr->dmatx_buf[ct]->data());
    } else {
        i2s_descr_deinit(descr, false);
    }
}

void HAL_I2S_RxCpltCallback(I2S_HandleTypeDef *i2s) {
    i2s_descr_t *descr = i2s_descr_get(i2s->Instance);

    if (descr == nullptr) {
        return;
    }

    // NOTE: CT bit is inverted, to get the DMA buffer that's Not currently in use.
    size_t ct = ! hal_dma_get_ct(&descr->dmarx);

    // Update the buffer's timestamp.
    descr->dmarx_buf[ct]->timestamp(us_ticker_read());

    // Flush the DMA buffer that was just used, move it to the ready queue, and
    // allocate a new one.
    if (descr->dmarx_pool->writable()) {
        // Make sure any cached data is discarded.
        descr->dmarx_buf[ct]->invalidate();
        // Move current DMA buffer to ready queue.
        descr->dmarx_buf[ct]->release();
        // Allocate a new free buffer.
        descr->dmarx_buf[ct] = descr->dmarx_pool->alloc(DMA_BUFFER_WRITE);
        // Currently, all multi-channel buffers are interleaved.
        if (descr->dmarx_buf[ct]->channels() > 1) {
            descr->dmarx_buf[ct]->set_flags(DMA_BUFFER_INTRLVD);
        }
    } else {
        descr->dmarx_buf[ct]->set_flags(DMA_BUFFER_DISCONT);
    }

    // Update the next DMA target pointer.
    // NOTE: If the pool was empty, the same buffer is reused.
    hal_dma_update_memory(&descr->dmarx, descr->dmarx_buf[ct]->data());
}

void HAL_I2SEx_TxRxCpltCallback(I2S_HandleTypeDef *i2s) {
    HAL_I2S_RxCpltCallback(i2s);
    HAL_I2S_TxCpltCallback(i2s);
}

} // extern C
