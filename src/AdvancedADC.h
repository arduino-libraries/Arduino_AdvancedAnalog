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

    public:
        template <typename ... T>
        AdvancedADC(pin_size_t p0, T ... args): n_channels(0), descr(nullptr) {
            static_assert(sizeof ...(args) < AN_MAX_ADC_CHANNELS,
                    "A maximum of 5 channels can be sampled successively.");

            for (auto p : {p0, args...}) {
                adc_pins[n_channels++] = analogPinToPinName(p);
            }
        }
        ~AdvancedADC();
        bool available();
        SampleBuffer read();
        int begin(uint32_t resolution, uint32_t sample_rate, size_t n_samples, size_t n_buffers);
        int stop();
};

#endif /* ARDUINO_ADVANCED_ADC_H_ */
