#pragma once

#include "Provider.hpp"
#include "../Framebuffer.hpp"
#include "../Shader.hpp"
#include "../math.hpp"

namespace dz::ecs {
    struct Camera : Provider<Camera> {
        inline static std::string CamerasBufferName = "Cameras";
        enum ProjectionType : int32_t {
            Perspective,
            Orthographic
        };
        mat<float, 4, 4> view;
        mat<float, 4, 4> projection;
        float nearPlane;
        float farPlane;
        int type;
        float aspect = 0;
        vec<float, 3> position;
        float fov = 80.f;
        vec<float, 3> center;
        float orthoWidth;
        vec<float, 3> up;
        float orthoHeight;
        inline static constexpr bool IsCameraProvider = true;
        inline static constexpr size_t PID = 3;
        inline static float Priority = 0.5f;
        inline static std::string ProviderName = "Camera";
        inline static std::string StructName = "Camera";
        inline static std::string GLSLStruct = R"(
struct Camera {
    mat4 view;
    mat4 projection;
    float nearPlane;
    float farPlane;
    int type;
    float aspect;
    vec3 position;
    float fov;
    vec3 center;
    float orthoWidth;
    vec3 up;
    float orthoHeight;
};
)";

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
    Camera camera = Cameras.data[pc.camera_index];
)", ShaderModuleType::Vertex},
            {0.5f, R"(
    Camera camera = Cameras.data[pc.camera_index];
)", ShaderModuleType::Fragment},
            {2.0f, R"(
    vec4 view_position = camera.view * entity.model * vertex_position;
    vec4 clip_position = camera.projection * view_position;
)", ShaderModuleType::Vertex}
        };
        
        struct CameraTypeReflectable : Reflectable {
            
        private:
            std::function<Camera*()> get_camera_function;
            std::function<void()> reset_reflectables_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"type", {0, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "type"}
            };
            inline static std::vector<std::string> prop_names = {
                "type"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(Camera::ProjectionType)
            };

        public:
            CameraTypeReflectable(
                const std::function<Camera*()>& get_camera_function,
                const std::function<void()>& reset_reflectables_function
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
            
        struct CameraViewReflectable : Reflectable {
            
        private:
            std::function<Camera*()> get_camera_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"position", {0, 0}},
                {"center", {1, 0}},
                {"up", {2, 0}},
                {"nearPlane", {3, 0}},
                {"farPlane", {4, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "posiiton"},
                {1, "center"},
                {2, "up"},
                {3, "nearPlane"},
                {4, "farPlane"},
            };
            inline static std::vector<std::string> prop_names = {
                "position",
                "center",
                "up",
                "nearPlane",
                "farPlane"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(vec<float, 3>),
                &typeid(vec<float, 3>),
                &typeid(vec<float, 3>),
                &typeid(float),
                &typeid(float)
            };

        public:
            CameraViewReflectable(const std::function<Camera*()>& get_camera_function);
            int GetID() override;
            std::string& GetName() override;
            DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
            DEF_GET_PROPERTY_NAMES(prop_names);
            void* GetVoidPropertyByIndex(int prop_index) override;
            DEF_GET_VOID_PROPERTY_BY_NAME;
            DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
            void NotifyChange(int prop_index) override;
        };
            
        struct CameraPerspectiveReflectable : Reflectable {
            
        private:
            std::function<Camera*()> get_camera_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"aspect", {0, 0}},
                {"fov", {1, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "aspect"},
                {1, "fov"}
            };
            inline static std::vector<std::string> prop_names = {
                "aspect",
                "fov"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(float),
                &typeid(float)
            };

        public:
            CameraPerspectiveReflectable(const std::function<Camera*()>& get_camera_function);
            int GetID() override;
            std::string& GetName() override;
            DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
            DEF_GET_PROPERTY_NAMES(prop_names);
            void* GetVoidPropertyByIndex(int prop_index) override;
            DEF_GET_VOID_PROPERTY_BY_NAME;
            DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
            void NotifyChange(int prop_index) override;
        };
            
        struct CameraOrthographicReflectable : Reflectable {
            
        private:
            std::function<Camera*()> get_camera_function;
            int uid;
            std::string name;
            inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
                {"orthoWidth", {0, 0}},
                {"orthoHeight", {1, 0}}
            };
            inline static std::unordered_map<int, std::string> prop_index_names = {
                {0, "orthoWidth"},
                {1, "orthoHeight"}
            };
            inline static std::vector<std::string> prop_names = {
                "orthoWidth",
                "orthoHeight"
            };
            inline static const std::vector<const std::type_info*> typeinfos = {
                &typeid(float),
                &typeid(float)
            };

        public:
            CameraOrthographicReflectable(const std::function<Camera*()>& get_camera_function);
            int GetID() override;
            std::string& GetName() override;
            DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
            DEF_GET_PROPERTY_NAMES(prop_names);
            void* GetVoidPropertyByIndex(int prop_index) override;
            DEF_GET_VOID_PROPERTY_BY_NAME;
            DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
            void NotifyChange(int prop_index) override;
        };

        struct CameraReflectableGroup : ReflectableGroup {
            BufferGroup* buffer_group = nullptr;
            std::string name;
            std::string imgui_name;
            std::vector<Reflectable*> reflectables;

            Framebuffer* framebuffer = 0;
            Image* fb_color_image = 0;
            Image* fb_depth_image = 0;
            VkDescriptorSet frame_image_ds = VK_NULL_HANDLE;
            bool open_in_editor = true;

            CameraReflectableGroup(BufferGroup* buffer_group):
                buffer_group(buffer_group),
                name("Camera")
            {}

            CameraReflectableGroup(BufferGroup* buffer_group, Serial& serial):
                buffer_group(buffer_group)
            {
                restore(serial);
            }

            ~CameraReflectableGroup() {
                ClearReflectables();
            }
            GroupType GetGroupType() override {
                return ReflectableGroup::Entity;
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
            void ClearReflectables() {
                for (auto& reflectable : reflectables)
                    delete reflectable;
                reflectables.clear();
            }
            void UpdateChildren() override {
                if (reflectables.size() == 0) {
                    reflectables.push_back(new CameraTypeReflectable([&]() mutable {
                        auto camera_buff = buffer_group_get_buffer_data_ptr(buffer_group, CamerasBufferName);
                        auto cameras_ptr = (struct Camera*)(camera_buff.get());
                        return &cameras_ptr[index];
                    }, [&]() mutable {
                        UpdateChildren();
                    }));
                    reflectables.push_back(new CameraViewReflectable([&]() mutable {
                        auto camera_buff = buffer_group_get_buffer_data_ptr(buffer_group, CamerasBufferName);
                        auto cameras_ptr = (struct Camera*)(camera_buff.get());
                        return &cameras_ptr[index];
                    }));
                }
                auto camera_buff = buffer_group_get_buffer_data_ptr(buffer_group, CamerasBufferName);
                auto cameras_ptr = (struct Camera*)(camera_buff.get());
                auto& camera = cameras_ptr[index];
                // clear type reflectables
                for (size_t index = 2; index < reflectables.size(); index++) {
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
                    reflectables.push_back(new CameraPerspectiveReflectable([&]() mutable {
                        auto camera_buff = buffer_group_get_buffer_data_ptr(buffer_group, CamerasBufferName);
                        auto cameras_ptr = (struct Camera*)(camera_buff.get());
                        return &cameras_ptr[index];
                    }));
                    break;
                case Camera::Orthographic:
                    reflectables.push_back(new CameraOrthographicReflectable([&]() mutable {
                        auto camera_buff = buffer_group_get_buffer_data_ptr(buffer_group, CamerasBufferName);
                        auto cameras_ptr = (struct Camera*)(camera_buff.get());
                        return &cameras_ptr[index];
                    }));
                    break;
                default: break;
                }
            }
            void InitFramebuffer(Shader* shader, float width, float height) {
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

                shader_set_render_pass(shader, framebuffer);
            }
            bool backup(Serial& serial) const override {
                if (!backup_internal(serial))
                    return false;
                serial << name << imgui_name;
                return true;
            }
            bool restore(Serial& serial) override {
                if (!restore_internal(serial))
                    return false;
                serial >> name >> imgui_name;
                return true;
            }
        };

        using ReflectableGroup = CameraReflectableGroup;
    };

    void CameraInit(
        Camera& camera, 
        vec<float, 3> position,
        vec<float, 3> center,
        vec<float, 3> up,
        float nearPlane,
        float farPlane,
        float width,
        float height,
        float fov,
        Camera::ProjectionType projectionType = Camera::Perspective
    );

    void CameraInit(
        Camera& camera, 
        vec<float, 3> position,
        vec<float, 3> center,
        vec<float, 3> up,
        float nearPlane,
        float farPlane,
        vec<float, 4> viewport,
        Camera::ProjectionType projectionType = Camera::Orthographic
    );

    void CameraInit(Camera& camera);

}