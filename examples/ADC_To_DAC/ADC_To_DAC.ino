#include "AdvancedADC.h"
#include "AdvancedDAC.h"

AdvancedADC adc0(A0);
AdvancedDAC dac1(A12);

void setup() {
    Serial.begin(9600);

    while (!Serial) {

    }

    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!adc0.begin(ADC_RESOLUTION_12, 16000, 32, 64)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }

    if (!dac1.begin(DAC_RESOLUTION_12, 8000, 32, 64)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }
}

void loop() {
    if (adc0.available()) {
        SampleBuffer buf = adc0.read();
        dac1.write(buf);
    }
}
