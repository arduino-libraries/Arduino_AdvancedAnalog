#include "AdvancedADC.h"
#include "AdvancedDAC.h"

AdvancedADC adc_in(A0);
AdvancedDAC dac_out(A12);

void setup() {
    Serial.begin(9600);

    while (!Serial) {

    }

    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!adc_in.begin(ADC_RESOLUTION_12, 16000, 32, 64)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }

    if (!dac_out.begin(DAC_RESOLUTION_12, 8000, 32, 64)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }
}

void loop() {
    if (adc_in.available()) {
        SampleBuffer buf = adc_in.read();
        dac_out.write(buf);
    }
}
