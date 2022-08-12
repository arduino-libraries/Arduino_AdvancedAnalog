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

template <class T, size_t A=__SCB_DCACHE_LINE_SIZE> class DBuffer {
    private:
        size_t idx;
        size_t sz;
        T *ptr;

    public:
        DBuffer(size_t size): idx(0), sz(size), ptr(nullptr) {
            size_t bufsize = AlignedAlloc<A>::round(size * sizeof(T));
            ptr = (T *) AlignedAlloc<A>::malloc(bufsize * 2);
        }

        ~DBuffer() {
            AlignedAlloc<A>::free(ptr);
        }

        void swap() {
            idx ^= 1;
        }

        T *data() {
            return &ptr[idx * sz];
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
};

template <class, size_t> class DMABufPool;

template <class T, size_t A=__SCB_DCACHE_LINE_SIZE> class DMABuffer {
    typedef DMABufPool<T, A> Pool;

    private:
        Pool *pool;
        size_t sz;
        T *ptr;
        uint32_t ts;

    public:
        DMABuffer *next;

        DMABuffer(Pool *pool=nullptr, size_t size=0, T *mem=nullptr):
            pool(pool), sz(size), ptr(mem), ts(0), next(nullptr) {
        }

        T *data() {
            return ptr;
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

        uint32_t timestamp() {
            return ts;
        }

        void timestamp(uint32_t ts) {
            this->ts = ts;
        }

        void release() {
            if (pool && ptr) {
                pool->release(this);
            }
        }

        T operator[](size_t i) {
            if (ptr && i < size()) {
                return data()[i];
            }
            return -1;
        }
};

template <class T, size_t A=__SCB_DCACHE_LINE_SIZE> class DMABufPool {
    private:
        LLQueue<DMABuffer<T>*> freeq;
        LLQueue<DMABuffer<T>*> readyq;
        std::unique_ptr<DMABuffer<T>[]> buffers;
        std::unique_ptr<uint8_t, decltype(&AlignedAlloc<A>::free)> pool;

    public:
        DMABufPool(size_t n_samples, size_t n_buffers):
            buffers(nullptr), pool(nullptr, AlignedAlloc<A>::free) {

            size_t bufsize = AlignedAlloc<A>::round(n_samples * sizeof(T));

            // Allocate non-aligned memory for the DMA buffers objects.
            buffers.reset(new DMABuffer<T>[n_buffers]);

            // Allocate aligned memory pool for DMA buffers pointers.
            pool.reset((uint8_t *) AlignedAlloc<A>::malloc(n_buffers * bufsize));

            // Init DMA buffers using aligned pointers to dma buffers memory.
            for (size_t i=0; i<n_buffers; i++) {
                buffers[i] = DMABuffer<T>(this, n_samples, (T *) &pool.get()[i * bufsize]);
                freeq.push(&buffers[i]);
            }
        }

        bool writable() {
            return !freeq.empty();
        }

        bool readable() {
            return !readyq.empty();
        }

        DMABuffer<T> *allocate() {
            // Get a DMA buffer from the free queue.
            return freeq.pop();
        }

        void release(DMABuffer<T> *buf) {
            // Return DMA buffer to the free queue.
            freeq.push(buf);
        }

        void enqueue(DMABuffer<T> *buf) {
            // Add DMA buffer to the ready queue.
            readyq.push(buf);
        }

        DMABuffer<T> *dequeue() {
            // Return a DMA buffer from the ready queue.
            return readyq.pop();
        }
};
