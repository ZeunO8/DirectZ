#include <dz/function.hpp>
#include <functional>
#include <iostream>
#include <chrono>
#include <thread>
#include "munit.h"

#define SIM_DURATION_MS 3200
#define TICK_INTERVAL_MS 800
#define ALLOC_SIZE_MIN 16
#define ALLOC_SIZE_MAX 256
#define OPS_PER_TICK 5000

struct Allocation
{
    void *ptr;
    size_t size;
};

static void print_arena_stats(const char *label, auto& arena, size_t current, size_t peak)
{
    std::cout << label << "\n\tArena Used   = " << arena.memory_used()
              << "B,\n\tArena Chunks = " << arena.chunks_size()
              << ",\n\tCurrent      = " << current
              << "B,\n\tPeak         = " << peak << "B" << std::endl;
}

static void print_stats(const char *label, size_t current, size_t peak)
{
    std::cout << label << "\n\tCurrent = " << current << "B,\n\tPeak    = " << peak << "B" << std::endl;
}

static MunitResult test_dz_arena_sim(const MunitParameter[], void *user_data)
{
    std::cout << std::endl << std::endl;
    dz::Arena<32768> arena;
    std::vector<Allocation> live_allocs;
    size_t current = 0;
    size_t peak = 0;

    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::milliseconds(SIM_DURATION_MS);
    auto next_tick = start + std::chrono::milliseconds(TICK_INTERVAL_MS);

    while (std::chrono::high_resolution_clock::now() < end)
    {
        for (int i = 0; i < OPS_PER_TICK; i++)
        {
            if (rand() % 2 == 0 || live_allocs.empty())
            {
                size_t size = ALLOC_SIZE_MIN + (rand() % (ALLOC_SIZE_MAX - ALLOC_SIZE_MIN + 1));
                void *ptr = arena.arena_malloc(size);
                live_allocs.push_back({ptr, size});
                current += size;
                if (current > peak)
                    peak = current;
            }
            else
            {
                int idx = rand() % live_allocs.size();
                auto& alloc = live_allocs[idx];
                arena.arena_free(alloc.ptr, alloc.size);
                current -= alloc.size;
                alloc = live_allocs.back();
                live_allocs.pop_back();
            }
        }

        auto now = std::chrono::high_resolution_clock::now();
        if (now >= next_tick)
        {
            print_arena_stats("[Arena TICK]", arena, current, peak);
            next_tick += std::chrono::milliseconds(TICK_INTERVAL_MS);
        }
    }

    print_arena_stats("[Arena FINAL]", arena, current, peak);
    std::cout << std::endl;
    return MUNIT_OK;
}

static MunitResult test_std_malloc_sim(const MunitParameter[], void *user_data)
{
    std::cout << std::endl << std::endl;
    std::vector<Allocation> live_allocs;
    size_t current = 0;
    size_t peak = 0;

    auto start = std::chrono::high_resolution_clock::now();
    auto end = start + std::chrono::milliseconds(SIM_DURATION_MS);
    auto next_tick = start + std::chrono::milliseconds(TICK_INTERVAL_MS);

    while (std::chrono::high_resolution_clock::now() < end)
    {
        for (int i = 0; i < OPS_PER_TICK; i++)
        {
            if (rand() % 2 == 0 || live_allocs.empty())
            {
                size_t size = ALLOC_SIZE_MIN + (rand() % (ALLOC_SIZE_MAX - ALLOC_SIZE_MIN + 1));
                void *ptr = std::malloc(size);
                live_allocs.push_back({ptr, size});
                current += size;
                if (current > peak)
                    peak = current;
            }
            else
            {
                int idx = rand() % live_allocs.size();
                current -= live_allocs[idx].size;
                std::free(live_allocs[idx].ptr);
                live_allocs[idx] = live_allocs.back();
                live_allocs.pop_back();
            }
        }

        auto now = std::chrono::high_resolution_clock::now();
        if (now >= next_tick)
        {
            print_stats("[Malloc TICK]", current, peak);
            next_tick += std::chrono::milliseconds(TICK_INTERVAL_MS);
        }
    }

    // Free remaining
    for (auto &alloc : live_allocs) {
        current -= alloc.size;
        std::free(alloc.ptr);
    }

    print_stats("[Malloc FINAL]", current, peak);
    std::cout << std::endl;
    return MUNIT_OK;
}

#define ITERATIONS 1'000'000

static MunitResult test_dz_arena_bulk(const MunitParameter[], void *user_data)
{
    size_t total_requested = 0;
    size_t total_allocated_arena = 0;
    size_t total_allocated_malloc = 0;
    std::cout << std::endl << std::endl;
    dz::Arena<32768> arena;
    total_requested = 0;
    total_allocated_arena = 0;
    for (auto i = 0; i < ITERATIONS; i++)
    {
        auto size = 16 + (i % 3) * 16; // 16, 32, 48
        void *ptr = arena.arena_malloc(size);
        *(int *)ptr = i;
        total_requested += size;
    }
    total_allocated_arena = arena.memory_used(); // assume Arena provides memory_used()
    arena.reset();
    std::cout << "[Arena Bulk] requested = " << total_requested << "B, allocated = " << total_allocated_arena << "B, overhead = " << (total_allocated_arena - total_requested) << "B" << std::endl;
    std::cout << std::endl;
    return MUNIT_OK;
}

