/*
  This file is part of the Arduino_AdvancedAnalog library.
  Copyright (c) 2023-2024 Arduino SA. All rights reserved.

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
#ifndef __ADVANCED_I2S_H__
#define __ADVANCED_I2S_H__

#include "AdvancedAnalog.h"

struct i2s_descr_t;

typedef enum {
    AN_I2S_MODE_IN      = (1U << 0U),
    AN_I2S_MODE_OUT     = (1U << 1U),
    AN_I2S_MODE_INOUT   = (AN_I2S_MODE_IN | AN_I2S_MODE_OUT),
} i2s_mode_t;

class AdvancedI2S {
    private:
        i2s_descr_t *descr;
        PinName i2s_pins[5];
        i2s_mode_t i2s_mode;

    public:
        AdvancedI2S(PinName ws, PinName ck, PinName sdi, PinName sdo, PinName mck):
            descr(nullptr), i2s_pins{ws, ck, sdi, sdo, mck} {
        }

        AdvancedI2S() {
        }

        ~AdvancedI2S();

        bool available();
        SampleBuffer read();
        SampleBuffer dequeue();
        void write(SampleBuffer dmabuf);
        int begin(i2s_mode_t i2s_mode, uint32_t sample_rate, size_t n_samples, size_t n_buffers);
        int stop();
};

#endif // __ADVANCED_I2S_H__
