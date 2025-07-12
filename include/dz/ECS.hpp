#pragma once
#include "GlobalUID.hpp"
#include <unordered_map>
#include <memory>
#include "BufferGroup.hpp"
#include "Framebuffer.hpp"
#include "DrawListManager.hpp"
#include "Window.hpp"
#include "Shader.hpp"
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
    
    template<typename EntityT, typename ComponentT, typename SystemT>
    struct ECS {

        struct EntityComponentEntry {
            int index;
            bool is_child;
            std::string name;
            std::unordered_map<int, std::shared_ptr<ComponentT>> components;
            std::set<int> children;
        };

        struct RegisteredComponentEntry {
            int type_id;
            std::string struct_name;
            std::string component_struct;
            std::string glsl_methods;
            std::string glsl_main;
            int constructed_count = 0;
        };

        std::map<int, EntityComponentEntry> id_entity_entries;

        std::map<int, RegisteredComponentEntry> registered_component_map;
        bool components_registered = false;
        DrawListManager<EntityT> draw_mg;
        Framebuffer* framebuffer = 0;
        Image* fb_image = 0;
        BufferGroup* buffer_group = 0;
        Shader* shader = 0;
        std::string buffer_name;
        bool buffer_initialized = false;
        int buffer_size = 0;

        int constructed_component_count = 0;

        ECS(const std::function<bool(ECS&)>& register_all_components_fn = {}):
            components_registered(RegisterComponents(register_all_components_fn)),
            draw_mg("Entities", [&](auto buffer_group, auto& entity) -> DrawTuple {
                return {framebuffer, shader, entity.GetVertexCount()};
            }),
            buffer_group(CreateBufferGroup()),
            shader(GenerateShader()),
            buffer_name("Entities")
        {};

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

        int AddEntity(const EntityT& entity, bool is_child = false) {
            auto id = GlobalUID::GetNew();
            ((EntityT&)entity).id = id;
            auto index = id_entity_entries.size();
            auto& entry = id_entity_entries[id];
            entry.index = index;
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
                auto id = GlobalUID::GetNew();
                auto& entry = id_entity_entries[id];
                entry.index = index;
                entry.name = ("Entity #" + std::to_string(id));
                entry.is_child = is_child;
                index++;
                ids_data[c - 1] = id;
            }
            if (index > buffer_size)
                Resize(index);
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

        int AddChildEntity(int entity_id, const EntityT& entity) {
            auto it = id_entity_entries.find(entity_id);
            if (it == id_entity_entries.end()) {
                return 0;
            }
            auto& entry = it->second;
            auto child_id = AddEntity(entity, true);
            entry.children.insert(child_id);
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
            return child_ids;
        }

        EntityT* GetEntity(int id) {
            static auto entity_size = sizeof(EntityT);
            auto it = id_entity_entries.find(id);
            if (it == id_entity_entries.end()) {
                return 0;
            }
            auto& entry = it->second;
            auto index = entry.index;
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, buffer_name);
            return (EntityT*)(buffer_ptr.get() + (entity_size * index));
        }

        template<typename RComponentT>
        bool RegisterComponent() {
            auto ID = ComponentTypeID<RComponentT>::id;
            registered_component_map[ID] = {
                .type_id = ComponentTypeID<RComponentT>::id,
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

            auto component_id = GlobalUID::GetNew();
            auto component_type_id = ComponentTypeID<DComponentT>::id;

            auto& component_entry = registered_component_map[component_type_id];
            auto component_type_index = component_entry.constructed_count++;

            auto& ucom = entry.components[component_id];

            auto component_index = constructed_component_count++;
            
            ucom = std::shared_ptr<DComponentT>(new DComponentT{}, [](DComponentT* dp){ delete dp; });
            auto& aucom = *ucom;
            aucom.index = component_index;
            aucom.id = component_id;

            auto& root_data = aucom.GetRootData(*this);

            root_data.index = component_index;
            root_data.type = component_type_id;
            root_data.type_index = component_type_index;
            root_data.data_size = sizeof(typename DComponentT::DataT);

            auto& entity = *entity_ptr;
            auto entity_component_index = entity.componentsCount++;
            entity.components[entity_component_index] = component_index;

            aucom.template GetData<DComponentT>(*this) = original_data;

            return *std::dynamic_pointer_cast<DComponentT>(ucom);
        }

        ComponentT::DataT& GetRootData(int component_index) {
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, "Components");
            return *(typename ComponentT::DataT*)(buffer_ptr.get() + (sizeof(typename ComponentT::DataT) * component_index));
        }

        template<typename AComponentT>
        AComponentT::DataT& GetData(int component_index) {
            return *(typename AComponentT::DataT*)GetDataVoid(component_index);
        }

        void* GetDataVoid(int component_index) {
            auto& root_data = GetRootData(component_index);

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
            std::vector<std::string> restricted_keys{"Entities", "Components"};
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

            if (!framebuffer) {
                auto& width = *window_get_width_ref(window_ptr);
                auto& height = *window_get_height_ref(window_ptr);
                fb_image = image_create({
                    .width = uint32_t(width),
                    .height = uint32_t(height),
                    .is_framebuffer_attachment = true
                });
                Image* fb_images[1] = {
                    fb_image
                };
                AttachmentType fb_attachment_types[1] = {
                    AttachmentType::Color
                };
                FramebufferInfo fb_info{
                    .pImages = fb_images,
                    .imagesCount = 1,
                    .pAttachmentTypes = fb_attachment_types,
                    .attachmentTypesCount = 1,
                    .own_images = true
                };
                framebuffer = framebuffer_create(fb_info);

                shader_set_render_pass(shader, framebuffer);
            }

            window_register_free_callback(window_ptr, [&]() mutable {
                framebuffer_destroy(framebuffer);
            });
        }

        std::string GenerateVertexShader() {
            auto binding_index = 0;
            auto shader_string = R"(
#version 450
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
)" +
EntityT::GetGLSLStruct() +
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

        Image* GetFramebufferImage() {
            return fb_image;
        }

    };
}