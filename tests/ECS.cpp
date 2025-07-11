#include <DirectZ.hpp>

#include <typeinfo>

Shader* default_entity_shader = 0;
uint32_t default_vertex_count = 6;

struct Entity;
struct Component;
struct System;
#define ExampleECS ECS<Entity, Component, System>

struct Entity {
    int id;
    int componentsCount;
    int components[ECS_MAX_COMPONENTS];
    
    inline static std::string GetGLSLStruct() {
        return R"(
struct Entity {
    int id;
    int componentsCount;
    int components[ECS_MAX_COMPONENTS];
};
)";
    }

    inline static std::string GetGLSLEntityVertexFunction() {
        return R"(
vec3 GetEntityVertex(in Entity entity) {
    switch (gl_VertexIndex)
    {
    case 0: return vec3(0.5, 0.5, 0);
    case 1: return vec3(-0.5, 0.5, 0);
    case 2: return vec3(-0.5, -0.5, 0);
    case 3: return vec3(-0.5, -0.5, 0);
    case 4: return vec3(0.5, -0.5, 0);
    case 5: return vec3(0.5, 0.5, 0);
    }
    return vec3(0);
}
)";
    }

    inline static std::string GetGLSLEntityVertexColorFunction() {
        return R"(
vec4 GetEntityVertexColor(in Entity entity) {
    return vec4(0, 0, 1, 0.8);
}
)";
    }

    Shader* GetShader() {
        return default_entity_shader;
    }

    uint32_t GetVertexCount() {
        return default_vertex_count;
    }
};

struct Component {
    struct ComponentData {
        int id;
        int type;
        int type_index;
        int placeholder;
    };
    using DataT = ComponentData;
    int index = -1;
    
    inline static std::string GetGLSLStruct() {
        return R"(
struct Component {
    int id;
    int type;
    int type_index;
    int placeholder;
};
)";
    }

    DataT& GetRootComponentData(ExampleECS& ecs) {
        return ecs.GetRootComponentData(index);
    }
    
    template<typename AComponentT>
    AComponentT::DataT& GetComponentData(ExampleECS& ecs) {
        return ecs.GetComponentData<AComponentT>(index);
    }

    virtual int GetPropertyIndexByName(const std::string& prop_name) = 0;
    virtual const std::vector<std::string>& GetPropertyNames() = 0;
    virtual void* GetVoidPropertyByIndex(ExampleECS& ecs, int prop_index) = 0;
    virtual void* GetVoidPropertyByName(ExampleECS& ecs, const std::string& prop_name) = 0;
    virtual const std::vector<const std::type_info*>& GetPropertyTypeinfos() = 0;
    template <typename T>
    T& GetPropertyByIndex(ExampleECS& ecs, int prop_index) {
        return *(T*)GetVoidPropertyByIndex(ecs, prop_index);
    }
    template <typename T>
    T& GetPropertyByName(ExampleECS& ecs, const std::string& prop_name) {
        return *(T*)GetVoidPropertyByName(ecs, prop_name);
    }
    virtual ~Component() = default;
};

struct PositionComponent : Component {
    using DataT = vec<float, 4>;
    int GetPropertyIndexByName(const std::string& prop_name) override {
        static std::unordered_map<std::string, int> prop_name_indexes = {
            {"x", 0},
            {"y", 1},
            {"z", 2},
            {"t", 3}
        };
        auto it = prop_name_indexes.find(prop_name);
        if (it == prop_name_indexes.end()) {
            return -1;
        }
        return it->second;
    }
    const std::vector<std::string>& GetPropertyNames() override {
        static std::vector<std::string> prop_names = {
            "x",
            "y",
            "z",
            "t"
        };
        return prop_names;
    }
    void* GetVoidPropertyByIndex(ExampleECS& ecs, int prop_index) override {
        auto& data = GetComponentData<PositionComponent>(ecs);
        switch (prop_index) {
        case 0:
            return &data[0];
        case 1:
            return &data[1];
        case 2:
            return &data[2];
        case 3:
            return &data[3];
        }
        return 0;
    }
    void* GetVoidPropertyByName(ExampleECS& ecs, const std::string& prop_name) override {
        auto prop_index = GetPropertyIndexByName(prop_name);
        if (prop_index == -1) {
            return 0;
        }
        return GetVoidPropertyByIndex(ecs, prop_index);
    }
    const std::vector<const std::type_info*>& GetPropertyTypeinfos() override {
        static const std::vector<const std::type_info*> typeinfos = {
            &typeid(float),
            &typeid(float),
            &typeid(float),
            &typeid(float)
        };
        return typeinfos;
    }
};

DEF_COMPONENT_ID(PositionComponent, 1);
DEF_COMPONENT_STRUCT_NAME(PositionComponent, "PositionComponent");
DEF_COMPONENT_STRUCT(PositionComponent, R"(
struct PositionComponent {
    float x;
    float y;
    float z;
    float t;
};
)");

// struct RotationComponent : Component {
//     float yaw;
//     float pitch;
//     float roll;
    
// };

struct System {
    int id;
    virtual ~System() = default;
};

