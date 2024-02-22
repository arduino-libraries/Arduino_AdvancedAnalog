/*
 * GIGA R1 - Audio Playback
 * Simple wav format audio playback via 12-Bit DAC output by reading from a USB drive.
 * In order for this sketch to work you need to rename 'USB_DRIVE' to the name of your USB stick drive.
 * Furthermore you need to store the provided audio file AUDIO_SAMPLE.wav on it.
*/

#include <Arduino_AdvancedAnalog.h>
#include <Arduino_USBHostMbed5.h>
#include <DigitalOut.h>
#include <FATFileSystem.h>

USBHostMSD msd;
mbed::FATFileSystem usb("USB_DRIVE");

WavReader wavreader;
AdvancedDAC dac1(A12);
#define N_SAMPLES   (512)

void setup() {
  Serial.begin(115200);
  while (!Serial) {
  }

  // Enable power for HOST USB connector.
  pinMode(PA_15, OUTPUT);
  digitalWrite(PA_15, HIGH);

  Serial.println("Please connect a USB stick to the USB host port ...");
  while (!msd.connect()) {
    delay(100);
  }

  Serial.println("Mounting USB device ...");
  int const rc_mount = usb.mount(&msd);
  if (rc_mount) {
    Serial.print("Error mounting USB device ");
    Serial.println(rc_mount);
    while (1);
  }

  Serial.println("Opening audio file ...");
  if (!wavreader.begin("/USB_DRIVE/AUDIO_SAMPLE.wav", N_SAMPLES, 1, false)) {
      Serial.print("Error opening audio file: ");
      while (1);
  }

  char msg[64] = {0};
  snprintf(msg, sizeof(msg), "Number of Channels: %hd", wavreader.channels()); Serial.println(msg);
  snprintf(msg, sizeof(msg), "Sample Rate: %ld", wavreader.sample_rate()); Serial.println(msg);
  snprintf(msg, sizeof(msg), "Bits per Sample: %hd", wavreader.resolution()); Serial.println(msg);
  snprintf(msg, sizeof(msg), "Number of Samples = %i", wavreader.sample_count()); Serial.println(msg);

  // Configure and start the DAC.
  if (!dac1.begin(AN_RESOLUTION_12, wavreader.sample_rate(), N_SAMPLES, 32)) {
    Serial.println("Failed to start DAC1 !");
    while (1);
  }
}

void loop() {
  if (dac1.available() && wavreader.available()) {
    // Get a free buffer for writing.
    SampleBuffer dacbuf = dac1.dequeue();

    // Read a samples buffer from the wav file.
    SampleBuffer pcmbuf = wavreader.read();

    // Process and write samples to the DAC buffer.
    for (size_t i=0; i<N_SAMPLES; i++) {
      // Map the samples to positive range and down scale to 12-bit.
      if (wavreader.channels() == 1) {
        dacbuf[i] = ((unsigned int) ((int16_t) pcmbuf[i] + 32768)) >> 4;
      } else {
        // If the file has two channels set the average.
        dacbuf[i] = ((unsigned int) ((((int16_t) pcmbuf[(i * 2)] + (int16_t) pcmbuf[(i * 2) + 1]) / 2) + 32768)) >> 4;
      }
    }

    // Write the buffer to DAC.
    dac1.write(dacbuf);
    pcmbuf.release();
  }
}
