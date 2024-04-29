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

#include "Arduino.h"
#include "WavReader.h"

WavReader::~WavReader() {
    stop();
}

int WavReader::begin(const char *path, size_t n_samples, size_t n_buffers, bool loop) {
    this->loop = loop;

    if ((file = fopen(path, "rb")) == nullptr) {
        return 0;
    }

    // Read file header
    fread(&header, sizeof(header), 1, file);

    // Add more sanity checks if needed.
    if (memcmp(header.chunk_id, "RIFF", 4) != 0 ||
        memcmp(header.format, "WAVEfmt", 7) != 0 ||
        ((sizeof(Sample) * 8) < header.bits_per_sample) ||
        (n_samples * header.num_channels) > sample_count()) {
        stop();
        return 0;
    }

    // Allocate the DMA buffer pool.
    pool = new DMAPool<Sample>(n_samples, header.num_channels, n_buffers);
    if (pool == nullptr) {
        stop();
        return 0;
    }
    return 1;
}

void WavReader::stop() {
    if (file) {
        fclose(file);
    }
    if (pool) {
        delete pool;
    }
    pool = nullptr;
    file = nullptr;
}

bool WavReader::available() {
    if (file != nullptr && pool != nullptr) {
        return pool->writable();
    }
    return false;
}

DMABuffer<Sample> &WavReader::read() {
    while (!available()) {
        __WFI();
    }

    DMABuffer<Sample> *buf = pool->alloc(DMA_BUFFER_WRITE);
    size_t offset = 0;
    Sample *rawbuf = buf->data();
    size_t n_samples = buf->size();

    while (offset < n_samples) {
        offset += fread(&rawbuf[offset], sizeof(Sample), n_samples - offset, file);
        if (offset < n_samples) {
            if (loop) {
                rewind();
            } else {
                for (size_t i=offset; i<n_samples; i++) {
                    rawbuf[i] = 0;
                }
                fclose(file);
                file = nullptr;
                break;
            }
        }
    }
    buf->clr_flags();
    buf->set_flags(DMA_BUFFER_READ);
    return *buf;
}

int WavReader::rewind() {
    if (file == nullptr) {
        return 0;
    }
    if (fseek(file, sizeof(WavHeader), SEEK_SET) == -1) {
        return 0;
    }
    return 1;
}
