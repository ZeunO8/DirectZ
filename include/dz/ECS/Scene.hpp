#pragma once
#include "Provider.hpp"

namespace dz::ecs {
    struct Scene : Provider<Scene> {
        int id = 0;
        
        inline static constexpr size_t PID = 6;
        inline static float Priority = 0.5f;
        inline static constexpr bool IsSceneProvider = true;
        inline static std::string ProviderName = "Scene";
        inline static std::string StructName = "Scene";
        inline static std::string GLSLStruct = R"(
struct Scene {
    int id;
};
)";

        struct SceneReflectableGroup : ReflectableGroup {
            int parent_id = -1;
            BufferGroup* buffer_group = 0;
            std::string name;
            std::vector<std::shared_ptr<ReflectableGroup>> reflectable_children;
            SceneReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("Scene")    
            {}
            GroupType GetGroupType() override {
                return ReflectableGroup::Scene;
            }
            std::string& GetName() override {
                return name;
            }
            std::vector<std::shared_ptr<ReflectableGroup>>& GetChildren() override {
                return reflectable_children;
            }
            void UpdateReflectables() { }
            bool serialize(Serial& ioSerial) const {
                ioSerial << parent_id << name;
                ioSerial << disabled << id << index << is_child;
                return true;
            }
            bool deserialize(Serial& ioSerial) {
                ioSerial >> parent_id >> name;
                ioSerial >> disabled >> id >> index >> is_child;
                return true;
            }
        };

        using ReflectableGroup = SceneReflectableGroup;

        inline static std::shared_ptr<ReflectableGroup> MakeGroup(BufferGroup* buffer_group) {
            return std::make_shared<SceneReflectableGroup>(buffer_group);
        }
    };
}