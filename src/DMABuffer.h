#include <vector>
#include <queue>
#include "Arduino.h"

#ifndef __SCB_DCACHE_LINE_SIZE
#define __SCB_DCACHE_LINE_SIZE  32
#endif
#define ALIGN_PTR(p, a) ((((uintptr_t) p) & (a-1)) ? (((uintptr_t)(p) + a) & ~(uintptr_t)(a-1)) : ((uintptr_t)p))

template <class T> class DMABuffer {
    private:
        size_t bytes;
        uint32_t flags;
        T *_data;

    public:
        DMABuffer(size_t size=0, T *data=nullptr): bytes(size), flags(0), _data(data) {
        }

        T *data() {
            return _data;
        }

        size_t size() {
            return bytes / sizeof(T);
        }

        void flush() {
            if (_data) {
                SCB_InvalidateDCache_by_Addr(_data, bytes);
            }
        }

        T operator[](size_t i) {
            if (_data && i < size()) {
                return _data[i];
            }
            return -1;
        }
};

template <class T, size_t Alignment=__SCB_DCACHE_LINE_SIZE> class DMABufferPool {
    private:
        size_t n_samples;
        size_t n_buffers;
        void *dmabuf_mem;
        std::queue<DMABuffer<T>> free_queue;
        std::queue<DMABuffer<T>> ready_queue;

    public:
        DMABufferPool(size_t n_buffers, size_t n_samples) {
            // TODO round up buffer size to the next multiple of alignment.
            size_t dmabuf_size = n_samples * sizeof(T);

            // This pointer is free'd in the destructor.
            dmabuf_mem = malloc(n_buffers * dmabuf_size + Alignment);

            // Init DMA buffers using aligned pointer to dma buffers memory.
            uint8_t *dmabuf_mem_align = (uint8_t *) ALIGN_PTR(dmabuf_mem, Alignment);
            for (size_t i=0; i<n_buffers; i++) {
                free_queue.push(DMABuffer<T>(dmabuf_size, (T *) &dmabuf_mem_align[i * dmabuf_size]));
                // This pre-allocates the queue's memory, which shouldn't shrink on pop().
                ready_queue.push(DMABuffer<T>());
            }

            while (!ready_queue.empty()) {
                ready_queue.pop();
            }
        }

        ~DMABufferPool() {
            free(dmabuf_mem);
        }

        bool available() {
            return !ready_queue.empty();
        }

        DMABuffer<T> allocate() {
            // Get a DMA buffer from the free_queue.
            DMABuffer<T> buf;
            if (!free_queue.empty()) {
                buf = free_queue.front();
                free_queue.pop();
            }
            return buf;
        }

        void release(DMABuffer<T> buf) {
            // Return DMA buffer to the free_queue.
            free_queue.push(buf);
        }

        void enqueue(DMABuffer<T> buf) {
            // Add DMA buffer to the ready queue.
            ready_queue.push(buf);
        }

        DMABuffer<T> dequeue() {
            // Return a DMA buffer from the ready queue.
            DMABuffer<T> buf;
            if (!ready_queue.empty()) {
                buf = ready_queue.front();
                ready_queue.pop();
                buf.flush();
            }
            return buf;
        }
};
