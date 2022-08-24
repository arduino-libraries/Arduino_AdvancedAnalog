#ifndef __QUEUE_H__
#define __QUEUE_H__
#include "Arduino.h"

template <class T> class LLQueue {
    private:
        T head;
        T tail;
        size_t _size;

    public:
        LLQueue(): head(nullptr), tail(nullptr), _size(0) {

        }

        size_t size() {
            return _size;
        }

        bool empty() {
            return !_size;
        }

        void push(T value) {
            _size++;
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
            _size--;
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
        size_t _size;
        size_t capacity;
        volatile size_t _head;
        volatile size_t _tail;
        std::unique_ptr<T[]> buffer;
    
    private:
        inline size_t next_pos(size_t x) {
            return (((x) + 1) % (capacity));
        }

    public:
        CircularQueue(size_t size=0):
            _size(0), capacity(size), _head(0), _tail(0), buffer(nullptr) {
            if (capacity) {
                buffer.reset(new T[capacity]);
            }
        }

        void reset() {
            _head = _tail = 0;
        }

        size_t size() {
            return _size;
        }

        size_t empty() {
            return _head == _tail;
        }

        void push(T value) {
            if (buffer && next_pos(_tail) != _head) {
                _size++;
                buffer[_tail] = value;
                _tail = next_pos(_tail);
            }
        }

        T pop() {
            if (buffer && _head != _tail) {
                _size--;
                T value = buffer[_head];
                _head = next_pos(_head);
                return value;
            }
            return T();
        }
};
#endif //__QUEUE_H__
