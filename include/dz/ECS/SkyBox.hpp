#pragma once
#include "Provider.hpp"
#include "../Reflectable.hpp"
#include "../math.hpp"
#include "../Shader.hpp"
namespace dz::ecs {
    struct SkyBox : Provider<SkyBox> {
        int parent_index = -1;
        int parent_cid = 0;
        int hdri_index = -1;
        int padding = 0;
        inline static constexpr size_t PID = 10;
        inline static float Priority = 4.0f;
        inline static constexpr BufferHost BufferHostType = BufferHost::GPU;
        inline static constexpr bool IsSkyBoxProvider = true;
        inline static std::string ProviderName = "SkyBox";
        inline static std::string StructName = "SkyBox";
        inline static std::string GLSLStruct = R"(
struct SkyBox {
    int parent_index;
    int parent_cid;
    int hdri_index;
    int padding;
};
)";
        inline static std::unordered_map<ShaderModuleType, std::string> GLSLMethods = {
            {ShaderModuleType::Fragment, R"(
)" }
        };
        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {4.0f, R"(
)", ShaderModuleType::Fragment}
        };

        struct SkyBoxReflectableGroup : ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<Reflectable*> reflectables;
            SkyBoxReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("SkyBox")
            {}
            SkyBoxReflectableGroup(BufferGroup* buffer_group, Serial& serial):
                buffer_group(buffer_group)
            {
                restore(serial);
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Light;
            }
            std::string& GetName() override {
                return name;
            }
            bool backup(Serial& serial) const override {
                if (!backup_internal(serial))
                    return false;
                serial << name;
                return true;
            }
            bool restore(Serial& serial) override {
                if (!restore_internal(serial))
                    return false;
                serial >> name;
                return true;
            }
        };

        using ReflectableGroup = SkyBoxReflectableGroup;
    };
}