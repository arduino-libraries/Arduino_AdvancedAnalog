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

#ifndef __ADVANCED_ANALOG_H__
#define __ADVANCED_ANALOG_H__

#include "Arduino.h"
#include "api/DMAPool.h"
#include "pinDefinitions.h"

enum {
    AN_RESOLUTION_8  = 0U,
    AN_RESOLUTION_10 = 1U,
    AN_RESOLUTION_12 = 2U,
    AN_RESOLUTION_14 = 3U,
    AN_RESOLUTION_16 = 4U,
};

typedef uint16_t                Sample;     // Sample type used for ADC/DAC.
typedef DMABuffer<Sample>       &SampleBuffer;

#define AN_MAX_ADC_CHANNELS     (16)
#define AN_MAX_DAC_CHANNELS     (1)
#define AN_ARRAY_SIZE(a)        (sizeof(a) / sizeof(a[0]))

#endif  // __ADVANCED_ANALOG_H__
