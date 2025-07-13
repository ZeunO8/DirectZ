#pragma once
#include "GlobalUID.hpp"
#include <unordered_map>
#include <memory>
#include "BufferGroup.hpp"
#include "Framebuffer.hpp"
#include "DrawListManager.hpp"
#include "Window.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Util.hpp"
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <set>
#define ECS_MAX_COMPONENTS 8
namespace dz {
    template <typename T>
    struct ComponentTypeID;

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
    struct ComponentTypeID<TYPE> { \
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

    inline static std::string cameras_buffer_name = "Cameras";
    
    template<typename EntityT, typename ComponentT, typename SystemT>
    struct ECS {

        struct EntityComponentReflectableGroup : ReflectableGroup {
            size_t scene_id = 0;
            std::string name;
            std::unordered_map<int, std::shared_ptr<ComponentT>> components;
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
                    auto id_it = ecs.id_entity_entries.find(id);
                    assert(id_it != ecs.id_entity_entries.end());
                    auto& child_entry = id_it->second;
                    reflectable_children.push_back(&child_entry);
                }
            }
        };

        struct SceneReflectableGroup : ReflectableGroup {
            std::string name;
            Framebuffer* framebuffer = 0;
            Image* fb_color_image = 0;
            Image* fb_depth_image = 0;
            std::vector<ReflectableGroup*> reflectable_children;
            ~SceneReflectableGroup() {
                framebuffer_destroy(framebuffer);
            }
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
                for (auto& [entity_id, entity_entry] : ecs.id_entity_entries) {
                    if (entity_entry.scene_id == id)
                        count++;
                }
                for (auto& [camera_id, camera_entry] : ecs.id_camera_entries) {
                    if (camera_entry.scene_id == id)
                        count++;
                }
                reflectable_children.reserve(count);
                for (auto& [entity_id, entity_entry] : ecs.id_entity_entries) {
                    if (entity_entry.scene_id == id)
                        reflectable_children.push_back(&entity_entry);
                }
                for (auto& [camera_id, camera_entry] : ecs.id_camera_entries) {
                    if (camera_entry.scene_id == id)
                        reflectable_children.push_back(&camera_entry);
                }
            }
        };

        struct RegisteredComponentEntry {
            int type_id;
            std::string component_name;
            std::string struct_name;
            std::string component_struct;
            std::string glsl_methods;
            std::string glsl_main;
            int constructed_count = 0;
        };

        struct CameraReflectableGroup : ReflectableGroup {
            size_t scene_id = 0;
            std::string name;
            std::vector<Reflectable*> reflectables;
            ~CameraReflectableGroup() {
                ClearChildren();
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Camera;
            }
            std::string& GetName() override {
                return name;
            }
            const std::vector<Reflectable*>& GetReflectables() override {
                return reflectables;
            }
            void UpdateChildren(ECS& ecs) {
                reflectables.clear();
                auto ecs_ptr = &ecs;
                auto camera_id = id;
                reflectables.push_back(new CameraTypeReflectable([ecs_ptr, camera_id]() mutable {
                    return ecs_ptr->GetCamera(camera_id);
                }));
                reflectables.push_back(new CameraViewReflectable([ecs_ptr, camera_id]() mutable {
                    return ecs_ptr->GetCamera(camera_id);
                }));
                auto camera_ptr = ecs.GetCamera(id);
                assert(camera_ptr);
                auto& camera = *camera_ptr;
                switch (Camera::ProjectionType(camera.type)) {
                case Camera::Perspective:
                    reflectables.push_back(new CameraPerspectiveReflectable([ecs_ptr, camera_id]() mutable {
                        return ecs_ptr->GetCamera(camera_id);
                    }));
                    break;
                case Camera::Orthographic:
                    reflectables.push_back(new CameraOrthographicReflectable([ecs_ptr, camera_id]() mutable {
                        return ecs_ptr->GetCamera(camera_id);
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
        };

        WINDOW* window_ptr = 0;

        std::map<int, EntityComponentReflectableGroup> id_entity_entries;
        std::map<size_t, SceneReflectableGroup> id_scene_entries;
        std::map<size_t, CameraReflectableGroup> id_camera_entries;
        size_t add_scene_id;

        std::map<int, RegisteredComponentEntry> registered_component_map;
        bool components_registered = false;

        DrawListManager<EntityT> draw_mg;

        BufferGroup* buffer_group = 0;
        bool buffer_initialized = false;
        int buffer_size = 0;
        
        Shader* shader = 0;

        std::string buffer_name;

        int constructed_component_count = 0;

        ECS(WINDOW* initial_window_ptr, const std::function<bool(ECS&)>& register_all_components_fn = {}):
            window_ptr(initial_window_ptr),
            components_registered(RegisterComponents(register_all_components_fn)),
            draw_mg("Entities", [&](auto buffer_group, auto& entity) -> DrawTuple {
                auto entity_it = id_entity_entries.find(entity.id);
                assert(entity_it != id_entity_entries.end());
                auto scene_it = id_scene_entries.find(entity_it->second.scene_id);
                assert(scene_it != id_scene_entries.end());
                return {scene_it->second.framebuffer, shader, entity.GetVertexCount()};
            }),
            buffer_group(CreateBufferGroup()),
            shader(GenerateShader()),
            buffer_name("Entities")
        {
            EnableDrawInWindow(window_ptr);
            EnsureFirstScene();
            AddCameraToScene(add_scene_id, Camera::Perspective);
        };

        void EnsureFirstScene() {
            auto scene_id = GlobalUID::GetNew("ECS:Scene");
            auto& scene_entry = id_scene_entries[scene_id];

            scene_entry.id = scene_id;
            scene_entry.name = "Scene #" + std::to_string(scene_id);

            add_scene_id = scene_id;

            {
                auto& width = *window_get_width_ref(window_ptr);
                auto& height = *window_get_height_ref(window_ptr);
                scene_entry.fb_color_image = image_create({
                    .width = uint32_t(width),
                    .height = uint32_t(height),
                    .is_framebuffer_attachment = true
                });
                scene_entry.fb_depth_image = image_create({
                    .width = uint32_t(width),
                    .height = uint32_t(height),
                    .format = VK_FORMAT_D32_SFLOAT,
                    .is_framebuffer_attachment = true
                });
                Image* fb_images[2] = {
                    scene_entry.fb_color_image,
                    scene_entry.fb_depth_image
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
                scene_entry.framebuffer = framebuffer_create(fb_info);

                shader_set_render_pass(shader, scene_entry.framebuffer);
            }
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
            if (!buffer_initialized) {
                buffer_group_initialize(buffer_group);
                buffer_initialized = true;
            }
        }

        size_t AddCameraToScene(size_t scene_id, Camera::ProjectionType projectionType) {
            auto camera_id = GlobalUID::GetNew("ECS:Camera");
            auto index = id_camera_entries.size();
            auto& camera_entry = id_camera_entries[camera_id];
            camera_entry.index = index;
            camera_entry.id = camera_id;
            camera_entry.scene_id = scene_id;
            camera_entry.name = ("Camera #" + std::to_string(camera_id));
            if (buffer_group) {
                auto camera_buffer_size = buffer_group_get_buffer_element_count(buffer_group, cameras_buffer_name);
                if ((index + 1) > camera_buffer_size) {
                    buffer_group_set_buffer_element_count(buffer_group, cameras_buffer_name, (index + 1));
                }
            }
            auto camera_ptr = GetCamera(camera_id);
            assert(camera_ptr);
            auto& camera = *camera_ptr;
            auto& width = *window_get_width_ref(window_ptr);
            auto& height = *window_get_height_ref(window_ptr);
            switch(projectionType) {
            case Camera::Perspective:
                CameraInit(camera, {0, 0, 10}, {0, 0, 0}, {0, 1, 0}, 0.25f, 1000.f, width / height, radians(81.f));
                break;
            case Camera::Orthographic:
                CameraInit(camera, {0, 0, 10}, {0, 0, 0}, {0, 1, 0}, 0.25f, 1000.f, vec<float, 4>(0, 0, width, height));
                break;
            }
            auto& scene_entry = id_scene_entries[scene_id];
            scene_entry.UpdateChildren(*this);
            camera_entry.UpdateChildren(*this);
            return camera_id;
        }

        int AddEntity(const EntityT& entity, bool is_child = false) {
            auto id = GlobalUID::GetNew("ECS:Entity");
            ((EntityT&)entity).id = id;
            auto index = id_entity_entries.size();
            auto& entry = id_entity_entries[id];
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
            auto& scene_entry = id_scene_entries[add_scene_id];
            scene_entry.UpdateChildren(*this);
            return id;
        }

        template <typename... Args>
        std::vector<int> AddEntities(const EntityT& first_entity, const Args&... rest_entities) {
            return AddEntities(false, first_entity, rest_entities...);
        }

        template <typename... Args>
        std::vector<int> AddEntities(bool is_child, const EntityT& first_entity, const Args&... rest_entities) {
            auto n = 1 + sizeof...(rest_entities);
            std::vector<int> ids(n, 0);
            auto ids_data = ids.data();
            auto index = id_entity_entries.size();
            for (int c = 1; c <= n; c++) {
                auto id = GlobalUID::GetNew("ECS:Entity");
                auto& entry = id_entity_entries[id];
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
            SetEntities(ids_data, id_index, first_entity, rest_entities...);
            auto& scene_entry = id_scene_entries[add_scene_id];
            scene_entry.UpdateChildren(*this);
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

        int AddChildEntity(int entity_id, const EntityT& entity) {
            auto it = id_entity_entries.find(entity_id);
            if (it == id_entity_entries.end()) {
                return 0;
            }
            auto& entry = it->second;
            auto child_id = AddEntity(entity, true);
            entry.children.insert(child_id);
            entry.UpdateChildren(*this);
            return child_id;
        }

        template <typename... Args>
        std::vector<int> AddChildEntities(int entity_id, const EntityT& first_entity, const Args&... rest_entities) {
            auto it = id_entity_entries.find(entity_id);
            if (it == id_entity_entries.end()) {
                return {};
            }
            auto& entry = it->second;
            auto child_ids = AddEntities(true, first_entity, rest_entities...);
            for (auto& child_id : child_ids)
                entry.children.insert(child_id);
            entry.UpdateChildren(*this);
            return child_ids;
        }

        EntityT* GetEntity(int id) {
            static auto entity_size = sizeof(EntityT);
            auto it = id_entity_entries.find(id);
            if (it == id_entity_entries.end()) {
                return nullptr;
            }
            auto& entry = it->second;
            auto index = entry.index;
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, buffer_name);
            return (EntityT*)(buffer_ptr.get() + (entity_size * index));
        }

        Camera* GetCamera(int camera_id) {
            static auto camera_size = sizeof(Camera);
            auto it = id_camera_entries.find(camera_id);
            if (it == id_camera_entries.end()) {
                return nullptr;
            }
            auto& entry = it->second;
            auto index = entry.index;
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, cameras_buffer_name);
            return (Camera*)(buffer_ptr.get() + (camera_size * index));
        }

        template<typename RComponentT>
        bool RegisterComponent() {
            auto ID = ComponentTypeID<RComponentT>::id;
            registered_component_map[ID] = {
                .type_id = ComponentTypeID<RComponentT>::id,
                .component_name = ComponentComponentName<RComponentT>::string,
                .struct_name = ComponentStructName<RComponentT>::string,
                .component_struct = ComponentStruct<RComponentT>::string,
                .glsl_methods = ComponentGLSLMethods<RComponentT>::string,
                .glsl_main = ComponentGLSLMain<RComponentT>::string
            };
            return true;
        }

        template<typename DComponentT>
        DComponentT& ConstructComponent(int entity_id, const DComponentT::DataT& original_data) {
            auto it = id_entity_entries.find(entity_id);
            if (it == id_entity_entries.end()) {
                throw std::runtime_error("entity not found with passed id!");
            }

            auto entity_ptr = GetEntity(entity_id);
            assert(entity_ptr);

            auto& entry = it->second;

            auto component_id = GlobalUID::GetNew("ECS:Component");
            auto component_type_id = ComponentTypeID<DComponentT>::id;

            auto& component_entry = registered_component_map[component_type_id];
            auto component_type_index = component_entry.constructed_count++;

            auto& ucom = entry.components[component_id];

            auto component_index = constructed_component_count++;
            
            ucom = std::shared_ptr<DComponentT>(new DComponentT{}, [](DComponentT* dp){ delete dp; });
            entry.UpdateReflectables();

            auto& aucom = *ucom;
            aucom.index = component_index;
            aucom.id = component_id;

            auto& root_data = aucom.GetRootData();

            root_data.index = component_index;
            root_data.type = component_type_id;
            root_data.type_index = component_type_index;
            root_data.data_size = sizeof(typename DComponentT::DataT);

            auto& entity = *entity_ptr;
            auto entity_component_index = entity.componentsCount++;
            entity.components[entity_component_index] = component_index;

            aucom.template GetData<DComponentT>() = original_data;

            return *std::dynamic_pointer_cast<DComponentT>(ucom);
        }

        ComponentT::DataT& GetComponentRootData(int component_index) {
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, "Components");
            return *(typename ComponentT::DataT*)(buffer_ptr.get() + (sizeof(typename ComponentT::DataT) * component_index));
        }

        template<typename AComponentT>
        AComponentT::DataT& GetComponentData(int component_index) {
            return *(typename AComponentT::DataT*)GetComponentDataVoid(component_index);
        }

        void* GetComponentDataVoid(int component_index) {
            auto& root_data = GetComponentRootData(component_index);

            auto& type_id = root_data.type;
            auto& type_index = root_data.type_index;

            auto type_it = registered_component_map.find(type_id);
            if (type_it == registered_component_map.end())
                throw std::runtime_error("type_id not registered!");

            auto& type_entry = type_it->second;
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, type_entry.struct_name + "s");

            return (buffer_ptr.get() + (root_data.data_size * type_index));
        }

        BufferGroup* CreateBufferGroup() {
            auto buffer_group_ptr = buffer_group_create("EntitiesGroup");
            std::vector<std::string> restricted_keys{"Entities", "Components", "Cameras"};
            for (auto& component_pair : registered_component_map) {
                auto& entry = component_pair.second;
                restricted_keys.push_back(entry.struct_name + "s");
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
                id_scene_entries.clear();
            });
        }

        std::string GenerateVertexShader() {
            auto binding_index = 0;
            auto shader_string = R"(
#version 450
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
)" +
Camera::GetGLSLStruct() +
R"(
layout(std430, binding = )" + std::to_string(binding_index++) + R"() buffer CamerasBuffer {
    Camera cameras[];
} Cameras;
)";
            shader_string += EntityT::GetGLSLStruct() +
R"(
layout(std430, binding = )" + std::to_string(binding_index++) + R"() buffer EntitiesBuffer {
    Entity entities[];
} Entities;
)" +
EntityT::GetGLSLEntityVertexFunction() +
EntityT::GetGLSLEntityVertexColorFunction();
            shader_string += ComponentT::GetGLSLStruct();
            shader_string += R"(
layout(std430, binding = )" + std::to_string(binding_index++) + R"() buffer ComponentBuffer {
    Component data[];
} Components;
)";
            shader_string += ComponentT::GetGLSLMethods();
            for (auto& [id, entry] : registered_component_map) {
                shader_string += entry.component_struct;
                shader_string += R"(
layout(std430, binding = )" + std::to_string(binding_index++) + ") buffer " + entry.struct_name + R"(Buffer {
    )" + entry.struct_name + R"( data[];
} )" + entry.struct_name + R"(s;

)";
                shader_string += entry.struct_name + " Get" + entry.struct_name + R"(Data(int t_component_index) {
    return )" + entry.struct_name + R"(s.data[t_component_index];
}
)";
                shader_string += entry.glsl_methods;
            }
            shader_string += R"(
