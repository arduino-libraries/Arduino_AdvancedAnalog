#include "AdvancedADC.h"

AdvancedADC adc(A0);
uint64_t last_micros = 0;

void setup() {
    Serial.begin(9600);
    while (!Serial) {

    }

    // 16bits, 48K, 32 samples (per channel), queue depth 128 buffers
    if (!adc.begin(ADC_RESOLUTION_16, ADC_SAMPLE_RATE_48K, 16, 8)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }
}

void loop() {
    if (adc.available()) {
        ADCBuffer buf = adc.dequeue();
        // do something with the data;
        Serial.println(buf[0]);

        // Direct access to the underlying buffer
        // buf.data()

        // done with the current buffer
        // Must call release to return to the buffer pool.
        adc.release(buf);
    } else if (micros() - last_micros > 1000*1000) {
      Serial.println("No buffer");
      last_micros = micros();
    }
}
