#include "Arduino.h"

template <class T> class LLQueue {
    private:
        T head;
        T tail;

    public:
        LLQueue(): head(nullptr), tail(nullptr) {

        }

        bool empty() {
            return head == nullptr;
        }

        void push(T value) {
            value->next = nullptr;
            if (head == nullptr) {
                head = value;
            }
            if (tail) {
                tail->next = value;
            }
            tail = value;
        }

        T pop() {
            T value = head;
            if (head) {
                head = head->next;
            }
            if (head == nullptr) {
                tail = nullptr;
            }
            return value;
        }
};

template <class T> class CircularQueue {
    private:
        size_t size;
        volatile size_t _head;
        volatile size_t _tail;
        std::unique_ptr<T[]> buffer;
    
    private:
        inline size_t next_pos(size_t x) {
            return (((x) + 1) % (size));
        }

    public:
        CircularQueue(size_t size=0):
            size(size), _head(0), _tail(0), buffer(nullptr) {
            if (size) {
                buffer.reset(new T[size]);
            }
        }

        void reset() {
            _head = _tail = 0;
        }

        size_t empty() {
            return _head == _tail;
        }

        void push(T value) {
            if (buffer && next_pos(_tail) != _head) {
                buffer[_tail] = value;
                _tail = next_pos(_tail);
            }
        }

        T pop() {
            if (buffer && _head != _tail) {
                T value = buffer[_head];
                _head = next_pos(_head);
                return value;
            }
            return T();
        }
};