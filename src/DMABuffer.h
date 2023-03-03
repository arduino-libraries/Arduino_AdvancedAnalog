/*
  This file is part of the Arduino_AdvancedAnalog library.
  Copyright (c) 2023 Arduino SA. All rights reserved.

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

#ifndef __DMA_BUFFER_H__
#define __DMA_BUFFER_H__

#include "Arduino.h"
#include "Queue.h"

#ifndef __SCB_DCACHE_LINE_SIZE
#define __SCB_DCACHE_LINE_SIZE  32
#endif

template <size_t A> class AlignedAlloc {
    // AlignedAlloc allocates extra memory before the aligned memory start, and uses that
    // extra memory to stash the pointer returned from malloc. Note memory allocated with
    // Aligned::alloc must be free'd later with Aligned::free.
    public:
        static void *malloc(size_t size) {
            void **ptr, *stashed;
            size_t offset = A - 1 + sizeof(void *);
            if ((A % 2) || !((stashed = ::malloc(size + offset)))) {
                return nullptr;
            }
            ptr = (void **) (((uintptr_t) stashed + offset) & ~(A - 1));
            ptr[-1] = stashed;
            return ptr;
        }

        static void free(void *ptr) {
            if (ptr != nullptr) {
                ::free(((void **) ptr)[-1]);
            }
        }

        static size_t round(size_t size) {
            return ((size + (A-1)) & ~(A-1));
        }
};

enum {
    DMA_BUFFER_DISCONT  = (1 << 0),
    DMA_BUFFER_INTRLVD  = (1 << 1),
};

template <class, size_t> class DMABufferPool;

template <class T, size_t A=__SCB_DCACHE_LINE_SIZE> class DMABuffer {
    typedef DMABufferPool<T, A> Pool;

    private:
        Pool *pool;
        size_t n_samples;
        size_t n_channels;
        T *ptr;
        uint32_t ts;
        uint32_t flags;

    public:
        DMABuffer *next;

        DMABuffer(Pool *pool=nullptr, size_t samples=0, size_t channels=0, T *mem=nullptr):
            pool(pool), n_samples(samples), n_channels(channels), ptr(mem), ts(0), flags(0), next(nullptr) {
        }

        T *data() {
            return ptr;
        }

        size_t size() {
            return n_samples * n_channels;
        }

        size_t bytes() {
            return n_samples * n_channels * sizeof(T);
        }

        void flush() {
            #if __DCACHE_PRESENT
            if (ptr) {
                SCB_CleanDCache_by_Addr(data(), bytes());
            }
            #endif
        }

        void invalidate() {
            #if __DCACHE_PRESENT
            if (ptr) {
                SCB_InvalidateDCache_by_Addr(data(), bytes());
            }
            #endif
        }

        uint32_t timestamp() {
            return ts;
        }

        void timestamp(uint32_t ts) {
            this->ts = ts;
        }

        uint32_t channels() {
            return n_channels;
        }

        void release() {
            if (pool && ptr) {
                pool->release(this);
            }
        }

        void setflags(uint32_t f) {
            flags |= f;
        }

        bool getflags(uint32_t f) {
            return flags & f;
        }

        void clrflags(uint32_t f=0xFFFFFFFFU) {
            flags &= (~f);
        }

        T& operator[](size_t i)
        {
            assert(ptr && i < size());
            return ptr[i];
        }

        const T& operator[](size_t i) const
        {
            assert(ptr && i < size());
            return ptr[i];
        }

        operator bool() const {
            return (ptr != nullptr);
        }
};

template <class T, size_t A=__SCB_DCACHE_LINE_SIZE> class DMABufferPool {
    private:
        Queue<DMABuffer<T>*> wr_queue;
        Queue<DMABuffer<T>*> rd_queue;
        std::unique_ptr<DMABuffer<T>[]> buffers;
        std::unique_ptr<uint8_t, decltype(&AlignedAlloc<A>::free)> pool;

    public:
        DMABufferPool(size_t n_samples, size_t n_channels, size_t n_buffers):
            buffers(nullptr), pool(nullptr, AlignedAlloc<A>::free) {
            // Round up to next multiple of alignment.
            size_t bufsize = AlignedAlloc<A>::round(n_samples * n_channels * sizeof(T));
            if (bufsize) {
                // Allocate non-aligned memory for the DMA buffers objects.
                buffers.reset(new DMABuffer<T>[n_buffers]);

                // Allocate aligned memory pool for DMA buffers pointers.
                pool.reset((uint8_t *) AlignedAlloc<A>::malloc(n_buffers * bufsize));
            }
            if (buffers && pool) {
                // Init DMA buffers using aligned pointers to dma buffers memory.
                for (size_t i=0; i<n_buffers; i++) {
                    buffers[i] = DMABuffer<T>(this, n_samples, n_channels, (T *) &pool.get()[i * bufsize]);
                    wr_queue.push(&buffers[i]);
                }
            }
        }

        size_t writable() {
            return wr_queue.size();
        }

        size_t readable() {
            return rd_queue.size();
        }

        DMABuffer<T> *allocate() {
            // Get a DMA buffer from the free queue.
            return wr_queue.pop();
        }

        void release(DMABuffer<T> *buf) {
            // Return DMA buffer to the free queue.
            buf->clrflags();
            wr_queue.push(buf);
        }

        void enqueue(DMABuffer<T> *buf) {
            // Add DMA buffer to the ready queue.
            rd_queue.push(buf);
        }

        DMABuffer<T> *dequeue() {
            // Return a DMA buffer from the ready queue.
            return rd_queue.pop();
        }
};
#endif //__DMA_BUFFER_H__
