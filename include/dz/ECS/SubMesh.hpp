#pragma once
#include "Provider.hpp"
#include "Mesh.hpp"
#include "Material.hpp"
#include "../Reflectable.hpp"

namespace dz::ecs {
    inline static std::string Meshs_Str = "Meshs";
    struct SubMesh : Provider<SubMesh> {
        int parent_index = -1;
        int parent_cid = 0;
        int mesh_index = -1;
        int material_index = -1;

        inline static constexpr size_t PID = 4;
        inline static float Priority = 0.5f;
        inline static constexpr BufferHost BufferHostType = BufferHost::GPU;
        inline static constexpr bool IsDrawProvider = true;
        inline static constexpr bool IsSubMeshProvider = true;
        inline static std::string ProviderName = "SubMesh";
        inline static std::string StructName = "SubMesh";
        inline static std::string GLSLStruct = R"(
struct SubMesh {
    int parent_index;
    int parent_cid;
    int mesh_index;
    int material_index;
};
)";

        uint32_t GetVertexCount(BufferGroup* buffer_group) {
            auto mesh_buffer_sh_ptr = buffer_group_get_buffer_data_ptr(buffer_group, Meshs_Str);
            auto& mesh = *(Mesh*)(mesh_buffer_sh_ptr.get() + (sizeof(Mesh) * mesh_index));
            return mesh.vertex_count;
        }
        
        struct SubMeshReflectable : ::Reflectable {

        private:
            std::function<SubMesh*()> get_submesh_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"Material Index", {0, 0}},
                {"Material", {1, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "Material Index"},
                {1, "Material"}
            };
            inline static std::vector<std::string> prop_names = {
                "Material Index",
                "Material"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(int),
                &typeid(MaterialIndexReflectable)
            };

        public:
            SubMeshReflectable(const std::function<SubMesh*()>& get_submesh_function);
            int GetID() override;
            std::string& GetName() override;
            DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
            DEF_GET_PROPERTY_NAMES(prop_names);
            void* GetVoidPropertyByIndex(int prop_index) override;
            DEF_GET_VOID_PROPERTY_BY_NAME;
            DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
            void NotifyChange(int prop_index) override;
        };

        struct SubMeshReflectableGroup : ::ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<Reflectable*> reflectables;
            std::vector<std::shared_ptr<ReflectableGroup>> reflectable_children;
            Image* image = nullptr;
            SubMeshReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("SubMesh")
            {}
            SubMeshReflectableGroup(BufferGroup* buffer_group, Serial& serial):
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
            std::vector<std::shared_ptr<ReflectableGroup>>& GetChildren() override {
                return reflectable_children;
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
                    reflectables.push_back(new SubMeshReflectable([&]() {
                        auto buffer = buffer_group_get_buffer_data_ptr(buffer_group, "SubMeshs");
                        return ((struct SubMesh*)(buffer.get())) + index;
                    }));
                }
            }
            bool backup(Serial& serial) const override {
                if (!backup_internal(serial))
                    return false;
                serial << name;
                if (!BackupGroupVector(serial, reflectable_children))
                    return false;
                return true;
            }
            bool restore(Serial& serial) override{
                if (!restore_internal(serial))
                    return false;
                serial >> name;
                if (!RestoreGroupVector(serial, reflectable_children, buffer_group))
                    return false;
                return true;
            }

        };

        using ReflectableGroup = SubMeshReflectableGroup;
    };
}