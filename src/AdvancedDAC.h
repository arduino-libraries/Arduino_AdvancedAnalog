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

#ifndef __ADVANCED_DAC_H__
#define __ADVANCED_DAC_H__

#include "AdvancedAnalog.h"

struct dac_descr_t;

class AdvancedDAC {
    private:
        size_t n_channels;
        dac_descr_t *descr;
        PinName dac_pins[AN_MAX_DAC_CHANNELS];

    public:
        template <typename ... T>
        AdvancedDAC(pin_size_t p0, T ... args): n_channels(0), descr(nullptr) {
            static_assert(sizeof ...(args) < AN_MAX_DAC_CHANNELS,
                    "A maximum of 1 channel is currently supported.");

            for (auto p : {p0, args...}) {
                dac_pins[n_channels++] = analogPinToPinName(p);
            }
        }
        ~AdvancedDAC();

        bool available();
        SampleBuffer dequeue();
        void write(SampleBuffer dmabuf);
        int begin(uint32_t resolution, uint32_t frequency, size_t n_samples=0, size_t n_buffers=0, bool loop=false);
        int stop();
        int frequency(uint32_t const frequency);
};

#endif // __ADVANCED_DAC_H__
