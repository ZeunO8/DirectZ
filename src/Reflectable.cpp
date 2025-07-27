#include <dz/Reflectable.hpp>

bool BackupGroupVector(Serial& serial, const std::vector<std::shared_ptr<ReflectableGroup>>& group_vector) {
    auto group_vector_size = group_vector.size();
    serial << group_vector_size;
    auto group_vector_data = group_vector.data();
    for (size_t index = 0; index < group_vector_size; ++index) {
        auto& group_ptr = group_vector_data[index];
        assert(!!group_ptr);
        auto& group = *group_ptr;
        serial << group.cid;
        if (!group.backup(serial))
            return false;
    }
    return true;
}

bool RestoreGroupVector(
    Serial& serial,
    std::vector<std::shared_ptr<ReflectableGroup>>& group_vector,
    dz::BufferGroup* buffer_group
) {
    auto& cid_restore_map = *ReflectableGroup::cid_restore_map_ptr;
    auto& pid_reflectable_vecs = *ReflectableGroup::pid_reflectable_vecs_ptr;
    auto& pid_id_index_maps = *ReflectableGroup::pid_id_index_maps_ptr;
    auto group_vector_size = group_vector.size();
    serial >> group_vector_size;
    group_vector.reserve(group_vector_size);
    for (size_t index = 0; index < group_vector_size; ++index) {
        size_t cid = 0;
        serial >> cid;
        assert(cid);
        auto restore_it = cid_restore_map.find(cid);
        if (restore_it == cid_restore_map.end()) {
            std::cerr << "Unable to restore cid [" << cid << "] not found" << std::endl;
            return false;
        }
        auto& restore_fn = restore_it->second;
        auto new_group_sh_ptr = restore_fn(buffer_group, serial);
        group_vector.push_back(new_group_sh_ptr);
        auto group_ptr = new_group_sh_ptr.get();
        pid_reflectable_vecs[cid][group_ptr->index] = group_ptr;
        pid_id_index_maps[cid][group_ptr->id] = group_ptr->index;
    }
    return true;
}