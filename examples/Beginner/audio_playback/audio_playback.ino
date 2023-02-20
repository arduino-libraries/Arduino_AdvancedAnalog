/*
 * GIGA R1 - Audio Playback
 * Simple wav format audio playback via 12-Bit DAC output by reading from a USB drive.
*/

#include <USBHostMbed5.h>
#include <DigitalOut.h>
#include <FATFileSystem.h>

// AdvancedDAC library is included inside wav_seeker library
#include "wav_seeker.h"
AdvancedDAC dac1(A12);

USBHostMSD msd;
mbed::FATFileSystem usb("USB_DRIVE_NAME");

void setup() {
  Serial.begin(115200);
  while (!Serial);

  pinMode(PA_15, OUTPUT);
  digitalWrite(PA_15, HIGH);

  delay(2500);
  Serial.println("Starting USB File Read example...");

  while (!msd.connect()) {
    delay(1000);
  }

  Serial.println("Mounting USB device...");
  int err =  usb.mount(&msd);
  if (err) {
    Serial.print("Error mounting USB device ");
    Serial.println(err);
    while (1);
  }

  // Read
  Serial.print("read done ");
  mbed::fs_file_t file;
  struct dirent *ent;
  int dirIndex = 0;
  int res = 0;
  Serial.println("Open file..");
  
  // 16-bit PCM Mono 16kHz realigned noise reduction
  FILE *f = fopen("/USB_DRIVE_NAME/AUDIO_SAMPLE.wav", "r+");

  // Crucial (from mBed)
  wav_play_rl(f, dac1, false);
 
  // Close the file 
  Serial.println("File closing");
  fflush(stdout);
  err = fclose(f);
  if (err < 0) {
    Serial.print("fclose error: ");
    Serial.print(strerror(errno));
    Serial.print(" (");
    Serial.print(-errno);
    Serial.print(")");
  } else {
    Serial.println("File closed!");
  } 
}

void loop() {}