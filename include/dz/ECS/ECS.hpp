#pragma once
#include "../GlobalUID.hpp"
#include <unordered_map>
#include <memory>
#include "../BufferGroup.hpp"
#include "../Framebuffer.hpp"
#include "../DrawListManager.hpp"
#include "../Window.hpp"
#include "../Shader.hpp"
#include "../Camera.hpp"
#include "../Util.hpp"
#include "Provider.hpp"
#include "Light.hpp"
#include "Component.hpp"
#include "Entity.hpp"
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <set>
#include <iostreams/Serial.hpp>
#include <fstream>

template <typename T>
struct TComponenTypeID;

template <typename T>
struct ComponentComponentName;

template <typename T>
struct ComponentStructName;

template <typename T>
struct ComponentStruct;

template <typename T>
struct ComponentGLSLMethods;

template <typename T>
struct ComponentGLSLMain;

#define DEF_COMPONENT_ID(TYPE, ID) \
template <> \
struct TComponenTypeID<TYPE> { \
    static constexpr size_t id = ID; \
}

#define DEF_COMPONENT_COMPONENT_NAME(TYPE, STRING) \
template <> \
struct ComponentComponentName<TYPE> { \
    inline static std::string string = STRING; \
}

#define DEF_COMPONENT_STRUCT_NAME(TYPE, STRING) \
template <> \
struct ComponentStructName<TYPE> { \
    inline static std::string string = STRING; \
}

#define DEF_COMPONENT_STRUCT(TYPE, STRING) \
template <> \
struct ComponentStruct<TYPE> { \
    inline static std::string string = STRING; \
}

#define DEF_COMPONENT_GLSL_METHODS(TYPE, STRING) \
template <> \
struct ComponentGLSLMethods<TYPE> { \
    inline static std::string string = STRING; \
}

#define DEF_COMPONENT_GLSL_MAIN(TYPE, STRING) \
template <> \
struct ComponentGLSLMain<TYPE> { \
    inline static std::string string = STRING; \
}

namespace dz {

    inline static const std::string cameras_buffer_name = "Cameras";
    inline static const std::string lights_buffer_name = "Lights";
    
    template<typename TEntity, typename TComponent, typename... TProviders>
    struct ECS : IGetComponentDataVoid {

        struct EntityComponentReflectableGroup : ReflectableGroup {
            size_t scene_id = 0;
            std::string name;
            std::unordered_map<int, std::shared_ptr<TComponent>> components;
            std::set<int> children;
            std::vector<Reflectable*> reflectables;
            std::vector<ReflectableGroup*> reflectable_children;
            GroupType GetGroupType() override {
                return ReflectableGroup::Entity;
            }
            std::string& GetName() override {
                return name;
            }
            const std::vector<Reflectable*>& GetReflectables() override {
                return reflectables;
            }
            const std::vector<ReflectableGroup*>& GetChildren() override {
                return reflectable_children;
            }
            void UpdateReflectables() {
                reflectables.clear();
                reflectables.reserve(components.size());
                for (auto& [id, component_ptr] : components)
                    reflectables.push_back(component_ptr.get());
            }
            void UpdateChildren(ECS& ecs) {
                reflectable_children.clear();
                reflectable_children.reserve(children.size());
                for (auto& id : children) {
                    auto id_it = ecs.id_entity_groups.find(id);
                    assert(id_it != ecs.id_entity_groups.end());
                    auto& child_group = id_it->second;
                    reflectable_children.push_back(&child_group);
                }
            }
            bool serialize(ECS& ecs, Serial& ioSerial) const {
                ioSerial << scene_id << name;
                ioSerial << disabled << id << index << is_child;
                ioSerial << components.size();
                for (auto& [component_id, component_ptr] : components) {
                    ioSerial << component_id;
                    auto& root_data = ecs.GetComponentRootData(component_ptr->index);
                    ioSerial << root_data.type;
                    if (!component_ptr->serialize(ioSerial))
                        return false;
                }
                ioSerial << children.size();
                for (auto& child_id : children)
                    ioSerial << child_id;
                return true;
            }
            bool deserialize(ECS& ecs, Serial& ioSerial) {
                ioSerial >> scene_id >> name;
                ioSerial >> disabled >> id >> index >> is_child;
                auto components_size = components.size();
                ioSerial >> components_size;
                for (size_t component_count = 1; component_count <= components_size; ++component_count) {
                    int component_id;
                    ioSerial >> component_id;
                    int component_type;
                    ioSerial >> component_type;
                    auto& component_reg = ecs.registered_component_map[component_type];
                    auto& component_ptr = (components[component_id] = component_reg.construct_component_fn());
                    if (!component_ptr->deserialize(ioSerial))
                        return false;
                }
                auto children_size = children.size();
                ioSerial >> children_size;
                for (size_t child_count = 1; child_count <= children_size; ++child_count) {
                    int child_id;
                    ioSerial >> child_id;
                    children.insert(child_id);
                }
                return true;
            }
        };

