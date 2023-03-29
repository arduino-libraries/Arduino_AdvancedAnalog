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

AdvancedDAC dac1(A12);

USBHostMSD msd;
mbed::FATFileSystem usb("USB_DRIVE");

FILE * file = nullptr;
int sample_size = 0;
int samples_count = 0;


void setup()
{
  Serial.begin(115200);
  while (!Serial);

  /* Enable power for HOST USB connector. */
  pinMode(PA_15, OUTPUT);
  digitalWrite(PA_15, HIGH);

  Serial.println("Please connect a USB stick to the GIGA's USB port ...");
  while (!msd.connect()) delay(100);

  Serial.println("Mounting USB device ...");
  int const rc_mount = usb.mount(&msd);
  if (rc_mount)
  {
    Serial.print("Error mounting USB device ");
    Serial.println(rc_mount);
    return;
  }

  Serial.println("Opening audio file ...");

  /* 16-bit PCM Mono 16kHz realigned noise reduction */
  file = fopen("/USB_DRIVE/AUDIO_SAMPLE.wav", "rb");
  if (file == nullptr)
  {
    Serial.print("Error opening audio file: ");
    Serial.println(strerror(errno));
    return;
  }

  Serial.println("Reading audio header ...");
  struct wav_header_t
  {
    char chunkID[4]; //"RIFF" = 0x46464952
    unsigned long chunkSize; //28 [+ sizeof(wExtraFormatBytes) + wExtraFormatBytes] + sum(sizeof(chunk.id) + sizeof(chunk.size) + chunk.size)
    char format[4]; //"WAVE" = 0x45564157
    char subchunk1ID[4]; //"fmt " = 0x20746D66
    unsigned long subchunk1Size; //16 [+ sizeof(wExtraFormatBytes) + wExtraFormatBytes]
    unsigned short audioFormat;
    unsigned short numChannels;
    unsigned long sampleRate;
    unsigned long byteRate;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
  };

  wav_header_t header;
  fread(&header, sizeof(header), 1, file);

  Serial.println("WAV File Header read:");
  char msg[64] = {0};
  snprintf(msg, sizeof(msg), "File Type: %s", header.chunkID);
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "File Size: %ld", header.chunkSize);
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "WAV Marker: %s", header.format);
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "Format Name: %s", header.subchunk1ID);
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "Format Length: %ld", header.subchunk1Size);
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "Format Type: %hd", header.audioFormat);
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "Number of Channels: %hd", header.numChannels);
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "Sample Rate: %ld", header.sampleRate);
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "Sample Rate * Bits/Sample * Channels / 8: %ld", header.byteRate);
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "Bits per Sample * Channels / 8: %hd", header.blockAlign);
  Serial.println(msg);
  snprintf(msg, sizeof(msg), "Bits per Sample: %hd", header.bitsPerSample);
  Serial.println(msg);

  /* Find the data section of the WAV file. */
  struct chunk_t
  {
    char ID[4];
    unsigned long size;
  };

  chunk_t chunk;
  snprintf(msg, sizeof(msg), "id\t" "size");
  Serial.println(msg);
  /* Find data chunk. */
  while (true)
  {
    fread(&chunk, sizeof(chunk), 1, file);
    snprintf(msg, sizeof(msg), "%c%c%c%c\t" "%li", chunk.ID[0], chunk.ID[1], chunk.ID[2], chunk.ID[3], chunk.size);
    Serial.println(msg);
    if (*(unsigned int *) &chunk.ID == 0x61746164)
      break;
    /* Skip chunk data bytes. */
    fseek(file, chunk.size, SEEK_CUR);
  }

  /* Determine number of samples. */
  sample_size = header.bitsPerSample / 8;
  samples_count = chunk.size * 8 / header.bitsPerSample;
  snprintf(msg, sizeof(msg), "Sample size = %i", sample_size); Serial.println(msg);
  snprintf(msg, sizeof(msg), "Samples count = %i", samples_count); Serial.println(msg);

  /* Configure the advanced DAC. */
  if (!dac1.begin(AN_RESOLUTION_12, header.sampleRate, 256, 16))
  {
    Serial.println("Failed to start DAC1 !");
    return;
  }
}

void loop()
{
  if (dac1.available() && !feof(file))
  {
    /* Read data from file. */
    uint16_t sample_data[256] = {0};
    fread(sample_data, sample_size, 256, file);

    /* Get a free buffer for writing. */
    SampleBuffer buf = dac1.dequeue();

    /* Write data to buffer. */
    for (size_t i = 0; i < buf.size(); i++)
    {
      /* Scale down to 12 bit. */
      uint16_t const dac_val = ((static_cast<unsigned int>(sample_data[i])+32768)>>4) & 0x0fff;
      buf[i] = dac_val;
    }

    /* Write the buffer to DAC. */
    dac1.write(buf);
  }
}
