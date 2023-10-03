/* ADC Multi Channel sampling usage demo
 *
 * Queries for pin numbers to sample on the Serial Monitor, then records and prints three readings on
 * each of those pins at a leisurely rate of 2 Hz.
 */

#include <Arduino_AdvancedAnalog.h>

AdvancedADC adc;
uint64_t last_millis = 0;
pin_size_t active_pins[AN_MAX_ADC_CHANNELS];
int num_active_pins = 0;
const int samples_per_round = 3;

void queryPins() {
    Serial.println("Enter pins to sample (number only, e.g. 3,4 for A3, and A4). Enter to repeat previous round.");

    int old_num_active_pins = num_active_pins;
    num_active_pins = 0;
    String buf;
    int c;
    do {
        c = Serial.read();
        if (c < 0) continue;

        if (c == ',' || c == '\n') {
            buf.trim();
            if (buf.length()) {
                active_pins[num_active_pins++] = buf.toInt() + A0;
                buf = String();
            }
        } else {
            buf += (char) c;
        }
    } while (!(c == '\n' || num_active_pins >= AN_MAX_ADC_CHANNELS));

    // No (valid) input? Repeat previous measurement cycle
    if (!num_active_pins) {
        num_active_pins = old_num_active_pins;
    }
}

void setup() {
    Serial.begin(9600);
    while (!Serial) {};
}

void loop() {
    queryPins();
    if (num_active_pins) {
        // Resolution, sample rate, number of samples per buffer per channel, queue depth, number of pins, array of pins.
        if (!adc.begin(AN_RESOLUTION_16, 2, 1, samples_per_round, num_active_pins, active_pins)) {
            Serial.println("Failed to start analog acquisition!");
            while (1);
        }

        for (int i = 0; i < samples_per_round; ++i) {
            while(!adc.available()) {};  // Your code could do something useful while waiting!

            SampleBuffer buf = adc.read();

            for (int i = 0; i < num_active_pins; ++i) {
                Serial.print(buf[i]);
                Serial.print(" ");
            }
            Serial.println();

            // Release the buffer to return it to the pool.
            buf.release();
        }

        adc.stop();
    }
}
