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

#ifndef __QUEUE_H__
#define __QUEUE_H__

#include "Arduino.h"

template <class T> class Queue {
    private:
        size_t capacity;
        volatile size_t tail;
        volatile size_t head;
        std::unique_ptr<T[]> buff;

    private:
        inline size_t next_pos(size_t x) {
            return (((x) + 1) % (capacity));
        }

    public:
        Queue(size_t size=0):
         capacity(size), tail(0), head(0), buff(nullptr) {
            if (size) {
                tail = head = 0;
                capacity = size + 1;
                buff.reset(new T[capacity]);
            }
        }

        void reset() {
            tail = head = 0;
        }

        size_t empty() {
            return tail == head;
        }

        operator bool() const {
            return buff.get() != nullptr;
        }

        bool push(T data) {
            bool ret = false;
            size_t next = next_pos(head);
            if (buff && (next != tail)) {
                buff[head] = data;
                head = next;
                ret = true;
            }
            return ret;
        }

        T pop(bool peek=false) {
            if (buff && (tail != head)) {
                T data = buff[tail];
                if (!peek) {
                    tail = next_pos(tail);
                }
                return data;
            }
            return T();
        }
};
#endif //__QUEUE_H__
