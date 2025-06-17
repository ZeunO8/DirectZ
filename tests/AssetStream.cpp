// #include <iostream>
// #include <fstream>
// #include <streambuf>
// #include <vector>
// #include <string>
// #include <unordered_map>
// #include <sstream>
// #include <type_traits>
// #include <cassert>
// #include <cstring>
// #include <cstdint>
// #include <stdexcept>
// #include <memory>
// #include <iomanip>
// #include <algorithm>

#include <DirectZ.hpp>

int main() {
    const char* filename = "kvstore.bin";

    FileHandle handle{FileHandle::MEMORY, filename};

    {
        KeyValueStream<int, std::string> kv(handle);
        kv.write(1, "hello");
        std::string out;
        if (kv.read(1, out)) std::cout << "Key 1: " << out << std::endl;
        kv.write(2, "world");
        kv.write(3, "foo");
        kv.write(2, "updated");
    }
    {
        KeyValueStream<int, std::string> kv(handle);
        std::string out;
        if (kv.read(2, out)) std::cout << "Key 2: " << out << std::endl;
        if (kv.read(1, out)) std::cout << "Key 1: " << out << std::endl;
        if (kv.read(3, out)) std::cout << "Key 3: " << out << std::endl;
    }
    // kv.erase(1);
    return 0;
}