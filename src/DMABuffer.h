#include "Arduino.h"
#include "CircularQueue.h"

#ifndef __SCB_DCACHE_LINE_SIZE
#define __SCB_DCACHE_LINE_SIZE  32
#endif

template <size_t A> class AlignedAlloc {
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

template <class, size_t> class DMABufferPool;

template <class T, size_t A=__SCB_DCACHE_LINE_SIZE> class DMABuffer {
    typedef DMABufferPool<T, A> Pool;

    private:
        Pool *pool;
        size_t sz;
        T *ptr[2];
        size_t index;

    public:
        DMABuffer(): DMABuffer(nullptr, 0, nullptr, false) {

        }

        DMABuffer(size_t size, bool dblbuf): DMABuffer(nullptr, size, nullptr, dblbuf) {

        }

        DMABuffer(Pool *pool, size_t size, T *mem): DMABuffer(pool, size, mem, false) {

        }

        DMABuffer(Pool *pool, size_t size, T *mem, bool dblbuf):
            pool(pool), sz(size), ptr{mem, nullptr}, index(0) {
            if (mem != nullptr && dblbuf == true) {
                // Double buffer with pool-managed memory.
                ptr[1] = mem + (size / 2);
            } else if (mem == nullptr && size != 0) {
                // Unmanaged buffer with self-allocated memory.
                size_t bufsize = AlignedAlloc<A>::round(size * sizeof(T));
                ptr[0] = (T *) AlignedAlloc<A>::malloc(bufsize);
                if (dblbuf) {
                    ptr[1] = (T *) AlignedAlloc<A>::malloc(bufsize);
                }
            }
        }

        ~DMABuffer() {
            if (pool == nullptr) {
                AlignedAlloc<A>::free(ptr[0]);
                AlignedAlloc<A>::free(ptr[1]);
            }
        }

        T *data() {
            return ptr[index];
        }

        size_t size() {
            return sz;
        }

        size_t bytes() {
            return sz * sizeof(T);
        }

        void flush() {
            if (ptr) {
                SCB_InvalidateDCache_by_Addr(data(), bytes());
            }
        }

        T *swap() {
            if (ptr[1]) {
                index ^= 1;
            }
            return data();
        }

        void release() {
            if (pool) {
                pool->release(*this);
            }
        }

        T operator[](size_t i) {
            if (ptr && i < size()) {
                return data()[i];
            }
            return -1;
        }
};

template <class T, size_t A=__SCB_DCACHE_LINE_SIZE> class DMABufferPool {
    private:
        CircularQueue<DMABuffer<T>> freeq;
        CircularQueue<DMABuffer<T>> readyq;
        std::unique_ptr<uint8_t, decltype(&AlignedAlloc<A>::free)> pool;

    public:
        DMABufferPool(): pool(nullptr, AlignedAlloc<A>::free) {

        }

        DMABufferPool(size_t n_samples, size_t n_buffers):
            freeq(n_buffers), readyq(n_buffers), pool(nullptr, AlignedAlloc<A>::free) {

            size_t bufsize = AlignedAlloc<A>::round(n_samples * sizeof(T));

            // This pointer will be free'd with aligned_free in the destructor.
            pool.reset((uint8_t *) AlignedAlloc<A>::malloc(n_buffers * bufsize));

            // Init DMA buffers using aligned pointer to dma buffers memory.
            for (size_t i=0; i<n_buffers; i++) {
                DMABuffer<T> buf(this, n_samples, (T *) &pool.get()[i * bufsize]);
                freeq.push(buf);
            }
        }

        bool writable() {
            return !freeq.empty();
        }

        bool readable() {
            return !readyq.empty();
        }

        DMABuffer<T> allocate() {
            // Get a DMA buffer from the free queue.
            if (!freeq.empty()) {
                return freeq.pop();
            }
            return DMABuffer<T>();
        }

        void release(DMABuffer<T> &buf) {
            // Return DMA buffer to the free queue.
            freeq.push(buf);
        }

        void enqueue(DMABuffer<T> &buf) {
            // Add DMA buffer to the ready queue.
            readyq.push(buf);
        }

        DMABuffer<T> dequeue() {
            // Return a DMA buffer from the ready queue.
            if (!readyq.empty()) {
                return readyq.pop();
            }
            return DMABuffer<T>();
        }
};
