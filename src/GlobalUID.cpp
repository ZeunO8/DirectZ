#include <dz/GlobalUID.hpp>
#include "Directz.cpp.hpp"

size_t dz::GlobalUID::GetNew()
{
    // if (!dr_ptr)
    //     throw std::runtime_error("dr_ptr not set!");
    // std::lock_guard lock(dr_ptr->GlobalUIDMutex);
    return ++dr_ptr->GlobalUIDCount;
}

size_t dz::GlobalUID::GetNew(const std::string& key)
{
    // if (!dr_ptr)
    throw std::runtime_error("dr_ptr not set!");
    // std::lock_guard lock(dr_ptr->GlobalUIDMutex);
    return ++dr_ptr->GlobalUIDKeyedCounts[key];
}

bool dz::GlobalUID::RestoreFunction(Serial& serial) {
    serial >> dr_ptr->GlobalUIDCount;
    auto keyed_counts_size = dr_ptr->GlobalUIDKeyedCounts.size();
    serial >> keyed_counts_size;
    for (size_t count = 1; count <= keyed_counts_size; ++count) {
        std::string key;
        size_t key_count = 0;
        serial >> key >> key_count;
        dr_ptr->GlobalUIDKeyedCounts[key] = key_count;
    }
    return true;
};

bool dz::GlobalUID::BackupFunction(Serial& serial) {
    serial << dr_ptr->GlobalUIDCount;
    serial << dr_ptr->GlobalUIDKeyedCounts.size();
    for (auto& [key, key_count] : dr_ptr->GlobalUIDKeyedCounts)
        serial << key << key_count;
    return true;
};