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
#include "AdvancedADC.h"

#define ADC_NP  ((ADCName) NC)
#define ADC_PIN_ALT_MASK    (uint32_t) (ALT0 | ALT1 )

struct adc_descr_t {
    ADC_HandleTypeDef adc;
    DMA_HandleTypeDef dma;
    IRQn_Type dma_irqn;
    TIM_HandleTypeDef tim;
    uint32_t  tim_trig;
    DMABufferPool<Sample> *pool;
    DMABuffer<Sample> *dmabuf[2];
};

static uint32_t adc_pin_alt[3] = {0, ALT0, ALT1};

static adc_descr_t adc_descr_all[3] = {
    {{ADC1}, {DMA1_Stream1, {DMA_REQUEST_ADC1}}, DMA1_Stream1_IRQn, {TIM1}, ADC_EXTERNALTRIG_T1_TRGO,
        nullptr, {nullptr, nullptr}},
    {{ADC2}, {DMA1_Stream2, {DMA_REQUEST_ADC2}}, DMA1_Stream2_IRQn, {TIM2}, ADC_EXTERNALTRIG_T2_TRGO,
        nullptr, {nullptr, nullptr}},
    {{ADC3}, {DMA1_Stream3, {DMA_REQUEST_ADC3}}, DMA1_Stream3_IRQn, {TIM3}, ADC_EXTERNALTRIG_T3_TRGO,
        nullptr, {nullptr, nullptr}},
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

static void dac_descr_deinit(adc_descr_t *descr, bool dealloc_pool) {
    if (descr) {
        HAL_TIM_Base_Stop(&descr->tim);
        HAL_ADC_Stop_DMA(&descr->adc);

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
        }
    }
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
        while (!available()) {
            __WFI();
        }
        return *descr->pool->dequeue();
    }
    return NULLBUF;
}

int AdvancedADC::begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers,bool do_start,uint8_t adcNum) {
    
    ADCName instance = ADC_NP;
    // Sanity checks.
    if (resolution >= AN_ARRAY_SIZE(ADC_RES_LUT) || (descr && descr->pool)) {
        return 0;
    }

    //if ADC specified is more than the number of available ADC bail out
    if(adcNum>AN_ARRAY_SIZE(adc_pin_alt))
    {
        descr = nullptr;
        return 0;
    }
  
    // Clear ALTx pin.
    for (size_t i=0; i<n_channels; i++) {
        adc_pins[i] =  (PinName) (adc_pins[i] & ~(ADC_PIN_ALT_MASK));
    }
    

    // If ADC not specified find an ADC that can be used with these set of pins/channels.
    if(adcNum==0)
    {
        for (size_t i=0; instance == ADC_NP && i<AN_ARRAY_SIZE(adc_pin_alt); i++) {
            // Calculate alternate function pin.
            PinName pin = (PinName) (adc_pins[0] | adc_pin_alt[i]); // First pin decides the ADC.

            // Check if pin is mapped.
            if (pinmap_find_peripheral(pin, PinMap_ADC) == NC) {
                break;
            }

            // Find the first free ADC according to the available ADCs on pin.
            for (size_t j=0; instance == ADC_NP && j<AN_ARRAY_SIZE(adc_descr_all); j++) {
                descr = &adc_descr_all[j];
                if (descr->pool == nullptr) {
                    ADCName tmp_instance = (ADCName) pinmap_peripheral(pin, PinMap_ADC);
                    if (descr->adc.Instance == ((ADC_TypeDef*) tmp_instance)) {
                        instance = tmp_instance;
                        adc_pins[0] = pin;
                        selectedADC=j;
                    }
                }
            }
        }
    }
    else if(adcNum>0)  //if ADC specified use that ADC to try to map first channel
    {
        PinName pin = (PinName) (adc_pins[0] | adc_pin_alt[adcNum-1]); 

        // Check if pin is mapped.
        if (pinmap_find_peripheral(pin, PinMap_ADC) == NC) {
            return 0;
        }
   
        descr = &adc_descr_all[adcNum-1];
        if (descr->pool == nullptr) {
            ADCName tmp_instance = (ADCName) pinmap_peripheral(pin, PinMap_ADC);
            if (descr->adc.Instance == ((ADC_TypeDef*) tmp_instance)) {
                instance = tmp_instance;
                adc_pins[0] = pin;
            }
        }
        selectedADC=adcNum; //Store selected number
    }
    
    if (instance == ADC_NP) {
        // Couldn't find a free ADC/descriptor.
        descr = nullptr;
        return 0;
    }

    // Configure ADC pins.
    pinmap_pinout(adc_pins[0], PinMap_ADC);
    
    //If ADC was not specified ensure the remaining channels are mappable to same ADC
    if(adcNum==0)
    {
        uint8_t ch_init = 1;
        for (size_t i=1; i<n_channels; i++) {
            for (size_t j=0; j<AN_ARRAY_SIZE(adc_pin_alt); j++) {
                // Calculate alternate function pin.
                PinName pin = (PinName) (adc_pins[i] | adc_pin_alt[j]);
                // Check if pin is mapped.
                if (pinmap_find_peripheral(pin, PinMap_ADC) == NC) {
                    break;
                }
                // Check if pin is connected to the selected ADC.
                if (instance == pinmap_peripheral(pin, PinMap_ADC)) {
                    pinmap_pinout(pin, PinMap_ADC);
                    adc_pins[i] = pin;
                    ch_init++;
                    break;
                }
            }
        }
        // All channels must share the same instance; if not, bail out.
        if (ch_init < n_channels) {
            return 0;
        }
    }
    else if(adcNum>0)  //If specific ADC was requested ensure all other channels map to that same ADC
    {
        uint8_t ch_init = 1;
        for (size_t i=1; i<n_channels; i++) {
            
            // Calculate alternate function pin.
            PinName pin = (PinName) (adc_pins[i] | adc_pin_alt[adcNum-1]);
            // Check if pin is mapped.
            if (pinmap_find_peripheral(pin, PinMap_ADC) == NC) {
                break;
            }
            // Check if pin is connected to the selected ADC.
            if (instance == pinmap_peripheral(pin, PinMap_ADC)) {
                pinmap_pinout(pin, PinMap_ADC);
                adc_pins[i] = pin;
                ch_init++;
            }
        }
        // All channels must share the same instance; if not, bail out.
        if (ch_init < n_channels) {
            return 0;
        }
    }
    
    
    // Allocate DMA buffer pool.
    descr->pool = new DMABufferPool<Sample>(n_samples, n_channels, n_buffers);
    if (descr->pool == nullptr) {
        return 0;
    }

    //Allocate two DMA buffers for double buffering
    descr->dmabuf[0] = descr->pool->allocate();
    descr->dmabuf[1] = descr->pool->allocate();

    // Init and config DMA.
    if (hal_dma_config(&descr->dma, descr->dma_irqn, DMA_PERIPH_TO_MEMORY) < 0) {
        return 0;
    }

    // Init and config ADC.
    if (hal_adc_config(&descr->adc, ADC_RES_LUT[resolution], descr->tim_trig, adc_pins, n_channels) < 0) {
        return 0;
    }

    // Link DMA handle to ADC handle, and start the ADC.
     __HAL_LINKDMA(&descr->adc, DMA_Handle, descr->dma);
    if (HAL_ADC_Start_DMA(&descr->adc, (uint32_t *) descr->dmabuf[0]->data(), descr->dmabuf[0]->size()) != HAL_OK) {
        return 0;
    }

    // Re/enable DMA double buffer mode.
    HAL_NVIC_DisableIRQ(descr->dma_irqn);
    hal_dma_enable_dbm(&descr->dma, descr->dmabuf[0]->data(), descr->dmabuf[1]->data());
    HAL_NVIC_EnableIRQ(descr->dma_irqn);

    if(do_start)
    {
        return(start(sample_rate));
    }

    return 1;
}

