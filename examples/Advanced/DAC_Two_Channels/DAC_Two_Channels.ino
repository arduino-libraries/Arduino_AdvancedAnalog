// This example outputs an 8KHz square wave on A12/DAC0 and 16KHz square wave on ADC13/DAC1.
#include <Arduino_AdvancedAnalog.h>

AdvancedDAC dac1(A12);
AdvancedDAC dac2(A13);

void setup() {
    Serial.begin(9600);

    while (!Serial) {

    }

    if (!dac1.begin(AN_RESOLUTION_12, 8000, 32, 64)) {
        Serial.println("Failed to start DAC1 !");
        while (1);
    }

    if (!dac2.begin(AN_RESOLUTION_12, 16000, 32, 64)) {
        Serial.println("Failed to start DAC2 !");
        while (1);
    }
}

void dac_output_sq(AdvancedDAC &dac_out) {
    if (dac_out.available()) {
      
        // Get a free buffer for writing.
        SampleBuffer buf = dac_out.dequeue();
        
        // Write data to buffer.
        for (int i=0; i<buf.size(); i++) {
          buf.data()[i] =  (i % 2 == 0) ? 0: 0xfff;
        }
        
        // Write the buffer to DAC.
        dac_out.write(buf);
    }
}

void loop() {
  dac_output_sq(dac1);
  dac_output_sq(dac2);
}
