#ifndef __ADVANCED_ANALOG_H__
#define __ADVANCED_ANALOG_H__
#include "Arduino.h"
#include "DMABuffer.h"
#include "pinDefinitions.h"

enum {
    DAC_RESOLUTION_8  = 0U,
    DAC_RESOLUTION_12 = 1U,
};

enum {
    ADC_RESOLUTION_8  = 0U,
    ADC_RESOLUTION_10 = 1U,
    ADC_RESOLUTION_12 = 2U,
    ADC_RESOLUTION_14 = 3U,
    ADC_RESOLUTION_16 = 4U,
};

typedef uint16_t                Sample;     // Sample type used for ADC/DAC.
typedef DMABuffer<Sample>       &SampleBuffer;
typedef mbed::Callback<void()>  user_callback_t;

#define AN_MAX_ADC_CHANNELS     (5)
#define AN_MAX_DAC_CHANNELS     (1)
#define AN_ARRAY_SIZE(a)        (sizeof(a) / sizeof(a[0]))

#endif  // __ADVANCED_ANALOG_H__
