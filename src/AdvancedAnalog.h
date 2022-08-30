#ifndef __ADVANCED_ANALOG_H__
#define __ADVANCED_ANALOG_H__
#include "Arduino.h"
#include "DMABuffer.h"
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

#define AN_MAX_ADC_CHANNELS     (5)
#define AN_MAX_DAC_CHANNELS     (1)
#define AN_ARRAY_SIZE(a)        (sizeof(a) / sizeof(a[0]))

#endif  // __ADVANCED_ANALOG_H__
