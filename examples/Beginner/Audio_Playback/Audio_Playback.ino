/*
 * GIGA R1 - Audio Playback
 * Simple wav format audio playback via 12-Bit DAC output by reading from a USB drive.
 * In order for this sketch to work you need to rename 'USB_DRIVE' to the name of your USB stick drive.
*/

#include <USBHostMbed5.h>
#include <DigitalOut.h>
#include <FATFileSystem.h>

// AdvancedDAC library is included inside wav_seeker library
#include "wav_seeker.h"
AdvancedDAC dac1(A12);

USBHostMSD msd;
mbed::FATFileSystem usb("USB_DRIVE");

void setup()
{
  Serial.begin(115200);
  while (!Serial);

  pinMode(PA_15, OUTPUT);
  digitalWrite(PA_15, HIGH);

  Serial.println("Please connect a USB stick to the GIGA's USB port ...");

  while (!msd.connect())
    delay(1000);

  Serial.println("Mounting USB device ...");
  int const rc_mount = usb.mount(&msd);
  if (rc_mount)
  {
    Serial.print("Error mounting USB device ");
    Serial.println(rc_mount);
    return;
  }

  Serial.println("Opening audio file ...");
  
  // 16-bit PCM Mono 16kHz realigned noise reduction
  FILE * f = fopen("/USB_DRIVE/AUDIO_SAMPLE.wav", "r+");
  if (f == nullptr)
  {
    Serial.print("Error opening audio file: ");
    Serial.println(strerror(errno));
    return;
  }

  Serial.println("Playing audio file ...");
  wav_play_rl(f, dac1, false);
 
  Serial.println("Cleaning up ...");
  fclose(f);
}

void loop() {}
