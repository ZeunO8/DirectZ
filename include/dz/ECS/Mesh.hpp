#pragma once
#include "Provider.hpp"

namespace dz::ecs {
    struct Mesh : Provider<Mesh> {
        int vertex_count = 0;
        int position_offset = -1;
        int uv2_offset = -1;
        int normal_offset = -1;

        inline static constexpr size_t PID = 7;
        inline static float Priority = 0.5f;
        inline static constexpr bool IsMeshProvider = true;
        inline static std::string ProviderName = "Mesh";
        inline static std::string StructName = "Mesh";
        inline static std::string GLSLStruct = R"(
struct Mesh {
    int vertex_count;
    int position_offset;
    int uv2_offset;
    int normal_offset;
};
)";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            { ShaderModuleType::Vertex, R"(
)" },
            { ShaderModuleType::Fragment, R"(
)" }
        };

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
)", ShaderModuleType::Vertex},
            {0.5f, R"(
)", ShaderModuleType::Fragment}
        };
        
        struct MeshReflectable : Reflectable {

        private:
            std::function<Mesh*()> get_mesh_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"vertex_count", {0, 0}},
                {"position_offset", {1, 0}},
                {"uv2_offset", {2, 0}},
                {"normal_offset", {3, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "vertex_count"},
                {1, "position_offset"},
                {2, "uv2_offset"},
                {3, "normal_offset"}
            };
            inline static std::vector<std::string> prop_names = {
                "vertex_count",
                "position_offset",
                "uv2_offset",
                "normal_offset"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(int),
                &typeid(int),
                &typeid(int),
                &typeid(int)
            };

        public:
            MeshReflectable(const std::function<Mesh*()>& get_mesh_function);
            int GetID() override;
            std::string& GetName() override;
            DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
            DEF_GET_PROPERTY_NAMES(prop_names);
            void* GetVoidPropertyByIndex(int prop_index) override;
            DEF_GET_VOID_PROPERTY_BY_NAME;
            DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
            void NotifyChange(int prop_index) override;
        };

        struct MeshReflectableGroup : ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<Reflectable*> reflectables;
            Image* image = nullptr;
            MeshReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("Mesh")
            {}
            MeshReflectableGroup(BufferGroup* buffer_group, Serial& serial):
                buffer_group(buffer_group)
            {
                restore(serial);
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Generic;
            }
            std::string& GetName() override {
                return name;
            }
            const std::vector<Reflectable*>& GetReflectables() override {
                return reflectables;
            }
            void ClearReflectables() {
                if (reflectables.empty()) {
                    return;
                }
                delete reflectables[0];
                reflectables.clear();
            }
            void UpdateChildren() override {
                if (reflectables.empty()) {
                    reflectables.push_back(new MeshReflectable([&]() {
                        auto buffer = buffer_group_get_buffer_data_ptr(buffer_group, "Meshs");
                        return ((struct Mesh*)(buffer.get())) + index;
                    }));
                }
            }
            bool backup(Serial& serial) const override {
                if (!backup_internal(serial))
                    return false;
                serial << name;
                return true;
            }
            bool restore(Serial& serial) override{
                if (!restore_internal(serial))
                    return false;
                serial >> name;
                return true;
            }

        };

        using ReflectableGroup = MeshReflectableGroup;
    };
}