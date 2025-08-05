#pragma once
#include "Provider.hpp"
#include "../Reflectable.hpp"
#include "../math.hpp"
namespace dz::ecs {
    struct Light : Provider<Light> {
        enum LightType : uint8_t {
            Directional,
            Spot,
            Point
        };
        int type = 0;
        float intensity;          // scalar
        float range;              // for point/spot
        float innerCone;          // for spot
        vec<float, 3> position;
        int padding1 = 0;
        vec<float, 3> direction;  // normalized, for directional/spot
        int padding2 = 0;
        vec<float, 3> color;      // RGBA
        float outerCone;          // for spot
        inline static constexpr size_t PID = 7;
        inline static float Priority = 3.5f;
        inline static constexpr BufferHost BufferHostType = BufferHost::GPU;
        inline static std::string ProviderName = "Light";
        inline static std::string StructName = "Light";
        inline static std::string GLSLStruct = R"(
struct Light {
    int type;
    float intensity;
    float range;
    float innerCone;
    vec3 position;
    int padding1;
    vec3 direction;
    int padding2;
    vec3 color;
    float outerCone;
};

struct LightingParams {
    vec3 worldPosition;
    vec3 viewDirection;
    float NdotV;
    vec3 normal;
    int lightsSize;
};

LightingParams lParams;
)";

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {3.5f, R"(
    lParams.worldPosition = inPosition;
    lParams.viewDirection = normalize(camera.position - lParams.worldPosition);
    lParams.NdotV = max(dot(current_normal, lParams.viewDirection), 0.0);
    lParams.normal = current_normal;
    lParams.lightsSize = Lights.data.length();
)", ShaderModuleType::Fragment}
        };
        
        struct LightMetaReflectable : Reflectable {
            
        private:
            std::function<Light*()> get_light_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                { "type", {0, 0}},
                { "intensity", {1, 0}},
                { "range", {2, 0}},
                { "innerCone", {3, 0}},
                { "position", {4, 0}},
                { "direction", {5, 0}},
                { "color", {6, 0}},
                { "outerCone", {7, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                { 0, "type" },
                { 1, "intensity"},
                { 2, "range"},
                { 3, "innerCone"},
                { 4, "position"},
                { 5, "direction"},
                { 6, "color"},
                { 7, "outerCone"}
            };
            inline static std::vector<std::string> prop_names = {
                "type",
                "intensity",
                "range",
                "innerCone",
                "position",
                "direction",
                "color",
                "outerCone"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(Light::LightType),
                &typeid(float),
                &typeid(float),
                &typeid(float),
                &typeid(vec<float, 3>),
                &typeid(vec<float, 3>),
                &typeid(color_vec<float, 3>),
                &typeid(float)
            };

        public:
            LightMetaReflectable(
                const std::function<Light*()>& get_light_function
            );
            int GetID() override;
            std::string& GetName() override;
            DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
            DEF_GET_PROPERTY_NAMES(prop_names);
            void* GetVoidPropertyByIndex(int prop_index) override;
            DEF_GET_VOID_PROPERTY_BY_NAME;
            DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
            void NotifyChange(int prop_index) override;
        };

        struct LightReflectableGroup : ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::vector<Reflectable*> reflectables;
            LightReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("Light")
            {}
            LightReflectableGroup(BufferGroup* buffer_group, Serial& serial):
                buffer_group(buffer_group)
            {
                restore(serial);
            }
            ~LightReflectableGroup() {
                ClearReflectables();
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Light;
            }
            std::string& GetName() override {
                return name;
            }
            const std::vector<Reflectable*>& GetReflectables() override {
                return reflectables;
            }
            void UpdateReflectables() { }
            void ClearReflectables() {
                if (reflectables.empty()) {
                    return;
                }
                delete reflectables[0];
                reflectables.clear();
            }
            void UpdateChildren() override {
                if (reflectables.empty()) {
                    reflectables.push_back(new LightMetaReflectable([&]() {
                        auto buffer = buffer_group_get_buffer_data_ptr(buffer_group, "Lights");
                        return ((struct Light*)(buffer.get())) + index;
                    }));
                }
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

        using ReflectableGroup = LightReflectableGroup;
    };
}