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

#ifndef __ADVANCED_ADC_H__
#define __ADVANCED_ADC_H__

#include "AdvancedAnalog.h"

struct adc_descr_t;

typedef enum {
    AN_ADC_SAMPLETIME_1_5 = ADC_SAMPLETIME_1CYCLE_5,
    AN_ADC_SAMPLETIME_2_5 = ADC_SAMPLETIME_2CYCLES_5,
    AN_ADC_SAMPLETIME_8_5 = ADC_SAMPLETIME_8CYCLES_5,
    AN_ADC_SAMPLETIME_16_5 = ADC_SAMPLETIME_16CYCLES_5,
    AN_ADC_SAMPLETIME_32_5 = ADC_SAMPLETIME_32CYCLES_5,
    AN_ADC_SAMPLETIME_64_5 = ADC_SAMPLETIME_64CYCLES_5,
    AN_ADC_SAMPLETIME_387_5 = ADC_SAMPLETIME_387CYCLES_5,
    AN_ADC_SAMPLETIME_810_5 = ADC_SAMPLETIME_810CYCLES_5,
} adc_sample_time_t;

class AdvancedADC {
    private:
        size_t n_channels;
        adc_descr_t *descr;
        PinName adc_pins[AN_MAX_ADC_CHANNELS];

    public:
        template <typename ... T>
        AdvancedADC(pin_size_t p0, T ... args): n_channels(0), descr(nullptr) {
            static_assert(sizeof ...(args) < AN_MAX_ADC_CHANNELS,
                    "A maximum of 16 channels can be sampled successively.");

            for (auto p : {p0, args...}) {
                adc_pins[n_channels++] = analogPinToPinName(p);
            }
        }
        AdvancedADC(): n_channels(0), descr(nullptr) {
        }
        ~AdvancedADC();
        int id();
        bool available();
        SampleBuffer read();
        int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples,
                  size_t n_buffers, bool start=true, adc_sample_time_t sample_time=AN_ADC_SAMPLETIME_8_5);
        int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples,
                  size_t n_buffers, size_t n_pins, pin_size_t *pins, bool start=true,
                  adc_sample_time_t sample_time=AN_ADC_SAMPLETIME_8_5) {
            if (n_pins > AN_MAX_ADC_CHANNELS) {
                n_pins = AN_MAX_ADC_CHANNELS;
            }
            for (size_t i = 0; i < n_pins; ++i) {
                adc_pins[i] = analogPinToPinName(pins[i]);
            }

            n_channels = n_pins;
            return begin(resolution, sample_rate, n_samples, n_buffers, start, sample_time);
        }
        int start(uint32_t sample_rate);
        int stop();
        void clear();
        size_t channels();
};

class AdvancedADCDual {
    private:
        AdvancedADC &adc1;
        AdvancedADC &adc2;
        size_t n_channels;

    public:
        AdvancedADCDual(AdvancedADC &adc1_in, AdvancedADC &adc2_in):
            n_channels(0), adc1(adc1_in), adc2(adc2_in) {
        }
        ~AdvancedADCDual();
        int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples,
                  size_t n_buffers, adc_sample_time_t sample_time=AN_ADC_SAMPLETIME_8_5);
        int stop();
};

#endif // __ADVANCED_ADC_H__
