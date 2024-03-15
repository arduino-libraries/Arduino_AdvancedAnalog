// This example demonstrates I2S in full-duplex mode. In the main loop, samples
// are continuously captured from the I2S input, and written back to I2S output.

#include <Arduino_AdvancedAnalog.h>

// WS, CK, SDI, SDO, MCK
AdvancedI2S i2s(PG_10, PG_11, PG_9, PB_5, PC_4);

void setup() {
    Serial.begin(9600);
    while (!Serial) {
      
    }

    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!i2s.begin(AN_I2S_MODE_INOUT, 32000, 512, 32)) {
        Serial.println("Failed to start I2S");
        while (1);
    }
}

void loop() {
    if (i2s.available()) {
      SampleBuffer rxbuf = i2s.read();
      SampleBuffer txbuf = i2s.dequeue();
      for (size_t i=0; i<rxbuf.size(); i++) {
        txbuf[i] = rxbuf[i];
      }
      rxbuf.release();
      i2s.write(txbuf);
    }
}
