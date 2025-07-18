#pragma once

#include "ECS/Provider.hpp"
#include "math.hpp"

namespace dz {
    struct Camera : Provider<Camera> {
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
        inline static std::string GLSLMethods = "";

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {
            {0.5f, R"(
    Camera camera = Cameras.data[pc.camera_index];
)", ShaderModuleType::Vertex},
            {2.0f, R"(
    // Vulkan Y Fix
    final_position.y *= -1.0;
    vec4 camera_position = camera.projection * camera.view * final_position;
    vec3 view_dir = normalize(camera.position - vec3(camera_position));
)", ShaderModuleType::Vertex}
        };
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

}