        struct SceneReflectableGroup : ReflectableGroup {
            std::string name;
            std::vector<ReflectableGroup*> reflectable_children;
            GroupType GetGroupType() override {
                return ReflectableGroup::Scene;
            }
            std::string& GetName() override {
                return name;
            }
            const std::vector<ReflectableGroup*>& GetChildren() override {
                return reflectable_children;
            }
            void UpdateChildren(ECS& ecs) {
                reflectable_children.clear();
                size_t count = 0;
                for (auto& [entity_id, entity_group] : ecs.id_entity_groups) {
                    if (entity_group.scene_id == id)
                        count++;
                }
                for (auto& [camera_id, camera_group] : ecs.indexed_camera_groups) {
                    if (camera_group.scene_id == id)
                        count++;
                }
                for (auto& [light_id, light_group] : ecs.indexed_light_groups) {
                    if (light_group.scene_id == id)
                        count++;
                }
                reflectable_children.reserve(count);
                for (auto& [entity_id, entity_group] : ecs.id_entity_groups) {
                    if (entity_group.scene_id == id)
                        reflectable_children.push_back(&entity_group);
                }
                for (auto& [camera_id, camera_group] : ecs.indexed_camera_groups) {
                    if (camera_group.scene_id == id)
                        reflectable_children.push_back(&camera_group);
                }
                for (auto& [light_id, light_group] : ecs.indexed_light_groups) {
                    if (light_group.scene_id == id)
                        reflectable_children.push_back(&light_group);
                }
            }
            bool serialize(ECS& ecs, Serial& ioSerial) const {
                ioSerial << name;
                ioSerial << disabled << id << index << is_child;
                return true;
            }
            bool deserialize(ECS& ecs, Serial& ioSerial) {
                ioSerial >> name;
                ioSerial >> disabled >> id >> index >> is_child;
                return true;
            }
        };

        struct RegisteredComponentEntry {
            int type_id;
            std::string struct_name;
            std::function<std::shared_ptr<TComponent>()> construct_component_fn;
            int constructed_count = 0;
        };

        struct CameraReflectableGroup : ReflectableGroup {
            size_t scene_id = 0;
            std::string name;
            std::string imgui_name;

            Framebuffer* framebuffer = 0;
            Image* fb_color_image = 0;
            Image* fb_depth_image = 0;
            std::vector<Reflectable*> reflectables;
            bool open_in_editor = true;
            VkDescriptorSet frame_image_ds = VK_NULL_HANDLE;
            ~CameraReflectableGroup() {
                framebuffer_destroy(framebuffer);
                ClearChildren();
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Camera;
            }
            std::string& GetName() override {
                return name;
            }
            void NotifyNameChanged() override {
                imgui_name.clear();
                auto name_len = strlen(name.c_str());
                imgui_name.insert(imgui_name.end(), name.begin(), name.begin() + name_len);
                imgui_name += "###Camera" + std::to_string(index);
                return;
            }
            const std::vector<Reflectable*>& GetReflectables() override {
                return reflectables;
            }
            void UpdateChildren(ECS& ecs) {
                auto ecs_ptr = &ecs;
                auto camera_index = index;
                if (reflectables.size() == 0) {
                    reflectables.push_back(new CameraTypeReflectable([ecs_ptr, camera_index]() mutable {
                        return ecs_ptr->GetCamera(camera_index);
                    }, [&, ecs_ptr]() mutable {
                        UpdateChildren(*ecs_ptr);
                    }));
                    reflectables.push_back(new CameraViewReflectable([ecs_ptr, camera_index]() mutable {
                        return ecs_ptr->GetCamera(camera_index);
                    }));
                }
                auto camera_ptr = ecs.GetCamera(index);
                assert(camera_ptr);
                auto& camera = *camera_ptr;
                // clear type reflectables
                for (size_t index = 0; index < reflectables.size(); index++) {
                    auto& reflectable = reflectables[index];
                    if (dynamic_cast<CameraPerspectiveReflectable*>(reflectable) ||
                        dynamic_cast<CameraOrthographicReflectable*>(reflectable)) {
                        delete reflectable;
                        reflectables.erase(reflectables.begin() + index);
                        index--;
                    }
                }
                switch (Camera::ProjectionType(camera.type)) {
                case Camera::Perspective:
                    reflectables.push_back(new CameraPerspectiveReflectable([ecs_ptr, camera_index]() mutable {
                        return ecs_ptr->GetCamera(camera_index);
                    }));
                    break;
                case Camera::Orthographic:
                    reflectables.push_back(new CameraOrthographicReflectable([ecs_ptr, camera_index]() mutable {
                        return ecs_ptr->GetCamera(camera_index);
                    }));
                    break;
                default: break;
                }
            }
            void ClearChildren() {
                for (auto reflectable_child : reflectables)
                    delete reflectable_child;
                reflectables.clear();
            }
            bool serialize(ECS& ecs, Serial& ioSerial) const {
                ioSerial << scene_id << name << imgui_name;
                ioSerial << disabled << id << index << is_child;
                return true;
            }
            bool deserialize(ECS& ecs, Serial& ioSerial) {
                ioSerial >> scene_id >> name >> imgui_name;
                ioSerial >> disabled >> id >> index >> is_child;
                return true;
            }
            void InitFramebuffer(ECS& ecs, float width, float height) {
                // Initialize camera framebuffer
                fb_color_image = image_create({
                    .width = uint32_t(width),
                    .height = uint32_t(height),
                    .is_framebuffer_attachment = true
                });
                fb_depth_image = image_create({
                    .width = uint32_t(width),
                    .height = uint32_t(height),
                    .format = VK_FORMAT_D32_SFLOAT,
                    .is_framebuffer_attachment = true
                });
                Image* fb_images[2] = {
                    fb_color_image,
                    fb_depth_image
                };
                AttachmentType fb_attachment_types[2] = {
                    AttachmentType::Color,
                    AttachmentType::Depth
                };
                FramebufferInfo fb_info{
                    .pImages = fb_images,
                    .imagesCount = 2,
                    .pAttachmentTypes = fb_attachment_types,
                    .attachmentTypesCount = 2,
                    .own_images = true
                };
                framebuffer = framebuffer_create(fb_info);

                auto frame_ds_pair = image_create_descriptor_set(fb_color_image);
                frame_image_ds = frame_ds_pair.second;
            }
        };

