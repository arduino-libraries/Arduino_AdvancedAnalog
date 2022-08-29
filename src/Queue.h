#ifndef __QUEUE_H__
#define __QUEUE_H__
#include "Arduino.h"

template <class T> class Queue {
    private:
        T head;
        T tail;
        size_t _size;

    public:
        Queue(): head(nullptr), tail(nullptr), _size(0) {

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
#endif //__QUEUE_H__