void main() {
    Entity entity = Entities.entities[gl_InstanceIndex];
    vec4 vertex = vec4(GetEntityVertex(entity), 1.0);
    vec4 final_color = GetEntityVertexColor(entity);
    vec4 final_position = vertex;
    int t_component_index = -1;
)";
            for (auto& [id, entry] : registered_component_map) {
                auto struct_name_lower = to_lower(entry.struct_name);
                shader_string += "    " + entry.struct_name + " " + struct_name_lower + ";\n";
                shader_string += R"(    if (HasComponentWithType(entity, )" + std::to_string(entry.type_id) + R"(, t_component_index)) {
        )" + struct_name_lower + " = Get" + entry.struct_name + R"(Data(t_component_index);
)" + entry.glsl_main + R"(
    }
)";
            }
            shader_string += R"(
    if (Cameras.cameras.length() > 0) {
        final_position.y *= -1.0;
        Camera camera = Cameras.cameras[0];
        vec4 camera_position = camera.projection * camera.view * final_position;
        final_position = camera_position;
    }
    gl_Position = final_position;
    outColor = final_color;
    outPosition = final_position;
}
)";
            return shader_string;
        }

        std::string GenerateFragmentShader() {
            auto shader_string = R"(
#version 450
layout(location = 0) in vec4 inColor;
layout(location = 1) in vec4 inPosition;

layout(location = 0) out vec4 FragColor;

void main() {
    FragColor = inColor;
}
)";
            return shader_string;
        }

        std::vector<size_t> GetSceneIDs() {
            std::vector<size_t> ids(id_scene_entries.size(), 0);
            size_t i = 0;
            for (auto& [id, entry] : id_scene_entries)
                ids[i++] = id;
            return ids;
        }

        std::map<size_t, SceneReflectableGroup>::iterator GetScenesBegin() {
            return id_scene_entries.begin();
        }

        std::map<size_t, SceneReflectableGroup>::iterator GetScenesEnd() {
            return id_scene_entries.end();
        }

        Image* GetFramebufferImage(size_t scene_id) {
            auto it = id_scene_entries.find(scene_id);
            if (it == id_scene_entries.end())
                return nullptr;
            return it->second.fb_color_image;
        }

        bool ResizeFramebuffer(size_t scene_id, uint32_t width, uint32_t height) {
            auto it = id_scene_entries.find(scene_id);
            if (it == id_scene_entries.end())
                return false;
            auto& scene_entry = it->second;
            auto fb_resized = framebuffer_resize(scene_entry.framebuffer, width, height);
            if (!fb_resized)
                return false;
            scene_entry.fb_color_image = framebuffer_get_image(scene_entry.framebuffer, AttachmentType::Color, true);
            scene_entry.fb_depth_image = framebuffer_get_image(scene_entry.framebuffer, AttachmentType::Depth, true);
            return true;
        }

        bool FramebufferChanged(size_t scene_id) {
            auto it = id_scene_entries.find(scene_id);
            if (it == id_scene_entries.end())
                return false;
            auto& scene_entry = it->second;
            return framebuffer_changed(scene_entry.framebuffer);
        }

        bool SetCameraAspect(size_t camera_id, float width, float height) {
            auto camera_ptr = GetCamera(camera_id);
            if (!camera_ptr)
                return false;
            auto& camera = *camera_ptr;
            if (camera.type == 1) {
                camera.aspect = width / height;
            }
            else if (camera.type == 2) {
                camera.orthoWidth = width;
                camera.orthoHeight = height;
            }
            CameraInit(camera);
            return true;
        }

    };
}