        struct LightReflectableGroup : ReflectableGroup {
            size_t scene_id = 0;
            std::string name;
            std::string imgui_name;
            std::vector<Reflectable*> reflectables;
            GroupType GetGroupType() override {
                return ReflectableGroup::Light;
            }
            std::string& GetName() override {
                return name;
            }
            ~LightReflectableGroup() {
                ClearChildren();
            }
            void NotifyNameChanged() override {
                imgui_name.clear();
                auto name_len = strlen(name.c_str());
                imgui_name.insert(imgui_name.end(), name.begin(), name.begin() + name_len);
                imgui_name += "###Light" + std::to_string(index);
                return;
            }
            const std::vector<Reflectable*>& GetReflectables() override {
                return reflectables;
            }
            void UpdateChildren(ECS& ecs) {
                auto ecs_ptr = &ecs;
                auto light_index = index;
                if (reflectables.size() == 0) {
                    reflectables.push_back(new ecs::LightMetaReflectable([ecs_ptr, light_index]() mutable {
                        return ecs_ptr->GetLight(light_index);
                    }, [&, ecs_ptr]() mutable {
                        UpdateChildren(*ecs_ptr);
                    }));
                }
            }
            void ClearChildren() {
                for (auto reflectable_child : reflectables)
                    delete reflectable_child;
                reflectables.clear();
            }
            bool serialize(ECS& ecs, Serial& ioSerial) const {
                ioSerial << scene_id << name << imgui_name;
                ioSerial << disabled << id << index << is_child;
                return true;
            }
            bool deserialize(ECS& ecs, Serial& ioSerial) {
                ioSerial >> scene_id >> name >> imgui_name;
                ioSerial >> disabled >> id >> index >> is_child;
                return true;
            }
        };

        struct ProviderReflectableGroup : ReflectableGroup {
            float priority;
            std::string name;
            std::string struct_name;
            std::string glsl_struct;
            std::string glsl_methods;
            std::string glsl_main;

            GroupType GetGroupType() override {
                return ReflectableGroup::Provider;
            }
            std::string& GetName() override {
                return name;
            }
        };

        WINDOW* window_ptr = 0;
        std::filesystem::path save_path;
        bool loaded_from_io = false;

        std::map<int, EntityComponentReflectableGroup> id_entity_groups;
        std::map<size_t, SceneReflectableGroup> id_scene_groups;
        std::map<int, CameraReflectableGroup> indexed_camera_groups;
        std::map<int, LightReflectableGroup> indexed_light_groups;
        std::map<size_t, ProviderReflectableGroup> id_provider_groups;
        std::map<float, std::vector<size_t>> prioritized_provider_ids;
        std::unordered_map<ShaderModuleType, std::map<float, std::vector<std::string>>> priority_glsl_mains;
        size_t add_scene_id;

        std::vector<std::string> restricted_keys{"Components"};
        std::map<int, RegisteredComponentEntry> registered_component_map;
        bool components_registered = false;

        DrawListManager<TEntity> draw_mg;

        BufferGroup* buffer_group = 0;
        bool buffer_initialized = false;
        int buffer_size = 0;
        
        Shader* shader = 0;

        std::string buffer_name;

        int constructed_component_count = 0;

        ECS(
            WINDOW* initial_window_ptr,
            const std::filesystem::path& save_path,
            const std::function<bool(ECS&)>& register_all_components_fn = {}
        ):
            window_ptr(initial_window_ptr),
            save_path(save_path),
            components_registered(RegisterComponents(register_all_components_fn)),
            draw_mg(
                "Entitys", [&](auto buffer_group, auto& entity) -> DrawTuples {
                    return {
                        {shader, entity.GetVertexCount(buffer_group, entity)}
                    };
                },
                "Cameras", [&](auto buffer_group, auto camera_index) -> CameraTuple {
                    auto camera_it = indexed_camera_groups.find(camera_index);
                    assert(camera_it != indexed_camera_groups.end());
                    return {camera_index, camera_it->second.framebuffer, [&, camera_index]() {
                        shader_update_push_constant(shader, 0, (void*)&camera_index, sizeof(uint32_t));
                    }};
                }
            ),
            buffer_name("Entitys") {
            RegisterProviders();
            buffer_group = CreateBufferGroup();
            shader = GenerateShader();
            EnableDrawInWindow(window_ptr);
            if (!LoadFromIO()) {
                EnsureFirstScene();
                AddCameraToScene(add_scene_id, Camera::Perspective);
            } else {
                loaded_from_io = true;
            }
            if (!buffer_initialized) {
                buffer_group_initialize(buffer_group);
                buffer_initialized = true;
            }
        };

        bool LoadFromIO() {
            std::ifstream stream(save_path, std::ios::in | std::ios::binary);
            Serial ioSerial(stream);
            if (!ioSerial.canRead() || ioSerial.getReadLength() == 0)
                return false;
            if (!DeserializeGroups(ioSerial, id_entity_groups))
                return false;
            if (!DeserializeGroups(ioSerial, indexed_camera_groups))
                return false;
            if (!DeserializeGroups(ioSerial, indexed_light_groups))
                return false;
            if (!DeserializeGroups(ioSerial, id_scene_groups))
                return false;
            if (!DeserializeBuffers(ioSerial))
                return false;
            UpdateGroupsChildren(id_entity_groups);
            UpdateGroupsChildren(indexed_camera_groups);
            UpdateGroupsChildren(indexed_light_groups);
            UpdateGroupsChildren(id_scene_groups);
            return true;
        }

