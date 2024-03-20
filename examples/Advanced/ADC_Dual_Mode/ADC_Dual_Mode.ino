#include <Arduino_AdvancedAnalog.h>

AdvancedADC adc1(A0, A1);
AdvancedADC adc2(A2, A3);
AdvancedADCDual adc_dual(adc1, adc2);
uint64_t last_millis = 0;

void setup() {
    Serial.begin(9600);
    while (!Serial) {
    }
    
    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!adc_dual.begin(AN_RESOLUTION_16, 16000, 32, 32)) {
        Serial.println("Failed to start analog acquisition!");
        while (1);
    }
}

void loop() {
    if (adc1.available()) {
        SampleBuffer buf1 = adc1.read();
        SampleBuffer buf2 = adc2.read();

        // Process the buffer.
        if (millis() - last_millis > 1) {
            Serial.println(buf1.timestamp());  // Print buffer timestamp
            Serial.println(buf1[0]);           // Print sample from first channel
            Serial.println(buf1[1]);           // Print sample from second channel
            
            Serial.println(buf2.timestamp());  // Print buffer timestamp
            Serial.println(buf2[0]);           // Print sample from first channel
            Serial.println(buf2[1]);           // Print sample from second channel

            last_millis = millis();
        }

        // Release the buffer to return it to the pool.
        buf1.release(); 
        buf2.release();
    }
}
