// This example outputs an 8KHz square wave on DAC1.
// Connect ADC channel A0 to VDD and channel A1 to GND.
// Note DAC's sample size is double, because ADC samples 2 channels,
#include "AdvancedADC.h"
#include "AdvancedDAC.h"

AdvancedADC adc0(A0, A1);
AdvancedDAC dac1(A12);

void setup() {
    Serial.begin(9600);

    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!adc0.begin(AN_RESOLUTION_12, 8000, 32, 64)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }

    if (!dac1.begin(AN_RESOLUTION_12, 8000, 32 * 2, 64)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }
}

void loop() {
    if (adc0.available()) {
        dac1.write(adc0.read());
    }
}
