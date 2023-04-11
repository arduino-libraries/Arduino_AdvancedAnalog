// This example generates different waveforms based on user input on A12/DAC1.

#include <Arduino_AdvancedAnalog.h>
#include <mbed_stats.h>

#define N_SAMPLES           (64)
#define DEFAULT_FREQUENCY   (32000)

AdvancedDAC dac1(A12);
uint8_t SAMPLES_BUFFER[N_SAMPLES];

uint32_t get_current_heap() {
    mbed_stats_heap_t heap_stats;
    mbed_stats_heap_get(&heap_stats);
    return heap_stats.current_size;
}

void print_menu() {
    Serial.println();
    Serial.println("Enter a command:");
    Serial.println("t: Triangle wave");
    Serial.println("q: Square wave");
    Serial.println("s: Sine wave");
    Serial.println("r: Sawtooth wave");
    Serial.println("k: stop DAC");
    Serial.println("+: Increase frequency");
    Serial.println("-: Decrease frequency");
}

void generate_waveform(int cmd) {
    static bool dac_started = false;
    static size_t dac_frequency = DEFAULT_FREQUENCY;
    static size_t starting_heap = get_current_heap();

    switch (cmd) {
        case 't':
            // Triangle wave
            Serial.print("Waveform: Triangle ");
            for (int i=0, j=N_SAMPLES-1; i<N_SAMPLES; i++, j--){
                SAMPLES_BUFFER[i] = abs((i - j) * 256 / N_SAMPLES);
            }
            break;

        case 'q':
            // Square wave
            Serial.print("Waveform: Square ");
            for (int i=0; i<N_SAMPLES; i++){
                SAMPLES_BUFFER[i] = i < (N_SAMPLES / 2) ? 0 : 255;
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
            Serial.print("Waveform: Sawtooth ");
            for (int i=0; i<N_SAMPLES; i++){
                SAMPLES_BUFFER[i] = i * (256 / N_SAMPLES);
            }
            break;

        case '+':
        case '-':
            Serial.print("Current frequency: ");

            if (cmd == '+' && dac_frequency < 128000) {
                dac_frequency *= 2;
            } else if (cmd == '-' && dac_frequency > 1000) {
                dac_frequency /= 2;
            } else {
                break;
            }

            // Change frequency.
            dac1.frequency(dac_frequency * N_SAMPLES);
            break;

        case 'k':
            dac1.stop();
            dac_started = false;
            break;

        default:
            Serial.print("Unknown command ");
            Serial.println((char) cmd);
            return;
    }

    if (cmd == 'k') {
        Serial.println("DAC stopped!");
        print_menu();
    } else {
        Serial.print(dac_frequency/1000);
        Serial.println("KHz");

        if (dac_started == false) {
            // Initialize and start the DAC.
            if (!dac1.begin(AN_RESOLUTION_8, dac_frequency * N_SAMPLES, N_SAMPLES, 32)) {
                Serial.println("Failed to start DAC1 !");
                while (1);
            }
            dac_started = true;
        }
    }

    Serial.print("Used memory: ");
    Serial.print(get_current_heap() - starting_heap);
    Serial.println(" bytes");
}

void setup() {
    Serial.begin(115200);

    while (!Serial) {

    }

    // Print list of commands.
    print_menu();

    // Start generating a sine wave.
    generate_waveform('s');
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
