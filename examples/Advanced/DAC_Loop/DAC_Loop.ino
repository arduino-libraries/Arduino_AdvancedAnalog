// This examples shows how to use the DAC in loop mode. In loop mode the
// DAC starts automatically after all buffers are filled, and continuously
// cycle through over all buffers.
#include <Arduino_AdvancedAnalog.h>

AdvancedDAC dac1(A12);

void setup() {
    Serial.begin(9600);

    while (!Serial) {

    }

    // Start DAC in loop mode.
    if (!dac1.begin(AN_RESOLUTION_12, 16000, 32, 16, true)) {
        Serial.println("Failed to start DAC1 !");
        while (1);
    }

    // Write all buffers.
    uint16_t sample = 0;
    while (dac1.available()) {
        // Get a free buffer for writing.
        SampleBuffer buf = dac1.dequeue();

        // Write data to buffer.
        for (int i=0; i<buf.size(); i++) {
            buf.data()[i] = sample;
        }       
        
        // Write the buffer to DAC.
        dac1.write(buf);
        sample += 256;
    }
}


void loop() {
  // In loop mode, no other DAC functions need to be called.
}
