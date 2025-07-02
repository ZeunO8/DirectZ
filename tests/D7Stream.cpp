#include <dz/D7Stream.hpp>
#include <iostream>
#include <thread>
#include <random>
#include <chrono>

using namespace dz;

int main()
{
    D7Stream d7stream;
    auto now = std::chrono::steady_clock::now();

    std::mt19937_64 rng(42);
    std::uniform_real_distribution<float> pos(-1000.0f, 1000.0f);
    std::uniform_int_distribution<int64_t> uid_dist(1, 10);
    std::uniform_int_distribution<uint64_t> Uid_dist(100, 1000);
    std::vector<std::string> actions = {
        "spawn", "jump", "move", "shoot", "explode", "dance", "hide", "seek", "charge", "build"
    };

    for (size_t t = 0; t < 1000; ++t)
    {
        StreamScalar x = pos(rng);
        StreamScalar y = pos(rng);
        StreamScalar z = pos(rng);
        StreamTimestamp time = now + std::chrono::milliseconds(t * 16);
        StreamIdentifier Uid = Uid_dist(rng);
        StreamInteger uid = uid_dist(rng);
        StreamString action = actions[rng() % actions.size()];

        StreamPoint point = std::make_tuple(x, y, z, time, Uid, uid, action);
        d7stream << point;
    }

    std::cout << "Simulated 1000 stream points.\n";

    // Remove all "shoot" events
    for (int i = static_cast<int>(d7stream.stream_points.size()) - 1; i >= 0; --i)
    {
        const auto& point = d7stream.stream_points[i];
        if (std::get<D7TypeToIndex<D7Type::a>()>(point) == "shoot")
        {
            size_t* ptr = d7stream.stream_indexes[i];
            d7stream.removeStreamPoint(ptr);
        }
    }

    std::cout << "Removed all 'shoot' actions.\n";

    std::cout << "\nPrinting all 'shoot' events (should be none):\n";
    d7stream.printStreamPoints(D7Type::a, "shoot");

    std::cout << "\nPrinting some 'move' events (should exist):\n";
    d7stream.printStreamPoints(D7Type::a, "move");

    return 0;
}