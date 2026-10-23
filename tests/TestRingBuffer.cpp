#include "core/RingBuffer.hpp"
#include <cassert>
#include <iostream>
#include <string>

void testRingBufferBasic() {
    pm::core::RingBuffer<int, 3> rb;
    assert(rb.empty());
    assert(rb.size() == 0);
    assert(rb.capacity() == 3);

    rb.push(10);
    assert(!rb.empty());
    assert(rb.size() == 1);
    assert(rb[0] == 10);
    assert(rb.back() == 10);

    rb.push(20);
    assert(rb.size() == 2);
    assert(rb[0] == 10);
    assert(rb[1] == 20);
    assert(rb.back() == 20);

    rb.push(30);
    assert(rb.full());
    assert(rb.size() == 3);
    assert(rb[0] == 10);
    assert(rb[1] == 20);
    assert(rb[2] == 30);

    // Переполнение: старейший элемент (10) вытесняется
    rb.push(40);
    assert(rb.full());
    assert(rb.size() == 3);
    assert(rb[0] == 20);
    assert(rb[1] == 30);
    assert(rb[2] == 40);
    assert(rb.back() == 40);

    auto vec = rb.toVector();
    assert(vec.size() == 3);
    assert(vec[0] == 20);
    assert(vec[1] == 30);
    assert(vec[2] == 40);

    rb.clear();
    assert(rb.empty());
    assert(rb.size() == 0);
}

int main() {
    testRingBufferBasic();
    std::cout << "All RingBuffer tests passed successfully!" << std::endl;
    return 0;
}
