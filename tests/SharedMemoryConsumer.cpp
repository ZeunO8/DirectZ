#include <dz/SharedMemory.hpp>
#include <dz/SharedMemoryPtr.hpp>
#include <iostream>
#include <chrono>

struct TestStruct {
    float x = 42.0;
    float y = 52.0;
    std::string str = "Hello shm!";
    bool active = true;
};

int main() {
    dz::SharedMemoryPtr<TestStruct> consumer_ptr;
    if (!consumer_ptr.Open("test-struct"))
        return 1;

    auto& consumer = *consumer_ptr;

    auto begin = std::chrono::system_clock::now();

    do {
        auto now = std::chrono::system_clock::now();
        auto diff = now - begin;
        if (diff.count() > 2'000'000'0)
            consumer.active = false;
        std::cout << "x: " << consumer.x << ", y: " << consumer.y << std::endl;
    } while (consumer.active);

    return 0;
}