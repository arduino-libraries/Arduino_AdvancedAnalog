#include "Arduino.h"
#include "FixedQueue.h"

#ifndef __SCB_DCACHE_LINE_SIZE
#define __SCB_DCACHE_LINE_SIZE  32
#endif

template <class, size_t> class DMABufferPool;

template <size_t A> class AlignedAlloc {
    public:
        static void *malloc(size_t size) {
            void **ptr, *stashed;
            size_t offset = A - 1 + sizeof(void *);
            if ((A % 2) || !((stashed = ::malloc(size + offset)))) {
                return NULL;
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
};

template <class T, size_t A=__SCB_DCACHE_LINE_SIZE> class DMABuffer {
    typedef DMABufferPool<T, A> Pool;

    private:
        Pool *pool;
        size_t sz;
        T *ptr;
        T *buf;
        bool dblbuf;
        bool allocated;

    public:
        DMABuffer(): DMABuffer(nullptr, 0, nullptr, false) {

        }

        DMABuffer(size_t size, bool dblbuf): DMABuffer(nullptr, size, nullptr, dblbuf) {

        }

        DMABuffer(Pool *pool, size_t size): DMABuffer(pool, size, nullptr, false) {

        }

        DMABuffer(Pool *pool, size_t size, T *mem): DMABuffer(pool, size, mem, false) {

        }

        DMABuffer(Pool *pool, size_t size, T *mem, bool dblbuf):
            pool(pool), sz(size), ptr(mem), buf(mem), dblbuf(dblbuf), allocated(false) {
            if (size != 0 && mem == nullptr) {
                allocated = true;
                // TODO round up buffer size to the next multiple of A.
                ptr = buf = (T *) AlignedAlloc<A>::malloc(size * ((dblbuf) ? 2 : 1) * sizeof(T));
            }
        }

        ~DMABuffer() {
            if (allocated) { // Memory was allocated by the DMABuffer.
                AlignedAlloc<A>::free(buf);
            }
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
                SCB_InvalidateDCache_by_Addr(ptr, bytes());
            }
        }

        T *swap() {
            T *ret = ptr;
            if (ptr && dblbuf) {
                if (ptr != buf) {
                    ptr = buf;
                } else {
                    ptr += size();
                }
            }
            return ret;
        }

        void release() {
            if (pool) {
                pool->release(*this);
            }
        }

        T operator[](size_t i) {
            if (ptr && i < size()) {
                return ptr[i];
            }
            return -1;
        }
};

template <class T, size_t A=__SCB_DCACHE_LINE_SIZE> class DMABufferPool {
    private:
        uint8_t *pool;
        FixedQueue<DMABuffer<T>> freeq;
        FixedQueue<DMABuffer<T>> readyq;

    public:
        DMABufferPool(size_t n_samples, size_t n_buffers):
            pool(nullptr), freeq(n_buffers), readyq(n_buffers) {

            // TODO round up buffer size to the next multiple of A.
            size_t bufsize = n_samples * sizeof(T);

            // This pointer must be free'd with aligned_free in the destructor.
            pool = (uint8_t *) AlignedAlloc<A>::malloc(n_buffers * bufsize);

            // Init DMA buffers using aligned pointer to dma buffers memory.
            for (size_t i=0; i<n_buffers; i++) {
                DMABuffer<T> buf(this, n_samples, (T *) &pool[i * bufsize]);
                freeq.push(buf);
            }
        }

        ~DMABufferPool() {
            if (pool) {
                AlignedAlloc<A>::free(pool);
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
