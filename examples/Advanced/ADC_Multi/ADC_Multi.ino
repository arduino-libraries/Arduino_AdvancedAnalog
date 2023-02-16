// This example shows how to use 2 ADC simultaneously.
#include <Arduino_AdvancedAnalog.h>

AdvancedADC adc1(A0);
AdvancedADC adc2(A1);
uint64_t last_millis = 0;

void setup() {
    Serial.begin(9600);

    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!adc1.begin(AN_RESOLUTION_16, 16000, 32, 64)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }

    if (!adc2.begin(AN_RESOLUTION_16, 8000, 32, 64)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }
}

void adc_print_buf(AdvancedADC &adc) {
    if (adc.available()) {
        SampleBuffer buf = adc.read();

        // Print first sample.
        Serial.println(buf[0]);

        // Release the buffer to return it to the pool.
        buf.release();
    }
}

void loop() {
    if (millis() - last_millis > 1) {
        adc_print_buf(adc1);
        adc_print_buf(adc2);
        last_millis = millis();
    }
}
