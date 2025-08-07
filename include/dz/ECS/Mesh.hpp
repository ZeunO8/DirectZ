#pragma once
#include "Provider.hpp"
#include "../Reflectable.hpp"

namespace dz::ecs {
    struct Mesh : Provider<Mesh> {
        int vertex_count = 0;
        int position_offset = -1;
        int uv2_offset = -1;
        int normal_offset = -1;

        int tangent_offset = -1;
        int bitangent_offset = -1;
        int padding1 = 0;
        int padding2 = 0;

        inline static constexpr size_t PID = 3;
        inline static float Priority = 0.5f;
        inline static constexpr BufferHost BufferHostType = BufferHost::GPU;
        inline static constexpr bool IsMeshProvider = true;
        inline static std::string ProviderName = "Mesh";
        inline static std::string StructName = "Mesh";
        inline static std::string GLSLStruct = R"(
struct Mesh {
    int vertex_count;
    int position_offset;
    int uv2_offset;
    int normal_offset;
    int tangent_offset;
    int bitangent_offset;
    int padding1;
    int padding2;
};
)";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            { ShaderModuleType::Vertex, R"(
vec3 GetMeshVertex(in Mesh mesh) {
    if (mesh.position_offset == -1)
        return vec3(0.0);
    return VertexPositions.data[mesh.position_offset + gl_VertexIndex].xyz;
}
vec3 GetMeshNormal(in Mesh mesh) {
    if (mesh.normal_offset == -1)
        return vec3(0.0);
    return VertexNormals.data[mesh.normal_offset + gl_VertexIndex].xyz;
}
vec2 GetMeshUV2(in Mesh mesh) {
    if (mesh.uv2_offset == -1)
        return vec2(0.0);
    return VertexUV2s.data[mesh.uv2_offset + gl_VertexIndex];
}
vec3 GetMeshTangent(in Mesh mesh) {
    if (mesh.tangent_offset == -1)
        return vec3(0.0);
    return VertexTangents.data[mesh.tangent_offset + gl_VertexIndex].xyz;
}
vec3 GetMeshBitangent(in Mesh mesh) {
    if (mesh.bitangent_offset == -1)
        return vec3(0.0);
    return VertexBitangents.data[mesh.bitangent_offset + gl_VertexIndex].xyz;
}
)" },
            { ShaderModuleType::Fragment, R"(
)" }
        };

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.0f, R"(
    vec3 mesh_vertex = GetMeshVertex(mesh);
    vec3 mesh_normal = GetMeshNormal(mesh);
    vec2 mesh_uv2 = GetMeshUV2(mesh);
    vec3 mesh_tangent = GetMeshTangent(mesh);
    vec3 mesh_bitangent = GetMeshBitangent(mesh);
)", ShaderModuleType::Vertex}
        };
        
        struct MeshReflectable : ::Reflectable {

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

        struct MeshReflectableGroup : ::ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<Reflectable*> reflectables;
            std::vector<std::shared_ptr<ReflectableGroup>> reflectable_children;
            int material_index = -1;
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
                    reflectables.push_back(new MeshReflectable([&]() {
                        auto buffer = buffer_group_get_buffer_data_ptr(buffer_group, "Meshs");
                        return ((struct Mesh*)(buffer.get())) + index;
                    }));
                }
            }
            bool backup(Serial& serial) const override {
                if (!backup_internal(serial))
                    return false;
                serial << name << material_index;
                if (!BackupGroupVector(serial, reflectable_children))
                    return false;
                return true;
            }
            bool restore(Serial& serial) override{
                if (!restore_internal(serial))
                    return false;
                serial >> name >> material_index;
                if (!RestoreGroupVector(serial, reflectable_children, buffer_group))
                    return false;
                return true;
            }

        };

        using ReflectableGroup = MeshReflectableGroup;
    };
}