        bool SaveToIO() {
            std::ofstream stream(save_path, std::ios::out | std::ios::binary | std::ios::trunc);
            Serial ioSerial(stream);
            if (!ioSerial.canWrite())
                return false;
            if (!SerializeGroups(ioSerial, id_entity_groups))
                return false;
            if (!SerializeGroups(ioSerial, indexed_camera_groups))
                return false;
            if (!SerializeGroups(ioSerial, indexed_light_groups))
                return false;
            if (!SerializeGroups(ioSerial, id_scene_groups))
                return false;
            if (!SerializeBuffers(ioSerial))
                return false;
            return true;
        }

        template <typename KeyT, typename GroupT>
        bool SerializeGroups(Serial& ioSerial, const std::map<KeyT, GroupT>& groups) {
            ioSerial << groups.size();
            for (auto& [id, group] : groups) {
                ioSerial << id;
                if (!group.serialize(*this, ioSerial))
                    return false;
            }
            return true;
        }

        template <typename KeyT, typename GroupT>
        bool DeserializeGroups(Serial& ioSerial, std::map<KeyT, GroupT>& groups) {
            auto size = groups.size();
            ioSerial >> size;
            for (size_t count = 1; count <= size; ++count) {
                KeyT id;
                ioSerial >> id;
                auto& group = groups[id];
                if (!group.deserialize(*this, ioSerial))
                    return false;
                if constexpr (std::is_same_v<GroupT, CameraReflectableGroup>) {
                    auto& width = *window_get_width_ref(window_ptr);
                    auto& height = *window_get_height_ref(window_ptr);
                    group.InitFramebuffer(*this, width, height);
                }
            }
            return true;
        }

        template <typename KeyT, typename GroupT>
        void UpdateGroupsChildren(std::map<KeyT, GroupT>& groups) {
            for (auto& [id, group] : groups) {
                group.UpdateChildren(*this);
                if constexpr (std::is_same_v<GroupT, EntityComponentReflectableGroup>)
                    group.UpdateReflectables();
            }
        }

        bool DeserializeBuffers(Serial& ioSerial) {
            if (!buffer_group)
                return false;
            auto keys_size = restricted_keys.size();
            ioSerial >> keys_size;
            for (size_t key_count = 1; key_count <= keys_size; ++key_count) {
                std::string restricted_key;
                ioSerial >> restricted_key;
                uint32_t element_count, element_size;
                ioSerial >> element_count >> element_size;
                auto buffer_size = element_count * element_size;
                auto r_key_it = std::find(restricted_keys.begin(), restricted_keys.end(), restricted_key);
                if (r_key_it == restricted_keys.end()) {
                    auto bytes = (char*)malloc(buffer_size);
                    ioSerial.readBytes((char*)bytes, buffer_size);
                    free(bytes);
                    continue;
                }
                buffer_group_set_buffer_element_count(buffer_group, restricted_key, element_count);
                auto actual_element_size = buffer_group_get_buffer_element_size(buffer_group, restricted_key);
                if (actual_element_size != element_size)
                    throw std::runtime_error("Incompatible buffer element sizes");
                auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, restricted_key);
                ioSerial.readBytes((char*)buffer_ptr.get(), buffer_size);
            }
            return true;
        }