static MunitResult test_std_malloc_bulk(const MunitParameter[], void *user_data)
{
    size_t total_requested = 0;
    size_t total_allocated_arena = 0;
    size_t total_allocated_malloc = 0;
    std::cout << std::endl << std::endl;
    total_requested = 0;
    total_allocated_malloc = 0;
    for (auto i = 0; i < ITERATIONS; i++)
    {
        auto size = 16 + (i % 3) * 16; // 16, 32, 48
        void *ptr = std::malloc(size);
        *(int *)ptr = i;
        total_requested += size;
        // we can't directly measure malloc overhead, so approximate as requested
        total_allocated_malloc += size;
        std::free(ptr);
    }
    std::cout << "[Malloc Bulk] requested = " << total_requested << "B, allocated~ = " << total_allocated_malloc << "B" << std::endl;
    std::cout << std::endl;
    return MUNIT_OK;
}

static MunitResult test_dz_arena(const MunitParameter params[], void *user_data)
{
    size_t total_requested = 0;
    size_t total_allocated_arena = 0;
    size_t total_allocated_malloc = 0;
    size_t total_allocated = 0;
    std::cout << std::endl << std::endl;
    void **ptrs = (void **)malloc(ITERATIONS * sizeof(void *));
    int *sizes = (int *)malloc(ITERATIONS * sizeof(int));
    dz::Arena<16384> arena(true);
    total_requested = 0;
    total_allocated = 0;
    for (auto i = 0; i < ITERATIONS; i++)
    {
        auto mod = i % 3;
        auto size = 0;
        switch (mod)
        {
        case 0:
            size = 16;
            break;
        case 1:
            size = 32;
            break;
        case 2:
            size = 64;
            break;
        }
        ptrs[i] = arena.arena_malloc(size);
        *(int *)ptrs[i] = i;
        sizes[i] = size;
        total_requested += size;
        total_allocated += size;
    }
    for (auto i = 0; i < ITERATIONS; i++)
        *(int *)ptrs[i] *= 2;
    for (auto i = 0; i < ITERATIONS; i++)
    {
        auto mod = i % 3;
        auto size = 0;
        switch (mod)
        {
        case 0:
            size = 32;
            break;
        case 1:
            size = 64;
            break;
        case 2:
            size = 128;
            break;
        }
        ptrs[i] = arena.arena_realloc(ptrs[i], sizes[i], size);
        total_requested += size;
        total_allocated += (size - sizes[i]);
        sizes[i] = size;
    }
    for (auto i = 0; i < ITERATIONS; i++)
        munit_assert_int(*(int *)ptrs[i], ==, i * 2);
    for (auto i = 0; i < ITERATIONS; i++)
        arena.arena_free(ptrs[i], sizes[i]);
    total_allocated_arena = arena.memory_used();
    std::cout << "[Arena Slow]\n\trequested = " << total_requested
        << "B,\n\tallocated = " << total_allocated_arena
        << "B,\n\toverhead = " << (total_allocated_arena - total_allocated) << "B"
        << ",\n\tallocated = " << total_allocated
        << std::endl;
    free(ptrs);
    free(sizes);
    std::cout << std::endl;
    return MUNIT_OK;
}

static MunitResult test_std_malloc(const MunitParameter params[], void *user_data)
{
    size_t total_requested = 0;
    size_t total_allocated_arena = 0;
    size_t total_allocated_malloc = 0;
    std::cout << std::endl << std::endl;
    void **ptrs = (void **)malloc(ITERATIONS * sizeof(void *));
    int *sizes = (int *)malloc(ITERATIONS * sizeof(int));
    total_requested = 0;
    total_allocated_malloc = 0;
    for (auto i = 0; i < ITERATIONS; i++)
    {
        auto mod = i % 3;
        auto size = 0;
        switch (mod)
        {
        case 0:
            size = 16;
            break;
        case 1:
            size = 32;
            break;
        case 2:
            size = 64;
            break;
        }
        ptrs[i] = std::malloc(size);
        *(int *)ptrs[i] = i;
        sizes[i] = size;
        total_requested += size;
        total_allocated_malloc += size; // approximation
    }
    for (auto i = 0; i < ITERATIONS; i++)
        *(int *)ptrs[i] *= 2;
    for (auto i = 0; i < ITERATIONS; i++)
    {
        auto mod = i % 3;
        auto size = 0;
        switch (mod)
        {
        case 0:
            size = 32;
            break;
        case 1:
            size = 64;
            break;
        case 2:
            size = 128;
            break;
        }
        ptrs[i] = std::realloc(ptrs[i], size);
        total_requested += size;
        total_allocated_malloc += (size - sizes[i]);
        sizes[i] = size;
    }
    for (auto i = 0; i < ITERATIONS; i++)
        munit_assert_int(*(int *)ptrs[i], ==, i * 2);
    for (auto i = 0; i < ITERATIONS; i++)
        std::free(ptrs[i]);
    std::cout << "[Malloc Slow] requested = " << total_requested << "B, allocated~ = " << total_allocated_malloc << "B" << std::endl;
    free(ptrs);
    free(sizes);
    std::cout << std::endl;
    return MUNIT_OK;
}

#include <stdlib.h>

static MunitTest test_suite_tests[] = {
    {(char *)"/arena.cpp/dz::Arena/sim", test_dz_arena_sim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/arena.cpp/std::malloc/sim", test_std_malloc_sim, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/dz::Arena/bulk", test_dz_arena_bulk, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/dz::Arena/slow", test_dz_arena, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/std::malloc/bulk", test_std_malloc_bulk, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {(char *)"/std::malloc/slow", test_std_malloc, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL},
    {NULL, NULL, NULL, NULL, MUNIT_TEST_OPTION_NONE, NULL}};

static const MunitSuite test_suite = {
    (char *)"/arena.cpp",
    test_suite_tests,
    NULL,
    1,
    MUNIT_SUITE_OPTION_NONE};

int main(int argc, char *argv[MUNIT_ARRAY_PARAM(argc + 1)])
{
    return munit_suite_main(&test_suite, (void *)"dz", argc, argv);
}