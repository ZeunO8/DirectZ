#include <dz/SharedMemory.hpp>
#include <dz/SharedMemoryPtr.hpp>
#include <dz/math.hpp>
#include <iostream>

struct TestStruct {
    float x = 42.0;
    float y = 52.0;
    std::string str = "Hello shm!";
    bool active = true;
};

int main() {
    dz::SharedMemoryPtr<TestStruct> producer_ptr;
    if (!producer_ptr.Create("test-struct"))
        return 1;

    auto& producer = *producer_ptr;

    while (producer.active) {
        producer.x = dz::Random::value<float>(42.f, 84.f);
        producer.y = dz::Random::value<float>(42.f, 84.f);
    }

    return 0;
}