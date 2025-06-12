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
    int padding;
    void set_model()
    {
        mat<float, 4, 4> final_model(1.0f),
        translate_model(1.0f),
        rotation_model(1.0f),
        scale_model(1.0f);

        translate_model.translate(vec<float, 3>(position));
        for (size_t i = 0; i < 3; ++i)
        {
            vec<float, 3> axis;
            axis[i] = 1;
            rotation_model.rotate<3>(radians(rotation[i]), axis);
        }

        scale_model.scale(vec<float, 3>(scale));

        final_model = translate_model * rotation_model * scale_model;
        model = final_model;
        inverseModel = model.inverse();
    }
};

struct Mesh
{
    int shape_type;
};

struct Material
{
    vec<float, 4> color;
    int type;
    vec<float, 3> padding;
};

struct WindowState {
    int keys[256];
    int buttons[8];
    float frametime;
};

std::string VERSION("#version 450\n");
std::string OUT_LAYOUT(R"(
layout(location = 0) out vec4 outColor;
layout(location = 1) out vec4 outPosition;
)");
std::string STRUCTS_AND_BUFFERS(R"(

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
    vec3 padding;
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
    int padding;
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
)");
std::string STRUCT_METHODS_VERT(R"(
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
)");

std::string WINDOW_STATE_S_A_B(R"(
struct WindowState {
    int keys[256];
    int buttons[8];
    float frametime;
};

layout(std430, binding = 4) buffer WindowStatesBuffer {
    WindowState states[];
} WindowStates;
)");

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
    buffer_group_restrict_to_keys(main_buffer_group, {"Meshes", "Materials", "Entities", "Cameras"});
    auto windows_buffer_group = buffer_group_create("windows_buffer_group");
    buffer_group_restrict_to_keys(windows_buffer_group, {"WindowStates"});

    auto& window_width = window_get_width_ref(main_window);
    auto& window_height = window_get_height_ref(main_window);

    auto stat_window = window_create({
        .title = "Shader Reflect Stats",
        .x = 660,
        .y = 240,
        .width = 640,
        .height = 480,
        .borderless = true,
        .vsync = false
    });

    auto update_entity_shader = shader_create();

    shader_add_buffer_group(update_entity_shader, windows_buffer_group);
    shader_add_buffer_group(update_entity_shader, main_buffer_group);

    shader_add_module(update_entity_shader, ShaderModuleType::Compute, 
VERSION +
"layout(local_size_x = 1) in;\n" +
STRUCTS_AND_BUFFERS +
WINDOW_STATE_S_A_B + R"(

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= Entities.entities.length())
        return;
    float x = Entities.entities[id].position.x;
    int right = WindowStates.states[0].keys[19];
    int left = WindowStates.states[0].keys[20];
    int up = WindowStates.states[0].keys[17];
    int down = WindowStates.states[0].keys[18];
    if (right == 1)
        Entities.entities[id].position.x += (1.0 * WindowStates.states[0].frametime);
    if (left == 1)
        Entities.entities[id].position.x -= (1.0 * WindowStates.states[0].frametime);
    if (up == 1)
        Entities.entities[id].position.y -= (1.0 * WindowStates.states[0].frametime);
    if (down == 1)
        Entities.entities[id].position.y += (1.0 * WindowStates.states[0].frametime);
}
)");

    buffer_group_set_buffer_element_count(windows_buffer_group, "WindowStates", 1);

    auto main_entity_shader = shader_create();
    auto green_entity_shader = shader_create();

    DrawListManager<Entity> entity_draw_list_mg("Entities", [&](auto buffer_group, auto& entity) -> std::pair<Shader*, uint32_t> {
        auto mesh_index = entity.mesh_index;
        auto material_index = entity.material_index;
        auto mesh_view = buffer_group_get_buffer_element_view(buffer_group, "Meshes", mesh_index);
        auto material_view = buffer_group_get_buffer_element_view(buffer_group, "Materials", material_index);
        auto& mesh = mesh_view.template as_struct<Mesh>();
        auto& material = material_view.template as_struct<Material>();
        Shader* chosen_shader = 0;
        uint32_t vert_count = 0;

        // example of choosing different shaders based on material.type
        if (material.type == 0)
        {
            chosen_shader = main_entity_shader;
        }
        else
        {
            chosen_shader = green_entity_shader;
        }
        switch (mesh.shape_type)
        {
        case 0:
        default:
            vert_count = 6;
            break;
        }
        return {chosen_shader, vert_count}; // return 6 indices for a plane for now, todo get size based on mesh.shape_type
    });

    window_add_drawn_buffer_group(main_window, &entity_draw_list_mg, main_buffer_group);
    window_add_drawn_buffer_group(stat_window, &entity_draw_list_mg, main_buffer_group);

    shader_add_buffer_group(main_entity_shader, main_buffer_group);
    shader_add_buffer_group(green_entity_shader, main_buffer_group);
    shader_add_buffer_group(main_entity_shader, windows_buffer_group);

    shader_set_define(main_entity_shader, "SUPER_MIP", "3.1415");
    shader_set_define(main_entity_shader, "MAEGA_MIP", "5.7832");

    shader_add_module(main_entity_shader, ShaderModuleType::Vertex, 
VERSION + 
OUT_LAYOUT +
STRUCTS_AND_BUFFERS +
STRUCT_METHODS_VERT +
WINDOW_STATE_S_A_B + R"(
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
    if (WindowStates.states[0].buttons[0] == 1)
        outColor = vec4(1, 1, 0, 1);
    else
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

    // green_entity_shader

    shader_add_module(green_entity_shader, ShaderModuleType::Vertex, 
VERSION + 
OUT_LAYOUT +
STRUCTS_AND_BUFFERS +
STRUCT_METHODS_VERT + R"(
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
    outColor = vec4(0, 1, 0, 1);
    gl_Position = outPosition;
}
)");
    shader_add_module(green_entity_shader, ShaderModuleType::Fragment, R"(
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

    std::vector<Material*> material_ptrs;
    material_ptrs.resize(4);
    buffer_group_set_buffer_element_count(main_buffer_group, "Materials", material_ptrs.size());
    buffer_group_set_buffer_element_count(main_buffer_group, "Meshes", 1);
    std::vector<Entity*> entity_ptrs;
    entity_ptrs.resize(5);
    buffer_group_set_buffer_element_count(main_buffer_group, "Entities", entity_ptrs.size());
    buffer_group_set_buffer_element_count(main_buffer_group, "Cameras", 1);
    
    buffer_group_initialize(main_buffer_group);
    buffer_group_initialize(windows_buffer_group);

    auto window_1_view = buffer_group_get_buffer_element_view(windows_buffer_group, "WindowStates", 0);
    auto& window_1 = window_1_view.template as_struct<WindowState>();
    window_set_keys_pointer(main_window, window_1.keys);
    window_set_buttons_pointer(main_window, window_1.buttons);
    window_set_frametime_pointer(main_window, &window_1.frametime);

    //

    auto mesh_1_view = buffer_group_get_buffer_element_view(main_buffer_group, "Meshes", 0);

    mesh_1_view.set_member("shape_type", 0);

    for (size_t i = 0; i < material_ptrs.size(); ++i)
    {
        auto material_view = buffer_group_get_buffer_element_view(main_buffer_group, "Materials", i);
        auto& material = material_view.as_struct<Material>();
        material_ptrs[i] = &material;
    }

    for (size_t i = 0; i < entity_ptrs.size(); ++i)
    {
        auto entity_view = buffer_group_get_buffer_element_view(main_buffer_group, "Entities", i);
        auto& entity = entity_view.as_struct<Entity>();
        entity_ptrs[i] = &entity;
    }

    int q = 1;
    for (auto& mp : material_ptrs)
    {
        auto& material = *mp;
        material.type = q ? 0 : 1;
        q = material.type;
        material.color = {
            Random::value<float>(0.3, 1),
            Random::value<float>(0.3, 1),
            Random::value<float>(0.3, 1),
            1
        };
    }

    long c = -2;
    for (auto& ep : entity_ptrs)
    {
        auto& entity = *ep;
        entity.mesh_index = 0;
        entity.material_index = Random::value<int32_t>(0, 3);
        entity.camera_index = 0;
        entity.scale = {1, 1, 1, 1};
        entity.position[1] = (c++ * 1.5);
    }

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
        if (main_window_polling)
        {
            main_window_polling = window_poll_events(main_window);
            if (main_window_polling)
            {
                if (esc_pressed)
                    break;
        
                shader_dispatch(update_entity_shader, {entity_ptrs.size(), 1, 1});

                for (auto& ep : entity_ptrs)
                {
                    auto& entity = *ep;
                    entity.set_model();
                }
        
                window_render(main_window);
            }
        }
        if (stat_window_polling)
        {
            stat_window_polling = window_poll_events(stat_window);
            if (stat_window_polling)
                window_render(stat_window);
        }
    }

    window_free(stat_window);
    window_free(main_window);
    return 0;
}