// This example outputs an 8KHz square wave on DAC1.
// Connect ADC channel A0 to VDD and channel A1 to GND.

#include <Arduino_AdvancedAnalog.h>

AdvancedADC adc1(A0, A1);
AdvancedDAC dac1(A12);

void setup() {
    Serial.begin(9600);

    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!adc1.begin(AN_RESOLUTION_12, 8000, 32, 64)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }

    // Note there's no need to allocate a pool for DAC, since all buffers
    // are allocated from the ADC pool.
    if (!dac1.begin(AN_RESOLUTION_12, 16000)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }
}

void loop() {
    if (adc1.available()) {
        dac1.write(adc1.read());
    }
}
