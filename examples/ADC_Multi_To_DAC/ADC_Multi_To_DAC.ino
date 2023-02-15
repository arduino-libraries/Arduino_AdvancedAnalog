// This example outputs an 1KHz square wave on DAC1, using both ADC1 and ADC2
// NOTE: Connect ADC channel A0 to VDD and channel A1 to GND.

#include <Arduino_AdvancedAnalog.h>

AdvancedADC adc1(A0);
AdvancedADC adc2(A1);
AdvancedDAC dac1(A12);
uint64_t last_millis = 0;

void setup() {
    Serial.begin(9600);

    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!adc1.begin(AN_RESOLUTION_12, 8000, 8, 64)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }

    if (!adc2.begin(AN_RESOLUTION_12, 8000, 8, 64)) {
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
    if (adc2.available()) {
        dac1.write(adc2.read());
    }
}
