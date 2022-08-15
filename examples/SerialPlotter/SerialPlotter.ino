#include "AdvancedADC.h"

AdvancedADC adc(A0, A1);
uint64_t last_millis = 0;
uint32_t last_ts= 0;

void setup() {
    Serial.begin(9600);

    while (!Serial) {

    }

    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!adc.begin(ADC_RESOLUTION_16, 16000, 32, 128)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }
}

void loop() {
    if (adc.available()) {
        ADCBuffer buf = adc.dequeue();

        // Process the buffer.
        if (millis() - last_millis > 1) {
            // Print timestamp.
          if (last_ts){
              Serial.println(buf.timestamp() - last_ts);
          }
          last_ts = buf.timestamp();

          //Serial.println(buf[0]);   // Sample from first channel
          //Serial.println(buf[1]);   // Sample from second channel
          last_millis = millis();
        }

        // Direct access to the underlying buffer.
        // buf.data()

        // Release the buffer to return it to the pool.
        buf.release();

    }

    if (millis() - last_millis > 100) {
      //Serial.println("No buffer");
     // last_millis = millis();
    }
}
