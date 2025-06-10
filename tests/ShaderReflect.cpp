#include <DirectZ.hpp>
using namespace dz;

struct Entity
{
    vec<float, 4> position;
    vec<float, 4> scale;
    vec<float, 4> rotation;
    mat<float, 4, 4> model;
    mat<float, 4, 4> inverseModel;
    int mesh_index;
    int material_index;
    int camera_index;
};

int main()
{
    auto main_window = window_create({
        .title = "Shader Reflect Test",
        .x = 0,
        .y = 240,
        .width = 640,
        .height = 480,
        .borderless = true,
        .vsync = false
    });

    auto main_buffer_group = buffer_group_create("main_buffer_group");

    auto& window_width = window_get_width_ref(main_window);
    auto& window_height = window_get_height_ref(main_window);

    auto stat_window = window_create({
        .title = "Shader Reflect Stats",
        .x = 640,
        .y = 240,
        .width = 240,
        .height = 480,
        .borderless = true,
        .vsync = false
    });

    // window_use_other_registry(stat_window, main_window);

//     auto update_entity_shader = shader_create(main_window);

//     shader_add_module(update_entity_shader, ShaderModuleType::Compute, R"(
// #version 450

// void main() {
// }
// )");

    auto main_entity_shader = shader_create(main_window);

    shader_set_buffer_group(main_entity_shader, main_buffer_group);

    shader_set_define(main_entity_shader, "SUPER_MIP", "3.1415");
    shader_set_define(main_entity_shader, "MAEGA_MIP", "5.7832");

    shader_add_module(main_entity_shader, ShaderModuleType::Vertex, R"(
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
    vec4 position;
    vec4 scale;
    vec4 rotation;
    mat4 model;
    mat4 inverseModel;
    int mesh_index;
    int material_index;
    int camera_index;
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

Camera get_camera(in Entity entity)
{
    return Cameras.cameras[entity.camera_index];
}

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
    Camera camera = get_camera(entity);
    vec3 inPosition = get_entity_vertex(gl_VertexIndex, entity);
    // mat4 mpv = camera.projection * camera.view * entity.model;
    vec4 worldPos = entity.model * vec4(inPosition, 1.0);
    vec4 viewPos  = camera.view * worldPos;
    vec4 clipPos  = camera.projection * viewPos;
    outPosition = clipPos;
    outColor = material.color;
    gl_Position = outPosition;
}
)");
    shader_add_module(main_entity_shader, ShaderModuleType::Fragment, R"(
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

    buffer_group_set_buffer_element_count(main_buffer_group, "Materials", 1);
    buffer_group_set_buffer_element_count(main_buffer_group, "Meshes", 1);
    buffer_group_set_buffer_element_count(main_buffer_group, "Entities", 5);
    buffer_group_set_buffer_element_count(main_buffer_group, "Cameras", 1);
    
    // 

    shader_initialize(main_entity_shader);

    //

    auto material_1_view = buffer_group_get_buffer_element_view(main_buffer_group, "Materials", 0);

    material_1_view.set_member("color", vec<float, 4>(0, 0, 1.0, 1.0));
    material_1_view.set_member("type", 0);

    auto mesh_1_view = buffer_group_get_buffer_element_view(main_buffer_group, "Meshes", 0);

    mesh_1_view.set_member("shape_type", 0);

    auto entity_1_view = buffer_group_get_buffer_element_view(main_buffer_group, "Entities", 0);
    auto& entity_1 = entity_1_view.as_struct<Entity>();

    entity_1.mesh_index = 0;
    entity_1.material_index = 0;
    entity_1.camera_index = 0;

    auto camera_1 = buffer_group_get_buffer_element_view(main_buffer_group, "Cameras", 0);

    auto& camera_1_view = camera_1.get_member<mat<float, 4, 4>>("view");
    auto& camera_1_projection = camera_1.get_member<mat<float, 4, 4>>("projection");
    auto& camera_1_inverse_view = camera_1.get_member<mat<float, 4, 4>>("inverse_view");
    auto& camera_1_inverse_projection = camera_1.get_member<mat<float, 4, 4>>("inverse_projection");

    camera_1_view = lookAt<float>({0, 0, 5}, {0, 0, 0}, {0, 1, 0});
    camera_1_projection = perspective<float>(radians(81.f), window_width / window_height, 0.01, 100.f);
    // camera_1_projection = orthographic<float>(-2, 2, -2, 2, 0.01, 100.f);
    camera_1_inverse_view = camera_1_view.inverse();
    camera_1_inverse_projection = camera_1_projection.inverse();

    //

    auto& entity_1_position = entity_1.position;
    auto& entity_1_scale = entity_1.scale;
    auto& entity_1_rotation = entity_1.rotation;

    entity_1_scale = {1, 1, 1, 1};

    auto& entity_1_model = entity_1.model;
    auto& entity_1_inverseModel = entity_1.inverseModel;

    entity_1_model = mat<float, 4, 4>(1.0f);

    vec<float, 4> th_vec(1.00, 0, 0, 0);
    vec<float, 4> tv_vec(0, -1.00, 0, 0);
    float scale_fac = 0.0008;
    vec<float, 4> s_vec(1, 1, 1, 1);

    auto& window_frametime = window_get_frametime_ref(main_window);

    bool entity_right = true;

    auto& right_pressed = window_get_keypress_ref(main_window, KEYCODE_RIGHT);
    auto& left_pressed = window_get_keypress_ref(main_window, KEYCODE_LEFT);
    auto& up_pressed = window_get_keypress_ref(main_window, KEYCODE_UP);
    auto& down_pressed = window_get_keypress_ref(main_window, KEYCODE_DOWN);
    auto& esc_pressed = window_get_keypress_ref(main_window, KEYCODE_ESCAPE);
    auto& u_pressed = window_get_keypress_ref(main_window, 'u');
    auto& o_pressed = window_get_keypress_ref(main_window, 'o');
    auto& r_pressed = window_get_keypress_ref(main_window, 'r');
    auto& f_pressed = window_get_keypress_ref(main_window, 'f');
    auto& v_pressed = window_get_keypress_ref(main_window, 'v');

    bool main_window_polling = true;
    bool stat_window_polling = true;

    while (main_window_polling)
    {
        if (stat_window_polling)
        {
            stat_window_polling = window_poll_events(stat_window);
            if (stat_window_polling)
                window_render(stat_window);
        }
        if (main_window_polling)
        {
            main_window_polling = window_poll_events(main_window);
            if (main_window_polling)
            {
                if (esc_pressed)
                    break;
        
                if ((entity_right || right_pressed) && !left_pressed)
                    entity_1_position += (th_vec * window_frametime);
                else 
                    entity_1_position += (-th_vec * window_frametime);
        
                if (up_pressed)
                    entity_1_position += (tv_vec * window_frametime);
                if (down_pressed)
                    entity_1_position += (-tv_vec * window_frametime);
        
                if (u_pressed)
                    entity_1_scale += (s_vec + scale_fac);
                if (o_pressed)
                    entity_1_scale += (s_vec - scale_fac);
                if (r_pressed)
                    entity_1_rotation[2] += (90.f * window_frametime);
                if (f_pressed)
                    entity_1_rotation[1] += (90.f * window_frametime);
                if (v_pressed)
                    entity_1_rotation[0] += (90.f * window_frametime);
        
                if (entity_1_position[0] > 5.0)
                {
                    entity_right = !entity_right;
                    entity_1_position[0] = 5.0;
                }
                else if(entity_1_position[0] < -5.0)
                {
                    entity_right = !entity_right;
                    entity_1_position[0] = -5.0;
                }
                if (entity_1_position[1] > 5.0)
                {
                    entity_1_position[1] = 5.0;
                }
                else if (entity_1_position[1] < -5.0)
                {
                    entity_1_position[1] = -5.0;
                }
        
                mat<float, 4, 4> final_model(1.0f),
                translate_model(1.0f),
                rotation_model(1.0f),
                scale_model(1.0f);
        
                translate_model.translate(vec<float, 3>(entity_1_position));
                for (size_t i = 0; i < 3; ++i)
                {
                    vec<float, 3> axis;
                    axis[i] = 1;
                    rotation_model.rotate<3>(radians(entity_1_rotation[i]), axis);
                }
        
                scale_model.scale(vec<float, 3>(entity_1_scale));
        
                final_model = translate_model * rotation_model * scale_model;
                entity_1_model = final_model;
                entity_1_inverseModel = entity_1_model.inverse();
        
                window_render(main_window);
            }
        }
    }

    window_free(stat_window);
    window_free(main_window);
    return 0;
}