/*
 * Based on mBed's repo, algorithm based of the wave_player library by Steve Ravet
 * https://os.mbed.com/users/sparkfun/code/wave_player/
 * Adapted to GIGA R1 
*/

#include "wav_seeker.h"

unsigned short DAC_fifo[256];
short DAC_wptr;
volatile short DAC_rptr;
short DAC_on;

typedef struct uFMT_STRUCT {
  short comp_code;
  short num_channels;
  unsigned sample_rate;
  unsigned avg_Bps;
  short block_align;
  short sig_bps;
} FMT_STRUCT;

void wav_play_rl(FILE *wavefile, AdvancedDAC &dac_out, bool verbosity){
  unsigned chunk_id,chunk_size,channel;
  unsigned data,samp_int,i;
  short unsigned dac_data;
  long long slice_value;
  char *slice_buf;
  short *data_sptr;
  unsigned char *data_bptr;
  int *data_wptr;
  FMT_STRUCT wav_format;
  long slice,num_slices;
  DAC_wptr=0;
  DAC_rptr=0;

  for (i=0;i<256;i+=2) {
    DAC_fifo[i]=0;
    DAC_fifo[i+1]=3000;
  }

  DAC_wptr=4;
  DAC_on=0;
  
  fread(&chunk_id,4,1,wavefile);
  fread(&chunk_size,4,1,wavefile);

  while (!feof(wavefile)) {
    if (verbosity)
      Serial.print(F("Read chunk ID ="));
      Serial.println(chunk_id);
      Serial.print(F("Read chunk size ="));
      Serial.println(chunk_size);

    switch (chunk_id) {
      case 0x46464952:
        fread(&data,4,1,wavefile);
        if (verbosity) {
          Serial.println(F("RIFF chunk"));
          Serial.print(F("  chunk size = "));
          Serial.println(chunk_size);
          Serial.print(F("  RIFF type = "));
          Serial.println(data);
        }
        break;
      case 0x20746d66:
        fread(&wav_format,sizeof(wav_format),1,wavefile);
        if (verbosity) {
          Serial.println(F("FORMAT chunk"));
          Serial.print(F("  chunk size = "));
          Serial.println(chunk_size);
          Serial.print(F("  compression code ="));
          Serial.println(wav_format.comp_code);
          Serial.print(F("  channels ="));
          Serial.println(wav_format.num_channels);
          Serial.print(F("  samples/sec ="));
          Serial.println(wav_format.sample_rate);
          Serial.print(F("  bytes/sec ="));
          Serial.println(wav_format.avg_Bps);
          Serial.print(F("  block align ="));
          Serial.println(wav_format.block_align);
          Serial.print(F("  bits per sample ="));
          Serial.println(wav_format.sig_bps);
        }

        // Initializing AdvancedAnalogRedux
        
        if (!dac_out.begin(AN_RESOLUTION_12, wav_format.sample_rate, 256, 512)) {
          Serial.println("Failed to start DAC1 !");
          while (1);
        } else{
          Serial.println(F("DAC1 Initialized"));          
        }

        if (chunk_size > sizeof(wav_format))
          fseek(wavefile,chunk_size-sizeof(wav_format),SEEK_CUR);
        break;
      case 0x61746164:
        slice_buf=(char *)malloc(wav_format.block_align);
        if (!slice_buf) {
          Serial.println(F("Unable to malloc slice buffer"));
          exit(1);
        }
        num_slices=chunk_size/wav_format.block_align;
        samp_int=1000000/(wav_format.sample_rate);
        
        if (verbosity) {
          Serial.println(F("DATA chunk"));
          Serial.print(F("  chunk size ="));
          Serial.println(chunk_size);
          Serial.print(F("  slices ="));
          Serial.println(num_slices);
          Serial.print(F("  Ideal sample interval= "));
          Serial.println((unsigned)(1000000.0/wav_format.sample_rate));
          Serial.print(F("  programmed interrupt tick interval ="));
          Serial.println(samp_int);
        }
        DAC_on=1; 
  
        for (slice=0;slice<num_slices;slice+=1) {
          while (dac_out.available()) {
            fread(slice_buf,wav_format.block_align,1,wavefile);
            if (feof(wavefile)) {
              Serial.println(F("Oops -- not enough slices in the wave file\n"));
              exit(1);
            }
            data_sptr=(short *)slice_buf;     // 16 bit samples
            data_bptr=(unsigned char *)slice_buf;     // 8 bit samples
            data_wptr=(int *)slice_buf;     // 32 bit samples
            slice_value=0;
            for (channel=0;channel<wav_format.num_channels;channel++) {
              switch (wav_format.sig_bps) {
                case 16:
                  if (verbosity)
                    Serial.print(F("16 bit channel ="));
                    Serial.println(channel);
                    Serial.print(F("  data = "));
                    Serial.println(data_sptr[channel]);
                  slice_value+=data_sptr[channel];
                  break;
                case 32:
                  if (verbosity)
                    Serial.print(F("32 bit channel ="));
                    Serial.println(channel);
                    Serial.print(F("  data = "));
                    Serial.println(data_wptr[channel]);
                  slice_value+=data_wptr[channel];
                  break;
                case 8:
                  if (verbosity)
                    Serial.print(F("8 bit channel ="));
                    Serial.println(channel);
                    Serial.print(F(" data = "));
                    Serial.println((int)data_bptr[channel]);
                  slice_value+=data_bptr[channel];
                  break;
              }
            }
            slice_value/=wav_format.num_channels;
            
            switch (wav_format.sig_bps) {
                          // Working for 8 bit for 12 bit output - accelerates in the end
              case 8:     slice_value<<=4;
                          break;
                          // WORKING 12 BIT - SCALING DOWN FROM 16 BIT TO 12 BIT (Applied from 16-Bit wav sample files )
              case 16:    slice_value = (((slice_value+32768)>>4) & 0xfff);
                          break;
                          // 32-Bit Case - Won't be used with GIGA as the highest resolution will be 12-Bit
              case 32:    slice_value>>=16;
                          slice_value+=32768;
                          break;
            }
            dac_data=(short unsigned)slice_value;
            if (verbosity)
              Serial.print(F("DAC-Data >>>>>>>>>>>>>>"));
              Serial.print(F("sample ="));
              Serial.println(slice);
              Serial.print(F("wptr ="));
              Serial.println(DAC_wptr);
              Serial.print(F("slice value ="));
              Serial.println((int)slice_value);
              Serial.print(F("dac_data ="));
              Serial.println(dac_data);
            DAC_fifo[DAC_wptr]=dac_data;

            // Custom_mode begin
            
            DAC_wptr=(DAC_wptr+1) & 0xff;
            // Conditional addition to switch between channels - FOR STEREO
            if (DAC_wptr >= 255){
              static size_t lut_offs = 0;
              
              Serial.println(F("Proceeding DAC ..."));
              // Get a free buffer for writing.
              SampleBuffer buf = dac_out.dequeue();
              // Write data to buffer.
              for (size_t i=0; i<buf.size(); i++, lut_offs++) {
                  buf[i] =  DAC_fifo[lut_offs % (sizeof(DAC_fifo)/sizeof(DAC_fifo[0]))];
              }

              // Writethe buffer to DAC.
              Serial.println(F("Writing to DAC ..."));
              dac_out.write(buf);
              
              DAC_wptr = 0;
            } else {
              Serial.println(F("Slicing"));
            }
            // Custom_mode end
          }
        }
        DAC_on=0;
        free(slice_buf);
        break;
      case 0x5453494c:
        if (verbosity)
          Serial.print(F("INFO chunk, size"));
          Serial.println(chunk_size);
        fseek(wavefile,chunk_size,SEEK_CUR);
        break;
      default:
        Serial.print(F("unknown chunk type ="));
        Serial.println(chunk_id);
        Serial.print(F("chunk size ="));
        Serial.println(chunk_size);
        data=fseek(wavefile,chunk_size,SEEK_CUR);
        break;
    }
    fread(&chunk_id,4,1,wavefile);
    fread(&chunk_size,4,1,wavefile);
  }
  Serial.println(F("Finished reading file"));
}