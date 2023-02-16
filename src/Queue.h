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
