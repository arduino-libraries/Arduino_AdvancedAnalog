// This example demonstrates how to playback a WAV file with I2S.
// To run this sketch, rename 'USB_DRIVE' to the name of your USB
// stick drive, and copy the provided audio sample to the drive.
#include <Arduino_AdvancedAnalog.h>
#include <Arduino_USBHostMbed5.h>
#include <FATFileSystem.h>
#include <DigitalOut.h>
#include <WavReader.h>

USBHostMSD msd;
mbed::FATFileSystem usb("USB_DRIVE");

WavReader wav;
// WS, CK, SDI, SDO, MCK
AdvancedI2S i2s(PG_10, PG_11, PG_9, PB_5, PC_4);
#define N_SAMPLES   (512)

void setup() {
    Serial.begin(9600);
    while (!Serial) {

    }

    // Enable power for HOST USB connector.
    pinMode(PA_15, OUTPUT);
    digitalWrite(PA_15, HIGH);

    Serial.println("Please connect a USB stick to the USB host port...");
    while (!msd.connect()) {
        delay(100);
    }

    Serial.println("Mounting USB device...");
    int const rc_mount = usb.mount(&msd);
    if (rc_mount) {
        Serial.print("Error mounting USB device ");
        Serial.println(rc_mount);
        while (1);
    }

    Serial.println("Opening audio file ...");
    if (!wav.begin("/USB_DRIVE/AUDIO_SAMPLE.wav", N_SAMPLES, 1, true)) {
        Serial.print("Error opening audio file: ");
        while (1);
    }

    // Resolution, sample rate, number of samples per channel, queue depth.
    if (!i2s.begin(AN_I2S_MODE_OUT, wav.sample_rate(), N_SAMPLES, 64)) {
        Serial.println("Failed to start I2S");
        while (1);
    }
    Serial.println("Playing audio file ...");
}

void loop() {
    if (i2s.available() && wav.available()) {
        // Get a free I2S buffer for writing.
        SampleBuffer i2s_buf = i2s.dequeue();

        // Read a PCM samples buffer from the wav file.
        SampleBuffer pcm_buf = wav.read();

        // Write PCM samples to the I2S buffer.
        for (size_t i = 0; i < N_SAMPLES * wav.channels(); i++) {
            // Note I2S buffers are always 2 channels.
            if (wav.channels() == 2) {
                i2s_buf[i] = pcm_buf[i];
            } else {
                i2s_buf[(i * 2) + 0] = ((int16_t) pcm_buf[i] / 2);
                i2s_buf[(i * 2) + 1] = ((int16_t) pcm_buf[i] / 2);
            }
        }  

        // Write back the I2S buffer.
        i2s.write(i2s_buf);

        // Release the PCM buffer.
        pcm_buf.release();
    }
}