        bool SerializeBuffers(Serial& ioSerial) {
            if (!buffer_group)
                return false;
            auto keys_size = restricted_keys.size();
            ioSerial << keys_size;
            for (auto& restricted_key : restricted_keys) {
                ioSerial << restricted_key;
                auto element_count = buffer_group_get_buffer_element_count(buffer_group, restricted_key);
                auto element_size = buffer_group_get_buffer_element_size(buffer_group, restricted_key);
                ioSerial << element_count << element_size;
                auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, restricted_key);
                ioSerial.writeBytes((const char*)buffer_ptr.get(), element_count * element_size);
            }
            return true;
        }

        void RegisterProviders() {
            RegisterProvider<TEntity>();
            (RegisterProvider<TProviders>(), ...);
        }

        template <typename TProvider>
        void RegisterProvider() {
            auto priority = TProvider::GetPriority();
            auto& name = TProvider::GetProviderName();
            auto& struct_name = TProvider::GetStructName();
            auto& glsl_struct = TProvider::GetGLSLStruct();
            auto& glsl_methods = TProvider::GetGLSLMethods();
            auto& priority_glsl_main = TProvider::GetGLSLMain();
            auto provider_id = GlobalUID::GetNew("ECS:Provider");
            prioritized_provider_ids[priority].push_back(provider_id);
            auto provider_index = id_provider_groups.size();
            auto& provider_group = id_provider_groups[provider_id];
            provider_group.index = provider_index;
            provider_group.id = provider_id;
            provider_group.name = name;
            provider_group.struct_name = struct_name;
            provider_group.glsl_struct = glsl_struct;
            provider_group.glsl_methods = glsl_methods;
            for (auto& [main_priority, main_string, module_type] : priority_glsl_main) {
                priority_glsl_mains[module_type][main_priority].push_back(main_string);
            }
        }

        void EnsureFirstScene() {
            auto scene_id = GlobalUID::GetNew("ECS:Scene");
            auto& scene_group = id_scene_groups[scene_id];

            scene_group.id = scene_id;
            scene_group.name = "Scene #" + std::to_string(scene_id);

            add_scene_id = scene_id;
        }

        bool RegisterComponents(const std::function<bool(ECS&)>& register_all_components_fn) {
            if (register_all_components_fn) {
                return register_all_components_fn(*this);
            }
            return false;
        }

        void Resize(int n) {
            if (buffer_group) {
                buffer_group_set_buffer_element_count(buffer_group, buffer_name, n);
                buffer_group_set_buffer_element_count(buffer_group, "Components", n * ECS_MAX_COMPONENTS);
                for (auto& component_pair : registered_component_map) {
                    auto& entry = component_pair.second;
                    buffer_group_set_buffer_element_count(buffer_group, entry.struct_name + "s", n * ECS_MAX_COMPONENTS);
                }
            }
            buffer_size = n;
        }

        size_t AddCameraToScene(size_t scene_id, Camera::ProjectionType projectionType) {
            auto camera_id = GlobalUID::GetNew("ECS:Camera");
            auto index = indexed_camera_groups.size();
            auto& camera_group = indexed_camera_groups[index];
            camera_group.index = index;
            camera_group.id = camera_id;
            camera_group.scene_id = scene_id;
            camera_group.name = ("Camera #" + std::to_string(camera_id));
            camera_group.NotifyNameChanged();
            if (buffer_group) {
                auto camera_buffer_size = buffer_group_get_buffer_element_count(buffer_group, cameras_buffer_name);
                if ((index + 1) > camera_buffer_size) {
                    buffer_group_set_buffer_element_count(buffer_group, cameras_buffer_name, (index + 1));
                }
            }
            auto camera_ptr = GetCamera(index);
            assert(camera_ptr);
            auto& camera = *camera_ptr;
            auto& width = *window_get_width_ref(window_ptr);
            auto& height = *window_get_height_ref(window_ptr);
            // Initialize camera matrices
            {
                switch(projectionType) {
                case Camera::Perspective:
                    CameraInit(camera, {0, 0, 10}, {0, 0, 0}, {0, 1, 0}, 0.25f, 1000.f, width, height, radians(81.f));
                    break;
                case Camera::Orthographic:
                    CameraInit(camera, {0, 0, 10}, {0, 0, 0}, {0, 1, 0}, 0.25f, 1000.f, vec<float, 4>(0, 0, width, height));
                    break;
                }
            }
            camera_group.InitFramebuffer(*this, width, height);
            // Setup Reflection
            {
                auto& scene_group = id_scene_groups[scene_id];
                scene_group.UpdateChildren(*this);
                camera_group.UpdateChildren(*this);
            }
            draw_mg.SetDirty();
            return camera_id;
        }

        size_t AddLightToScene(size_t scene_id, ecs::Light::LightType lightType) {
            auto light_id = GlobalUID::GetNew("ECS:Light");
            auto index = indexed_light_groups.size();
            auto& light_group = indexed_light_groups[index];
            light_group.index = index;
            light_group.id = light_id;
            light_group.scene_id = scene_id;
            light_group.name = ("Light #" + std::to_string(light_id));
            light_group.NotifyNameChanged();
            if (buffer_group) {
                auto light_buffer_size = buffer_group_get_buffer_element_count(buffer_group, lights_buffer_name);
                if ((index + 1) > light_buffer_size) {
                    buffer_group_set_buffer_element_count(buffer_group, lights_buffer_name, (index + 1));
                }
            }
            auto light_ptr = GetLight(index);
            assert(light_ptr);
            auto& light = *light_ptr;
            // Initialize light matrices
            {
                light.type = (int32_t)lightType;
                switch(lightType) {
                case ecs::Light::Directional:
                    // LightInit(light, {0, 0, 10}, {0, 0, 0}, {0, 1, 0}, 0.25f, 1000.f, width, height, radians(81.f));
                    break;
                case ecs::Light::Spot:
                    // LightInit(light, {0, 0, 10}, {0, 0, 0}, {0, 1, 0}, 0.25f, 1000.f, vec<float, 4>(0, 0, width, height));
                    break;
                case ecs::Light::Point:
                    // LightInit(light, {0, 0, 10}, {0, 0, 0}, {0, 1, 0}, 0.25f, 1000.f, vec<float, 4>(0, 0, width, height));
                    break;
                }
            }
            // Setup Reflection
            {
                auto& scene_group = id_scene_groups[scene_id];
                scene_group.UpdateChildren(*this);
                light_group.UpdateChildren(*this);
            }
            // draw_mg.SetDirty();
            return light_id;
        }

        int AddEntity(const TEntity& entity, bool is_child = false) {
            auto id = GlobalUID::GetNew("ECS:Entity");
            ((TEntity&)entity).id = id;
            auto index = id_entity_groups.size();
            auto& entry = id_entity_groups[id];
            entry.index = index;
            entry.scene_id = add_scene_id;
            entry.name = ("Entity #" + std::to_string(id));
            entry.is_child = is_child;
            if (buffer_group) {
                buffer_size = buffer_group_get_buffer_element_count(buffer_group, buffer_name);
                if ((index + 1) > buffer_size) {
                    Resize(buffer_size * 2);
                }
                auto entity_ptr = GetEntity(id);
                if (!entity_ptr)
                    assert(false);
                *entity_ptr = entity;
            }
            auto& scene_group = id_scene_groups[add_scene_id];
            scene_group.UpdateChildren(*this);
            return id;
        }

        template <typename... Args>
        std::vector<int> AddEntitys(const TEntity& first_entity, const Args&... rest_entitys) {
            return AddEntitys(false, first_entity, rest_entitys...);
        }

        template <typename... Args>
        std::vector<int> AddEntitys(bool is_child, const TEntity& first_entity, const Args&... rest_entitys) {
            auto n = 1 + sizeof...(rest_entitys);
            std::vector<int> ids(n, 0);
            auto ids_data = ids.data();
            auto index = id_entity_groups.size();
            for (int c = 1; c <= n; c++) {
                auto id = GlobalUID::GetNew("ECS:Entity");
                auto& entry = id_entity_groups[id];
                entry.index = index;
                entry.scene_id = add_scene_id;
                entry.name = ("Entity #" + std::to_string(id));
                entry.is_child = is_child;
                index++;
                ids_data[c - 1] = id;
            }
            if (index > buffer_size)
                Resize(index);
            size_t id_index = 0;
            SetEntitys(ids_data, id_index, first_entity, rest_entitys...);
            auto& scene_group = id_scene_groups[add_scene_id];
            scene_group.UpdateChildren(*this);
            return ids;
        }

        template <typename... Args>
        void SetEntitys(int* ids_data, size_t& id_index, const TEntity& set_entity, const Args&... rest_entitys) {
            auto& id = ids_data[id_index++];
            auto entity_ptr = GetEntity(id);
            if (entity_ptr) {
                ((TEntity&)set_entity).id = id;
                *entity_ptr = set_entity;
            }
            SetEntitys(ids_data, id_index, rest_entitys...);
        }

        void SetEntitys(int* ids_data, size_t& id_index) { }

        int AddChildEntity(int entity_id, const TEntity& entity) {
            auto it = id_entity_groups.find(entity_id);
            if (it == id_entity_groups.end()) {
                return 0;
            }
            auto& entry = it->second;
            auto child_id = AddEntity(entity, true);
            entry.children.insert(child_id);
            entry.UpdateChildren(*this);
            return child_id;
        }

        template <typename... Args>
        std::vector<int> AddChildEntitys(int entity_id, const TEntity& first_entity, const Args&... rest_entitys) {
            auto it = id_entity_groups.find(entity_id);
            if (it == id_entity_groups.end()) {
                return {};
            }
            auto& entry = it->second;
            auto child_ids = AddEntitys(true, first_entity, rest_entitys...);
            for (auto& child_id : child_ids)
                entry.children.insert(child_id);
            entry.UpdateChildren(*this);
            return child_ids;
        }

        TEntity* GetEntity(int id) {
            static auto entity_size = sizeof(TEntity);
            auto it = id_entity_groups.find(id);
            if (it == id_entity_groups.end()) {
                return nullptr;
            }
            auto& entry = it->second;
            auto index = entry.index;
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, buffer_name);
            return (TEntity*)(buffer_ptr.get() + (entity_size * index));
        }

        Camera* GetCamera(int camera_index) {
            static auto camera_size = sizeof(Camera);
            auto it = indexed_camera_groups.find(camera_index);
            if (it == indexed_camera_groups.end()) {
                return nullptr;
            }
            auto& camera_entry = it->second;
            auto index = camera_entry.index;
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, cameras_buffer_name);
            return (Camera*)(buffer_ptr.get() + (camera_size * index));
        }

        ecs::Light* GetLight(int light_index) {
            static auto light_size = sizeof(ecs::Light);
            auto it = indexed_light_groups.find(light_index);
            if (it == indexed_light_groups.end()) {
                return nullptr;
            }
            auto& light_entry = it->second;
            auto index = light_entry.index;
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, lights_buffer_name);
            return (ecs::Light*)(buffer_ptr.get() + (light_size * index));

        }
        
        void SetProviderCount(const std::string& buffer_name, int count) {
            if (buffer_group) {
                auto buffer_size = buffer_group_get_buffer_element_count(buffer_group, buffer_name);
                if (count > buffer_size) {
                    buffer_group_set_buffer_element_count(buffer_group, buffer_name, count);
                }
            }
        }

        template <typename T>
        T* GetProviderData(const std::string& buffer_name) {
            if (buffer_group) {
                auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, buffer_name);
                return (T*)buffer_ptr.get();
            }
            return nullptr;
        }

        template<typename TRComponent>
        bool RegisterComponent() {
            auto ID = int(TComponenTypeID<TRComponent>::id);
            auto& StructName = ComponentStructName<TRComponent>::string;
            auto ecs_ptr = this;
            registered_component_map[ID] = {
                .type_id = ID,
                .struct_name = StructName,
                .construct_component_fn = [ecs_ptr]() -> std::shared_ptr<TComponent> {
                    auto ptr = std::shared_ptr<TRComponent>(new TRComponent{}, [](TRComponent* dp){ delete dp; });
                    ptr->i = ecs_ptr;
                    return ptr;
                }
            };
            RegisterProvider<TRComponent>();
            return true;
        }

        template<typename TDComponent>
        TDComponent& ConstructComponent(int entity_id, const TDComponent::DataT& original_data) {
            auto it = id_entity_groups.find(entity_id);
            if (it == id_entity_groups.end()) {
                throw std::runtime_error("entity not found with passed id!");
            }

            auto entity_ptr = GetEntity(entity_id);
            assert(entity_ptr);

            auto& entry = it->second;

            auto component_id = GlobalUID::GetNew("ECS:Component");
            auto component_type_id = TComponenTypeID<TDComponent>::id;

            auto& component_group = registered_component_map[component_type_id];
            auto component_type_index = component_group.constructed_count++;

            auto& ucom = entry.components[component_id];

            auto component_index = constructed_component_count++;
            
            ucom = std::shared_ptr<TDComponent>(new TDComponent{}, [](TDComponent* dp){ delete dp; });
            entry.UpdateReflectables();

            auto& aucom = *ucom;
            aucom.index = component_index;
            aucom.id = component_id;
            aucom.i = this;

            auto& root_data = GetComponentRootData(aucom.index);

            root_data.index = component_index;
            root_data.type = component_type_id;
            root_data.type_index = component_type_index;
            root_data.data_size = sizeof(typename TDComponent::DataT);

            auto& entity = *entity_ptr;
            auto entity_component_index = entity.componentsCount++;
            entity.components[entity_component_index] = component_index;

            GetComponentData<TDComponent>(aucom.index) = original_data;

            return *std::dynamic_pointer_cast<TDComponent>(ucom);
        }

        TComponent::DataT& GetComponentRootData(int component_index) {
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, "Components");
            return *(typename TComponent::DataT*)(buffer_ptr.get() + (sizeof(typename TComponent::DataT) * component_index));
        }

        template<typename TAComponent>
        TAComponent::DataT& GetComponentData(int component_index) {
            return *(typename TAComponent::DataT*)GetComponentDataVoid(component_index);
        }

        void* GetComponentDataVoid(int component_index) override {
            auto& root_data = GetComponentRootData(component_index);

            auto& type_id = root_data.type;
            auto& type_index = root_data.type_index;

            auto type_it = registered_component_map.find(type_id);
            if (type_it == registered_component_map.end())
                throw std::runtime_error("type_id not registered!");

            auto& type_group = type_it->second;
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, type_group.struct_name + "s");

            return (buffer_ptr.get() + (root_data.data_size * type_index));
        }

        BufferGroup* CreateBufferGroup() {
            auto buffer_group_ptr = buffer_group_create("EntitysGroup");
            for (auto& [provider_id, provider_group] : id_provider_groups) {
                restricted_keys.push_back(provider_group.struct_name + "s");
            }
            buffer_group_restrict_to_keys(buffer_group_ptr, restricted_keys);
            return buffer_group_ptr;
        }

        Shader* GenerateShader() {
            auto shader_ptr = shader_create();

            shader_set_define(shader_ptr, "ECS_MAX_COMPONENTS", std::to_string(ECS_MAX_COMPONENTS));

            shader_add_buffer_group(shader_ptr, buffer_group);

            shader_add_module(shader_ptr, ShaderModuleType::Vertex, GenerateVertexShader());
            shader_add_module(shader_ptr, ShaderModuleType::Fragment, GenerateFragmentShader());

            return shader_ptr;
        }

        void EnableDrawInWindow(WINDOW* window_ptr) {
            window_add_drawn_buffer_group(window_ptr, &draw_mg, buffer_group);

            window_register_free_callback(window_ptr, [&]() mutable {
                if (!SaveToIO())
                    std::cerr << "Failed to Serialize ECS" << std::endl;
                indexed_camera_groups.clear();
                id_scene_groups.clear();
            });
        }

        std::string GenerateVertexShader() {
            auto binding_index = 0;

            std::string shader_string = R"(
#version 450
layout(location = 0) out int outID;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec4 outPosition;

layout(push_constant) uniform PushConstants {
    int camera_index;
} pc;
)";
            // Setup Structs
            shader_string += TComponent::ComponentGLSLStruct;
            for (auto& [priority, provider_ids] : prioritized_provider_ids) {
                for (auto& provider_id : provider_ids) {
                    auto& provider_group = id_provider_groups[provider_id];
                    shader_string += provider_group.glsl_struct;
                }
            }

            // Setup Buffers
            for (auto& [priority, provider_ids] : prioritized_provider_ids) {
                for (auto& provider_id : provider_ids) {
                    auto& provider_group = id_provider_groups[provider_id];
                    shader_string += R"(
layout(std430, binding = )" + std::to_string(binding_index++) + ") buffer " + provider_group.struct_name + R"(Buffer {
    )" + provider_group.struct_name + R"( data[];
} )" + provider_group.struct_name + R"(s;

)";
                    shader_string += provider_group.struct_name + " Get" + provider_group.struct_name + R"(Data(int t_provider_index) {
    return )" + provider_group.struct_name + R"(s.data[t_provider_index];
}
)";
                    shader_string += provider_group.glsl_methods;
                }
            }

            // TComponent
            shader_string += R"(
