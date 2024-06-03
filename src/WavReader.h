/*
  This file is part of the Arduino_AdvancedAnalog library.
  Copyright (c) 2024 Arduino SA. All rights reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
*/

#ifndef __ADVANCED_WAV_READER_H__
#define __ADVANCED_WAV_READER_H__

#include "AdvancedAnalog.h"

class WavReader {
    typedef struct {
        char chunk_id[4];
        unsigned int chunk_size;
        char format[4];
        char subchunk1_id[4];
        unsigned int subchunk1_size;
        unsigned short audio_format;
        unsigned short num_channels;
        unsigned int sample_rate;
        unsigned int byte_rate;
        unsigned short block_align;
        unsigned short bits_per_sample;
        char subchunk2_id[4];
        unsigned int subchunk2_size;
    } WavHeader;

    private:
        FILE *file;
        bool loop;
        WavHeader header;
        DMAPool<Sample> *pool;

    public:
        WavReader(): file(nullptr), loop(false), pool(nullptr) {
        }
        ~WavReader();
        size_t channels() {
            return header.num_channels;
        }

        size_t resolution() {
            return header.bits_per_sample;
        }

        size_t sample_rate() {
            return header.sample_rate;
        }

        size_t sample_count() {
            return (header.subchunk2_size * 8) / header.bits_per_sample;
        }

        int begin(const char *path, size_t n_samples, size_t n_buffers, bool loop=false);
        void stop();
        bool available();
        SampleBuffer read();
        int rewind();
};
#endif // __ADVANCED_WAV_READER_H__
