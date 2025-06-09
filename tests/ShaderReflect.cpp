#include <DirectZ.hpp>
using namespace dz;

int main()
{
    auto window = window_create({
        .title = "Shader Reflect Test",
        .x = -1,
        .y = -1,
        .width = 640,
        .height = 480,
        .borderless = true,
        .vsync = false
    });
    auto shader = shader_create(window);
    shader_add_module(shader, ShaderModuleType::Vertex, R"(
#version 450

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;

struct Mesh
{
    int shape_type;
};
layout(std430, binding = 0) buffer MeshBuffer
{
    Mesh meshes[];
} Meshes;

struct Material
{
    vec4 color;
    int type;
};
layout(std430, binding = 1) buffer MaterialBuffer
{
    Material materials[];
} Materials;

struct Entity
{
    mat4 model;
    mat4 inverseModel;
    int mesh_index;
    int material_index;
};

layout(std430, binding = 2) buffer EntitiesBuffer
{
    Entity entities[];
} Entities;

struct Camera
{
    mat4 view;
    mat4 projection;
    mat4 inverse_view;
    mat4 inverse_projection;
};

layout(std430, binding = 3) buffer CamerasBuffer
{
    Camera cameras[];
} Cameras;

int get_entity_id()
{
    return gl_InstanceIndex;
}

Entity get_entity(int entity_id)
{
    if (entity_id >= 0) {
        return Entities.entities[entity_id];
    }
    Entity defaultEntity;
    defaultEntity.model = mat4(1);
    defaultEntity.inverseModel = inverse(defaultEntity.model);
    return defaultEntity;
}

Material get_material(in Entity entity)
{
    int material_index = entity.material_index;
    return Materials.materials[material_index];
}

Mesh get_mesh(in Entity entity)
{
    int mesh_index = entity.mesh_index;
    return Meshes.meshes[mesh_index];
}

vec3 get_entity_vertex(int vertex_index, in Entity entity)
{
    switch (vertex_index)
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

void main()
{
    int entity_id = get_entity_id();
    Entity entity = get_entity(entity_id);
    Material material = get_material(entity);
    Mesh mesh = get_mesh(entity);
    vec4 inPosition = vec4(get_entity_vertex(gl_VertexIndex, entity), 1);
    outPosition = inPosition * entity.model;
    outColor = material.color;
    gl_Position = outPosition;
}
)");
    shader_add_module(shader, ShaderModuleType::Fragment, R"(
#version 450

layout(location = 0) in vec4 inColor;
layout(location = 1) in vec4 inPosition;

layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = inColor;
}
)");


    // Instantiate the shader buffers with some counts

    shader_set_buffer_element_count(shader, "Materials", 1);
    shader_set_buffer_element_count(shader, "Meshes", 1);
    shader_set_buffer_element_count(shader, "Entities", 5);
    shader_set_buffer_element_count(shader, "Cameras", 1);
    
    // 

    shader_create_resources(shader);
    shader_compile(shader);
    shader_update_descriptor_sets(shader);

    // Get views into the shader buffers at index 0

    auto material_1_view = shader_get_buffer_element_view(shader, "Materials", 0);
    auto mesh_1_view = shader_get_buffer_element_view(shader, "Meshes", 0);
    auto entity_1_view = shader_get_buffer_element_view(shader, "Entities", 0);
    auto camera_1_view = shader_get_buffer_element_view(shader, "Cameras", 0);

    camera_1_view.get_member<mat<float, 4, 4>>("view");
    camera_1_view.get_member<mat<float, 4, 4>>("projection");
    camera_1_view.get_member<mat<float, 4, 4>>("inverse_view");
    camera_1_view.get_member<mat<float, 4, 4>>("inverse_projection");

    //

    material_1_view.set_member("color", vec<float, 4>(0, 0, 1.0, 1.0));
    material_1_view.set_member("type", 0);

    mesh_1_view.set_member("shape_type", 0);

    entity_1_view.set_member("mesh_index", 0);
    entity_1_view.set_member("material_index", 0);

    auto& entity_1_model = entity_1_view.get_member<mat<float, 4, 4>>("model");
    auto& entity_1_inverseModel = entity_1_view.get_member<mat<float, 4, 4>>("inverseModel");

    entity_1_model = mat<float, 4, 4>(1.0f);

    vec<float, 3> th_vec(1.00, 0, 0);
    vec<float, 3> tv_vec(0, -1.00, 0);
    vec<float, 3> s_vec(1.01, 1.01, 1);

    auto& window_frametime = window_get_frametime_ref(window);

    bool entity_right = true;

    auto& right_pressed = window_get_keypress_ref(window, KEYCODE_RIGHT);
    auto& left_pressed = window_get_keypress_ref(window, KEYCODE_LEFT);
    auto& up_pressed = window_get_keypress_ref(window, KEYCODE_UP);
    auto& down_pressed = window_get_keypress_ref(window, KEYCODE_DOWN);
    auto& esc_pressed = window_get_keypress_ref(window, KEYCODE_ESCAPE);

    while (window_poll_events(window))
    {
        if (esc_pressed)
            break;

        if ((entity_right || right_pressed) && !left_pressed)
            entity_1_model.translate(th_vec * window_frametime);
        else 
            entity_1_model.translate(-th_vec * window_frametime);

        if (up_pressed)
            entity_1_model.translate(tv_vec * window_frametime);
        if (down_pressed)
            entity_1_model.translate(-tv_vec * window_frametime);

        vec<float, 3> pos(entity_1_model[0][3], entity_1_model[1][3], entity_1_model[2][3]);
        if (pos[0] > 0.5)
        {
            entity_right = !entity_right;
            entity_1_model[0][3] = 0.5;
        }
        else if(pos[0] < -0.5)
        {
            entity_right = !entity_right;
            entity_1_model[0][3] = -0.5;
        }
        if (pos[1] > 0.5)
        {
            entity_1_model[1][3] = 0.5;
        }
        else if (pos[1] < -0.5)
        {
            entity_1_model[1][3] = -0.5;
        }

        entity_1_inverseModel = entity_1_model.inverse();

        window_render(window);
    }

    window_free(window);
    return 0;
}