layout(std430, binding = )" + std::to_string(binding_index++) + R"() buffer ComponentBuffer {
    Component data[];
} Components;
)";
            shader_string += TComponent::ComponentGLSLMethods;

            // Main
            shader_string += R"(
void main() {
    outID = gl_InstanceIndex;
    Entity entity = GetEntityData(outID);
    vec4 final_color;
    vec4 final_position;
    int t_component_index = -1;
)";

            // Main Get Components
            for (auto& [id, entry] : registered_component_map) {
                auto struct_name_lower = to_lower(entry.struct_name);
                shader_string += "    " + entry.struct_name + " " + struct_name_lower + ";\n";
                shader_string += R"(    if (HasComponentWithType(entity, )" + std::to_string(entry.type_id) + R"(, t_component_index))
        )" + struct_name_lower + " = Get" + entry.struct_name + "Data(t_component_index);\n";
            }

            // Priority Mains
            auto& module_glsl_mains = priority_glsl_mains[ShaderModuleType::Vertex];
            for (auto& [priority, string_vec] : module_glsl_mains) {
                for (auto& string : string_vec) {
                    shader_string += string;
                }
            }

            // Main Output
            shader_string += R"(
    final_position = camera_position;
    gl_Position = final_position;
    outColor = final_color;
    outPosition = final_position;
}
)";

            return shader_string;
        }

        std::string GenerateFragmentShader() {
            auto binding_index = 0;

            std::string shader_string = R"(
#version 450
layout(location = 0) flat in int inID;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inPosition;

layout(location = 0) out vec4 FragColor;

layout(push_constant) uniform PushConstants {
    int camera_index;
} pc;
)";

            // Setup Structs
            shader_string += TComponent::ComponentGLSLStruct;
            for (auto& [priority, provider_ids] : prioritized_provider_ids) {
                for (auto& provider_id : provider_ids) {
                    auto& provider_group = id_provider_groups[provider_id];
                    shader_string += provider_group.glsl_struct;
                }
            }

            // Setup Buffers
            for (auto& [priority, provider_ids] : prioritized_provider_ids) {
                for (auto& provider_id : provider_ids) {
                    auto& provider_group = id_provider_groups[provider_id];
                    shader_string += R"(
layout(std430, binding = )" + std::to_string(binding_index++) + ") buffer " + provider_group.struct_name + R"(Buffer {
    )" + provider_group.struct_name + R"( data[];
} )" + provider_group.struct_name + R"(s;

)";
                    shader_string += provider_group.struct_name + " Get" + provider_group.struct_name + R"(Data(int t_provider_index) {
    return )" + provider_group.struct_name + R"(s.data[t_provider_index];
}
)";
                }
            }

            // TComponent
            shader_string += R"(
