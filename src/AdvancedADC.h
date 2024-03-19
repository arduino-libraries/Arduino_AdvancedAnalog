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

#include <array>
#include "DMABuffer.h"
#include "AdvancedAnalog.h"

#ifndef ARDUINO_ADVANCED_ADC_H_
#define ARDUINO_ADVANCED_ADC_H_

struct adc_descr_t;

class AdvancedADC {
    private:
        size_t n_channels;
        adc_descr_t *descr;
        PinName adc_pins[AN_MAX_ADC_CHANNELS];
        bool dualMode;
        uint8_t selectedADC;

    public:
        template <typename ... T>
        AdvancedADC(pin_size_t p0, T ... args): n_channels(0), descr(nullptr) {
            static_assert(sizeof ...(args) < AN_MAX_ADC_CHANNELS,
                    "A maximum of 5 channels can be sampled successively.");

            for (auto p : {p0, args...}) {
                adc_pins[n_channels++] = analogPinToPinName(p);
            }
            dualMode=false;
            selectedADC=0;
        }
        AdvancedADC(): n_channels(0), descr(nullptr) {}
        ~AdvancedADC();
        bool available();
        SampleBuffer read();
        int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers, bool do_start=true,uint8_t adcNum=0);
        int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers, size_t n_pins, pin_size_t *pins, bool start=true,uint8_t fixed_adc=0) {
            if (n_pins > AN_MAX_ADC_CHANNELS) n_pins = AN_MAX_ADC_CHANNELS;
            for (size_t i = 0; i < n_pins; ++i) {
                adc_pins[i] = analogPinToPinName(pins[i]);
            }

            n_channels = n_pins;
            return begin(resolution, sample_rate, n_samples, n_buffers,start,fixed_adc);
        }
        
        void clear();

        int start(uint32_t sample_rate);
        
        //void setADCDualMode(bool dm);
        //int enableDualMode();
        //int disableDualMode();
        
        int stop();
};

class AdvancedADCDual {
		private:
			size_t n_channels;
			pin_size_t adc_pins_unmapped[AN_MAX_ADC_CHANNELS];
			AdvancedADC *adcIN1;
			AdvancedADC *adcIN2;
		public:
			
			//For now ADC1 will always be one pin, and ADC2 will sample the rest
			template <typename ... T>
			AdvancedADCDual(pin_size_t p0, T ... args):n_channels(0), adcIN1(nullptr), adcIN2(nullptr){
				static_assert(sizeof ...(args) < AN_MAX_ADC_CHANNELS+1,
                    "A maximum of 5 channels can be sampled successively.");
				static_assert (sizeof ...(args) >=2,
					"At least two channels are required for Dual Mode ADC.");
				for (auto p : {p0, args...}) {
					adc_pins_unmapped[n_channels++] = p;
                 }
            }
			
			AdvancedADCDual(): n_channels(0), adcIN1(nullptr), adcIN2(nullptr)
			{
			}
            ~AdvancedADCDual();

			int begin(AdvancedADC *in1, AdvancedADC *in2,uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers);
			int begin(AdvancedADC *in1, AdvancedADC *in2,uint32_t resolution, uint32_t sample_rate, 
				size_t n_samples, size_t n_buffers, size_t n_pins, pin_size_t *pins) {
            if (n_pins > AN_MAX_ADC_CHANNELS) n_pins = AN_MAX_ADC_CHANNELS;
            if(n_pins<2)  //Cannot run Dual mode with less than two input pins
                return(0);
            for (size_t i = 0; i < n_pins; ++i) {
                adc_pins_unmapped[i] = pins[i];
                Serial.println("Pin: "+String(pins[i]));
            }
            n_channels = n_pins;
            return begin(in1, in2, resolution, sample_rate, n_samples, n_buffers);
        }
			int stop();
};

#endif /* ARDUINO_ADVANCED_ADC_H_ */
