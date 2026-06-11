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

    // The DAC pool is sized to match the ADC buffer length: 32 samples per
    // channel x 2 ADC channels = 64 samples per DAC buffer.
    if (!dac1.begin(AN_RESOLUTION_12, 16000, 64, 64)) {
        Serial.println("Failed to start DAC1 !");
        while (1);
    }
}

void loop() {
    if (adc1.available() && dac1.available()) {
        // Get a sample buffer from the ADC and a free buffer from the DAC.
        SampleBuffer in_buf = adc1.read();
        SampleBuffer out_buf = dac1.dequeue();

        // Copy the ADC samples to the DAC buffer.
        for (size_t i = 0; i < in_buf.size(); i++) {
            out_buf[i] = in_buf[i];
        }

        // Write the output buffer to the DAC and release the input buffer.
        dac1.write(out_buf);
        in_buf.release();
    }
}