layout(std430, binding = )" + std::to_string(binding_index++) + R"() buffer ComponentBuffer {
    Component data[];
} Components;
)";
            shader_string += TComponent::ComponentGLSLMethods;

            shader_string += R"(
void main() {
    Entity entity = GetEntityData(inID);
    int t_component_index = -1;
)";
    
            // Main Get Components
            for (auto& [id, entry] : registered_component_map) {
                auto struct_name_lower = to_lower(entry.struct_name);
                shader_string += "    " + entry.struct_name + " " + struct_name_lower + ";\n";
                shader_string += R"(    if (HasComponentWithType(entity, )" + std::to_string(entry.type_id) + R"(, t_component_index))
        )" + struct_name_lower + " = Get" + entry.struct_name + "Data(t_component_index);\n";
            }

            // Priority Mains
            auto& module_glsl_mains = priority_glsl_mains[ShaderModuleType::Fragment];
            for (auto& [priority, string_vec] : module_glsl_mains) {
                for (auto& string : string_vec) {
                    shader_string += string;
                }
            }
            shader_string += R"(
    FragColor = inColor;
}
)";
            return shader_string;
        }

        std::vector<size_t> GetSceneIDs() {
            std::vector<size_t> ids(id_scene_groups.size(), 0);
            size_t i = 0;
            for (auto& [id, entry] : id_scene_groups)
                ids[i++] = id;
            return ids;
        }

        std::map<size_t, SceneReflectableGroup>::iterator GetScenesBegin() {
            return id_scene_groups.begin();
        }

        std::map<size_t, SceneReflectableGroup>::iterator GetScenesEnd() {
            return id_scene_groups.end();
        }

        std::map<int, CameraReflectableGroup>::iterator GetCamerasBegin() {
            return indexed_camera_groups.begin();
        }

        std::map<int, CameraReflectableGroup>::iterator GetCamerasEnd() {
            return indexed_camera_groups.end();
        }

        Image* GetFramebufferImage(size_t camera_index) {
            auto it = indexed_camera_groups.find(camera_index);
            if (it == indexed_camera_groups.end())
                return nullptr;
            return it->second.fb_color_image;
        }

        bool ResizeFramebuffer(size_t camera_index, uint32_t width, uint32_t height) {
            auto it = indexed_camera_groups.find(camera_index);
            if (it == indexed_camera_groups.end())
                return false;
            auto& camera_group = it->second;
            auto fb_resized = framebuffer_resize(camera_group.framebuffer, width, height);
            if (!fb_resized)
                return false;
            if (FramebufferChanged(camera_index)) {
                camera_group.fb_color_image = framebuffer_get_image(camera_group.framebuffer, AttachmentType::Color, true);
                camera_group.fb_depth_image = framebuffer_get_image(camera_group.framebuffer, AttachmentType::Depth, true);
                SetCameraAspect(camera_index, width, height);
                auto frame_ds_pair = image_create_descriptor_set(camera_group.fb_color_image);
                camera_group.frame_image_ds = frame_ds_pair.second;
            }
            return true;
        }

        bool FramebufferChanged(size_t camera_index) {
            auto it = indexed_camera_groups.find(camera_index);
            if (it == indexed_camera_groups.end())
                return false;
            auto& camera_group = it->second;
            return framebuffer_changed(camera_group.framebuffer);
        }

        bool SetCameraAspect(size_t camera_index, float width, float height) {
            if (!width || !height)
                return false;
            auto camera_ptr = GetCamera(camera_index);
            if (!camera_ptr)
                return false;
            auto& camera = *camera_ptr;
            camera.orthoWidth = width;
            camera.orthoHeight = height;
            switch (Camera::ProjectionType(camera.type)) {
            case Camera::Perspective:
                camera.aspect = camera.orthoWidth / camera.orthoHeight;
                break;
            default: break;
            }
            CameraInit(camera);
            return true;
        }

    };
}