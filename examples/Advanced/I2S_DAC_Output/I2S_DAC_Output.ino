// This example demonstrates how to capture samples from an I2S mic,
// and output them on a DAC.

#include <Arduino_AdvancedAnalog.h>

AdvancedDAC dac1(A12);
// WS, CK, SDI, SDO, MCK
AdvancedI2S i2s(PG_10, PG_11, PG_9, PB_5, PC_4);

#define N_SAMPLES   (256)
#define SAMPLE_AVERAGE(s0, s1) (((int16_t) s0 / 2) + ((int16_t) s1 / 2))

void setup() {
    Serial.begin(9600);
    while (!Serial) {
    }

    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!dac1.begin(AN_RESOLUTION_12, 32000, N_SAMPLES, 32)) {
        Serial.println("Failed to start DAC1 !");
        while (1);
    }

    // I2S mode, sample rate, number of samples per channel, queue depth.
    if (!i2s.begin(AN_I2S_MODE_IN, 32000, N_SAMPLES, 32)) {
        Serial.println("Failed to start I2S");
        while (1);
    }
}

void loop() {
    if (i2s.available() && dac1.available()) {
        SampleBuffer i2sbuf = i2s.read();
        SampleBuffer dacbuf = dac1.dequeue();

        // Write data to buffer.
        for (int i=0; i<dacbuf.size(); i++) {
            // Average the 2 samples, map to positive and down scale to 12-bit. 
            // Note that I2S always captures 2 channels.
            dacbuf[i] = ((uint32_t) (SAMPLE_AVERAGE(i2sbuf[(i * 2)], i2sbuf[(i * 2) + 1]) + 32768)) >> 4;
        }

        // Write the buffer to DAC.
        dac1.write(dacbuf);
        i2sbuf.release();
    }
}
