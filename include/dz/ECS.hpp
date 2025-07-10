#pragma once
#include <dz/GlobalUID.hpp>
#include <unordered_map>
#include <memory>
#include <dz/BufferGroup.hpp>
#include <string>
#include <vector>
namespace dz {
    
    template<typename EntityT, typename ComponentT, typename SystemT>
    struct ECS {

        struct EntityComponentEntry {
            int index;
            bool ensured = false;
            EntityT original_entity;
            std::unordered_map<int, std::unique_ptr<ComponentT>> components;
        };

        std::string shader_string_entity_struct;
        std::string shader_string_component_struct;
        std::string shader_string_system_struct;

        std::unordered_map<int, EntityComponentEntry> id_entity_entries;
        BufferGroup* buffer_group = 0;
        std::string buffer_name;
        int buffer_reserved = 0;

        ECS(
            const std::string& shader_string_entity_struct = R"(
struct Entity {
    int id;
};
)",
            const std::string& shader_string_component_struct = R"(
struct Component {
    int id;
};
)",
            const std::string& shader_string_system_struct = R"(
struct System {
    int id;
};
)"
        ):
            shader_string_entity_struct(shader_string_entity_struct),
            shader_string_component_struct(shader_string_component_struct),
            shader_string_system_struct(shader_string_system_struct)
        {};
        
        void BindEntityBuffer(BufferGroup* buffer_group, const std::string& buffer_name) {
            this->buffer_group = buffer_group;
            this->buffer_name = buffer_name;
            Reserve(8);
        }

        void Reserve(int n) {
            if (buffer_group) {
                buffer_reserved = buffer_group_get_buffer_element_count(buffer_group, buffer_name);
            }
            if (buffer_reserved < n) {
                if (buffer_group) {
                    buffer_group_set_buffer_element_count(buffer_group, buffer_name, n);
                }
                buffer_reserved = n;
            }
        }

        int AddEntity(const EntityT& entity) {
            auto id = GlobalUID::GetNew();
            ((EntityT&)entity).id = id;
            auto index = id_entity_entries.size();
            auto& entry = id_entity_entries[id];
            entry.index = index;
            entry.original_entity = entity;
            if (buffer_group) {
                buffer_reserved = buffer_group_get_buffer_element_count(buffer_group, buffer_name);
                if ((index + 1) > buffer_reserved) {
                    Reserve(buffer_reserved * 2);
                }
                auto entity_ptr = GetEntity(id);
                if (!entity_ptr)
                    assert(false);
                *entity_ptr = entity;
            }
            return id;
        }

        template <typename... Args>
        std::vector<int> AddEntities(const EntityT& first_entity, const Args&... rest_entities) {
            auto n = 1 + sizeof...(rest_entities);
            std::vector<int> ids(n, 0);
            auto ids_data = ids.data();
            auto index = id_entity_entries.size();
            for (int c = 1; c <= n; c++) {
                auto id = GlobalUID::GetNew();
                auto& entry = id_entity_entries[id];
                entry.index = index;
                index++;
                ids_data[c - 1] = id;
            }
            if ((index + 1) > buffer_reserved) {
                int n = buffer_reserved * 2;
                while ((index + 1) > n) {
                    n *= 2;
                }
                Reserve(n);
            }
            size_t id_index = 0;
            SetEntities(ids_data, id_index, first_entity, rest_entities...);
            return ids;
        }

        template <typename... Args>
        void SetEntities(int* ids_data, size_t& id_index, const EntityT& set_entity, const Args&... rest_entities) {
            auto& id = ids_data[id_index++];
            auto entity_ptr = GetEntity(id);
            if (entity_ptr) {
                ((EntityT&)set_entity).id = id;
                *entity_ptr = set_entity;
            }
            SetEntities(ids_data, id_index, rest_entities...);
        }

        void SetEntities(int* ids_data, size_t& id_index) { }

        EntityT* GetEntity(int id) {
            static auto entity_size = sizeof(EntityT);
            auto it = id_entity_entries.find(id);
            if (it == id_entity_entries.end()) {
                return 0;
            }
            auto& entry = it->second;
            if (entry.ensured) {
                auto index = entry.index;
                auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, buffer_name);
                return (EntityT*)(buffer_ptr.get() + (entity_size * index));
            }
            else {
                return &entry.original_entity;
            }
        }

        template<typename DComponentT>
        DComponentT& ConstructComponent(int entity_id, const DComponentT& original_component) {
            auto it = id_entity_entries.find(entity_id);
            if (it == id_entity_entries.end()) {
                throw std::runtime_error("entity not found with passed id!");
            }
            auto& entry = it->second;
            auto com_it = entry.components.emplace(GlobalUID::GetNew(), std::make_unique<DComponentT>(original_component));
            auto& ucom = com_it.first->second;
            return *(DComponentT*)ucom.get();
        }

        void Ensure() {
            Reserve(id_entity_entries.size());
            for (auto& [id, entry] : id_entity_entries) {
                auto& index = entry.index;
            }
        }
    };
}