float ORIGINAL_WINDOW_WIDTH = 1280.f;
float ORIGINAL_WINDOW_HEIGHT = 768.f;

struct Frame {
    int width;
    int height;
};

std::string GenerateFrameVertexShader();
std::string GenerateFrameFragmentShader();

int main() {
    auto window = window_create({
        .title = "ECS Test",
        .x = 0,
        .y = 240,
        .width = ORIGINAL_WINDOW_WIDTH,
        .height = ORIGINAL_WINDOW_HEIGHT,
        .borderless = false,
        .vsync = false
    });
    
    ExampleECS ecs([](auto& ecs){
        assert(ecs.template RegisterComponent<PositionComponent>());
        return true;
    });
    
    ecs.EnableDrawInWindow(window);

    auto eids = ecs.AddEntities(Entity{0}, Entity{0});

    auto e1_ptr = ecs.GetEntity(eids[0]);
    assert(e1_ptr);
    auto& e1 = *e1_ptr;
    auto& e1_position_component = ecs.ConstructComponent<PositionComponent>(e1.id, {1.f, 1.f, 1.f, 1.f});

    const auto& position_component_typeinfos = e1_position_component.GetPropertyTypeinfos();
    const auto& position_component_prop_names = e1_position_component.GetPropertyNames();
    for (auto& prop_name : position_component_prop_names) {
        auto prop_index = e1_position_component.GetPropertyIndexByName(prop_name);
        auto type_info = position_component_typeinfos[prop_index];
        if (*type_info == typeid(float)) {
            auto& value = e1_position_component.GetPropertyByIndex<float>(ecs, prop_index);
            std::cout << "Property " << prop_name << "<float>(" << value << ")" << std::endl;
        } else if (*type_info == typeid(int)) {
            auto& value = e1_position_component.GetPropertyByIndex<int>(ecs, prop_index);
            std::cout << "Property " << prop_name << "<int>(" << value << ")" << std::endl;
        } else if (*type_info == typeid(std::string)) {
            auto& value = e1_position_component.GetPropertyByIndex<std::string>(ecs, prop_index);
            std::cout << "Property " << prop_name << "<std::string>(" << value << ")" << std::endl;
        } else {
            // Handle unknown type
            std::cout << "Property " << prop_name << " has unknown type" << std::endl;
        }
    }

    auto e2_ptr = ecs.GetEntity(eids[1]);
    assert(e2_ptr);
    auto& e2 = *e2_ptr;
    auto& e2_position_component = ecs.ConstructComponent<PositionComponent>(e2.id, {2.f, 2.f, 2.f, 1.f});

    auto frame_image_buffer_group = buffer_group_create("FramesGroup");
    buffer_group_restrict_to_keys(frame_image_buffer_group, {"Frames", "frame_image"});

    auto frame_shader = shader_create();

    shader_add_buffer_group(frame_shader, frame_image_buffer_group);

    auto frame_image = ecs.GetFramebufferImage();

    shader_use_image(frame_shader, "frame_image", frame_image);

    shader_add_module(frame_shader, ShaderModuleType::Vertex, GenerateFrameVertexShader());
    shader_add_module(frame_shader, ShaderModuleType::Fragment, GenerateFrameFragmentShader());

    buffer_group_set_buffer_element_count(frame_image_buffer_group, "Frames", 1);

    buffer_group_initialize(frame_image_buffer_group);

    DrawListManager<Frame> frame_draw_list_mg("Frames", [&](auto buffer_group, auto& frame) -> DrawTuple {
        return {0, frame_shader, 6};
    });
    
    window_add_drawn_buffer_group(window, &frame_draw_list_mg, frame_image_buffer_group);

    while (window_poll_events(window)) {
        window_render(window);
    }
}

std::string GenerateFrameVertexShader() {
    std::string shader_string("#version 450\n");
    shader_string += R"(
layout(location = 0) out vec2 outUV;
const vec3 positions[6] = vec3[6](
    vec3(1.0, 1.0, 0),
    vec3(-1.0, 1.0, 0),
    vec3(-1.0, -1.0, 0),
    vec3(-1.0, -1.0, 0),
    vec3(1.0, -1.0, 0),
    vec3(1.0, 1.0, 0)
);
const vec2 uvs[6] = vec2[6](
    vec2(1, 1),
    vec2(0, 1),
    vec2(0, 0),
    vec2(0, 0),
    vec2(1, 0),
    vec2(1, 1)
);
struct Frame {
    int width;
    int height;
};
layout(std430, binding = 0) buffer FramesBuffer {
    Frame frames[];
} Frames;
void main() {
    vec4 position = vec4(positions[gl_VertexIndex], 1.0);
    vec2 uv = uvs[gl_VertexIndex];
    gl_Position = position;
    outUV = uv;
}
)";
    return shader_string;
}

std::string GenerateFrameFragmentShader() {
    std::string shader_string("#version 450\n");
    shader_string += R"(
layout(location = 0) in vec2 inUV;
layout(location = 0) out vec4 FragColor;
layout(binding = 1) uniform sampler2D frame_image;
void main() {
    FragColor = texture(frame_image, inUV);
}
)";
    return shader_string;
}