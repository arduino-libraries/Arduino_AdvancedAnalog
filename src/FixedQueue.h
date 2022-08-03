#include "Arduino.h"

template <class T> class FixedQueue {
    private:
        size_t size;
        T *buffer;
        volatile size_t _head;
        volatile size_t _tail;
    
    private:
        size_t next_pos(size_t x) {
            return (((x) + 1) % (size));
        }

    public:
        FixedQueue(size_t size): size(size), _head(0), _tail(0) {
            buffer = (T *) malloc(size * sizeof(T));
        }

        ~FixedQueue() {
            if (buffer) {
                free(buffer);
            }
        }

        void reset() {
            _head = _tail = 0;
        }

        size_t empty() {
            return _head == _tail;
        }

        void push(T &value) {
            if (buffer && next_pos(_tail) != _head) {
                buffer[_tail] = value;
                _tail = next_pos(_tail);
            }
        }

        T pop() {
            T value;
            if (buffer && _head != _tail) {
                value = buffer[_head];
                _head = next_pos(_head);
            }
            return value;
        }
};
