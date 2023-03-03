// This example generates different waveforms based on user input on A12/DAC1.

#include <Arduino_AdvancedAnalog.h>

#define N_SAMPLES           (256)
#define DEFAULT_FREQUENCY   (16000)

AdvancedDAC dac1(A12);
uint8_t SAMPLES_BUFFER[N_SAMPLES];
size_t dac_frequency = DEFAULT_FREQUENCY;

void generate_waveform(int cmd) 
{   
    switch (cmd) {
        case 't':
            // Triangle wave
            Serial.print("Waveform: Triangle ");
            for (int i=0; i<N_SAMPLES; i++){
                SAMPLES_BUFFER[i] = abs((i % 255) - 127);
            }
            break;

        case 'q':
            // Square wave
            Serial.print("Waveform: Square ");
            for (int i=0; i<N_SAMPLES; i++){
                SAMPLES_BUFFER[i] = (i % 255) < 127 ? 127 : 0;
            }
            break;

        case 's':
            // Sine wave
            Serial.print("Waveform: Sine ");
            for (int i=0; i<N_SAMPLES; i++){
                SAMPLES_BUFFER[i] = sin(2 * 3.14 * (i / (float) N_SAMPLES)) * 127 + 127;
            }
            break;

        case 'r':
            // Sawtooth
            Serial.print("Waveform: Sawtooth");
            for (int i=0; i<N_SAMPLES; i++){
                SAMPLES_BUFFER[i] = i;
            }
            break;

        case '+':
        case '-':
            Serial.print("Current frequency: ");
            
            if (cmd == '+' && dac_frequency < 64000) {
              dac_frequency *= 2;
            } else if (cmd == '-' && dac_frequency > 1000) {
              dac_frequency /= 2;
            } else {
              break;
            }

            // Change frequency.
            dac1.frequency(dac_frequency * N_SAMPLES);
            break;
            
        default:
            Serial.print("Unknown command ");
            Serial.println((char) cmd);
            return;
    }
    
    Serial.print(dac_frequency/1000);
    Serial.println("KHz");
}

void setup() {
    Serial.begin(115200);

    while (!Serial) {

    }

    
    Serial.println("Enter a command:");
    Serial.println("t: Triangle wave");
    Serial.println("q: Square wave");
    Serial.println("s: Sine wave");
    Serial.println("r: Sawtooth wave");
    Serial.println("+: Increase frequency");
    Serial.println("-: Decrease frequency");
    
    generate_waveform('s');
    
    // DAC initialization
    if (!dac1.begin(AN_RESOLUTION_8, DEFAULT_FREQUENCY * N_SAMPLES, N_SAMPLES, 32)) {
        Serial.println("Failed to start DAC1 !");
        while (1);
    }
}

void loop() {
    if (Serial.available() > 0) {
        int cmd = Serial.read();
        if (cmd != '\n') {
          generate_waveform(cmd);
        }
    } 
    
    if (dac1.available()) {
        // Get a free buffer for writing.
        SampleBuffer buf = dac1.dequeue();

        // Write data to buffer.
        for (size_t i=0; i<buf.size(); i++) {
            buf[i] =  SAMPLES_BUFFER[i];
        }

        dac1.write(buf);
    }
}