int AdvancedADC::start(uint32_t sample_rate)
{
    // Init, config and start the ADC timer.
    hal_tim_config(&descr->tim, sample_rate);
    
    //Start Timer and ADC Capture.  If Dual Mode was enabled, then this will also start ADC2
    if (HAL_TIM_Base_Start(&descr->tim) != HAL_OK) {
        return 0;
    }

    return 1;
}

int AdvancedADC::stop()
{
    dac_descr_deinit(descr, true);
    return 1;
}

void AdvancedADC::clear()
{
    descr->pool->flush();
}

AdvancedADC::~AdvancedADC()
{
    dac_descr_deinit(descr, true);
}

int AdvancedADCDual::begin(AdvancedADC *in1, AdvancedADC *in2,uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers)
{
	adcIN1=in1;
	adcIN2=in2;
	
	int result=0;
	
	//Configure first pin on ADC1
	result=adcIN1->begin(resolution,sample_rate,n_samples,n_buffers,1,&(adc_pins_unmapped[0]),false,1);
	if(result!=1)
	{
		return(0);
	}
	//Configure all other pins on ADC2
	result=adcIN2->begin(resolution,sample_rate,n_samples,n_buffers,n_channels-1,&(adc_pins_unmapped[1]),false,2);
	if(result!=1)
	{
		return(0);
	}
	
	result=hal_enable_dual_mode();
	
	result=adcIN1->start(sample_rate);
	
	if(result!=1)
	{
		return(0);
	}
	
	return(1);
	
}

int AdvancedADCDual:: stop()
{
	if(adcIN1!=nullptr)
	{
		adcIN1->stop();
	}
    if(adcIN2!=nullptr)
    {
        adcIN2->stop();
    }
    //Always disable dual mode when stopped
	int result=hal_disable_dual_mode();
    return(1);
	
}

AdvancedADCDual::~AdvancedADCDual()
{
    int result=stop();
}

extern "C" {
void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *adc) {
    adc_descr_t *descr = adc_descr_get(adc->Instance);
    // NOTE: CT bit is inverted, to get the DMA buffer that's Not currently in use.
    size_t ct = ! hal_dma_get_ct(&descr->dma);

    // Timestamp the buffer.
    descr->dmabuf[ct]->timestamp(us_ticker_read());

    if (descr->pool->writable()) {
        // Make sure any cached data is discarded.
        descr->dmabuf[ct]->invalidate();

        // Move current DMA buffer to ready queue.
        descr->pool->enqueue(descr->dmabuf[ct]);

        // Allocate a new free buffer.
        descr->dmabuf[ct] = descr->pool->allocate();

        // Currently, all multi-channel buffers are interleaved.
        if (descr->dmabuf[ct]->channels() > 1) {
            descr->dmabuf[ct]->setflags(DMA_BUFFER_INTRLVD);
        }
    } else {
        descr->dmabuf[ct]->setflags(DMA_BUFFER_DISCONT);
    }

    // Update the next DMA target pointer.
    // NOTE: If the pool was empty, the same buffer is reused.
    hal_dma_update_memory(&descr->dma, descr->dmabuf[ct]->data());
}

} // extern C
