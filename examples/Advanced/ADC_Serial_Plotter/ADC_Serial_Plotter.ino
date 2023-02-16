#include <Arduino_AdvancedAnalog.h>

AdvancedADC adc(A0, A1);
uint64_t last_millis = 0;

void setup() {
    Serial.begin(9600);

    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!adc.begin(AN_RESOLUTION_16, 16000, 32, 128)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }
}

void loop() {
    if (adc.available()) {
        SampleBuffer buf = adc.read();
        // Process the buffer.
        if ((millis() - last_millis) > 20) {
          Serial.println(buf[0]);   // Sample from first channel
          Serial.println(buf[1]);   // Sample from second channel
          last_millis = millis();
        }
        // Release the buffer to return it to the pool.
        buf.release();
    }
}
