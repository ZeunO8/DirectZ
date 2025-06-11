#include <DirectZ.hpp>
using namespace dz;

struct Particle
{
    vec<float, 4> position;
    vec<float, 4> velocity;
    vec<float, 4> color;
    float lifetime;
    float age;
    float mass;
    float padding;

    void reset(AABB<float, 2> aabb)
    {
        position = {Random::value<float>(aabb.min[0], aabb.max[0]), Random::value<float>(aabb.min[1], aabb.max[1]), 0.0f, 1.0f};
        velocity = {Random::value<float>(-2.0f, 2.0f), Random::value<float>(-2.0f, 2.0f), 0.0f, 0.0f};
        color = {Random::value<float>(0.5f, 1.0f), Random::value<float>(0.5f, 1.0f), Random::value<float>(0.5f, 1.0f), 1.0f};
        lifetime = Random::value<float>(12.0f, 124.0f);
        mass = 10.0f;
        age = 0.0f;
    }
};

struct WindowState
{
    int keys[256];
    int buttons[8];
    vec<float, 2> cursor;
    float frametime;
    float padding;
};

struct Quad
{
    vec<float, 4> viewport;
};

#define TOTAL_PARTICLES 1024

int main()
{
    auto window = window_create({.title = "Particle Storm", .width = 1280, .height = 720, .vsync = false});

    auto& window_width = window_get_width_ref(window);
    auto& window_height = window_get_height_ref(window);


    auto quad_group = buffer_group_create("quads");
    buffer_group_restrict_to_keys(quad_group, {"Quads"});

    auto particle_group = buffer_group_create("particles");
    buffer_group_restrict_to_keys(particle_group, {"Particles"});

    auto window_group = buffer_group_create("window_group");
    buffer_group_restrict_to_keys(window_group, {"WindowStates"});

    auto image_group = buffer_group_create("image_group");
    buffer_group_restrict_to_keys(image_group, {"color_image"});

    auto image_clear_compute_shader = shader_create();
    auto particle_motion_compute_shader = shader_create();
    auto render_shader = shader_create();
    
    DrawListManager<Quad> quad_draw_list_mg("Quads", [&](auto buffer_group, auto& quad) -> std::pair<Shader*, uint32_t> {
        return {render_shader, 6};
    });

    window_add_drawn_buffer_group(window, &quad_draw_list_mg, quad_group);

    shader_add_buffer_group(particle_motion_compute_shader, particle_group);
    shader_add_buffer_group(particle_motion_compute_shader, window_group);
    shader_add_buffer_group(particle_motion_compute_shader, image_group);

    shader_add_buffer_group(image_clear_compute_shader, image_group);

    shader_add_buffer_group(render_shader, image_group);
    shader_add_buffer_group(render_shader, quad_group);

    std::string version = "#version 450\n";

    std::string struct_def = R"(
    struct Particle {
        vec4 position;
        vec4 velocity;
        vec4 color;
        float lifetime;
        float age;
        float mass;
        float padding;

    };
    layout(std430, binding = 0) buffer ParticleBuffer {
        Particle particles[];
    } Particles;
    )";

    std::string input_state = R"(
struct WindowState {
    int keys[256];
    int buttons[8];
    vec2 cursor;
    float frametime;
    float padding;
};

layout(std430, binding = 1) buffer WindowStatesBuffer {
    WindowState states[];
} WindowStates;
    )";

    shader_add_module(particle_motion_compute_shader, ShaderModuleType::Compute,
version + "layout(local_size_x = 64) in;\n" + struct_def + input_state + R"(
layout(binding = 2, rgba32f) uniform image2D color_image;
void main() {
    uint id = gl_GlobalInvocationID.x;

    uint particles_length = Particles.particles.length();

    if (id >= particles_length) return;

    WindowState state = WindowStates.states[0];

    float dt = state.frametime;

    Particle p = Particles.particles[id];

    p.position.xyz += p.velocity.xyz * dt;
    p.age += dt;

    vec2 particlePosXY = p.position.xy;
    vec2 cursorPos = vec2(state.cursor.x, state.cursor.y);
    vec2 dirToCursor = cursorPos - particlePosXY;
    float distance = length(dirToCursor);
    vec2 dirNorm = (distance > 0.0001) ? normalize(dirToCursor) : vec2(0.0, 0.0);

    float interactionStrength = 200.0;
    float effectRadius = 150.0;

    if (distance < effectRadius) {
        float falloff = 1.0 - (distance / effectRadius);
        if (state.buttons[0] == 1) {
            vec2 pushForce = -dirNorm * interactionStrength * falloff * dt;
            p.position.xy += pushForce;
        } else if (state.buttons[1] == 1) {
            vec2 pullForce = dirNorm * interactionStrength * falloff * dt;
            p.position.xy += pullForce;
        }
    }

    // Gravity attraction
    vec2 gravityForce = vec2(0.0);
    for (uint i = 0; i < particles_length; ++i) {
        if (i == id) continue;
        Particle other = Particles.particles[i];
        vec2 toOther = other.position.xy - p.position.xy;
        float distSq = dot(toOther, toOther);
        if (distSq < 1.0) continue;
        float dist = sqrt(distSq);
        vec2 forceDir = toOther / dist;
        float strength = (p.mass * other.mass) / distSq;
        gravityForce += forceDir * strength;
    }

    // Apply gravity acceleration to velocity
    p.velocity.xy += gravityForce * dt;

    imageStore(color_image, ivec2(int(p.position.x), int(p.position.y)), p.color);

    Particles.particles[id] = p;
}
)");

    shader_add_module(render_shader, ShaderModuleType::Vertex,
version + R"(
struct Quad {
    vec4 viewport;
};

layout(std430, binding = 0) buffer QuadsBuffer {
    Quad quads[];
} Quads;

vec3 get_quad_position()
{
    vec3 pos;
    switch (gl_VertexIndex)
    {
    case 0: pos = vec3(1.0, 1.0, 0);    break;
    case 1: pos = vec3(-1.0, 1.0, 0);   break;
    case 2: pos = vec3(-1.0, -1.0, 0);  break;
    case 3: pos = vec3(-1.0, -1.0, 0);  break;
    case 4: pos = vec3(1.0, -1.0, 0);   break;
    case 5: pos = vec3(1.0, 1.0, 0);    break;
    }
    // vec4 viewport = Quads.quads[gl_InstanceIndex].viewport;
    return pos;
}

vec2 get_quad_uv()
{
    switch (gl_VertexIndex)
    {
    case 0: return vec2(1, 1);
    case 1: return vec2(0, 1);
    case 2: return vec2(0, 0);
    case 3: return vec2(0, 0);
    case 4: return vec2(1, 0);
    case 5: return vec2(1, 1);
    }
    return vec2(0);
}

layout(location = 0) out vec2 UV;
void main() {
    gl_Position = vec4(get_quad_position(), 1);
    UV = get_quad_uv();
}
)");

    shader_add_module(render_shader, ShaderModuleType::Fragment,
version + R"(
layout(location = 0) in vec2 UV;
layout(location = 0) out vec4 outColor;
layout(binding = 2) uniform sampler2D color_image;
void main() {
    outColor = texture(color_image, UV);//vec4(1, 0, 0, 1);
}
)");

    shader_add_module(image_clear_compute_shader, ShaderModuleType::Compute,
version + R"(

layout(local_size_x = 16, local_size_y = 16) in;

layout(binding = 2, rgba32f) uniform image2D color_image;

void main()
{
    ivec2 pixel_coords = ivec2(gl_GlobalInvocationID.xy);

    // Get image size
    ivec2 image_size = imageSize(color_image);

    // Only write if inside image bounds
    if (pixel_coords.x >= image_size.x || pixel_coords.y >= image_size.y)
    {
        return;
    }

    // Clear color: black with full opacity
    vec4 clear_color = vec4(0.0, 0.0, 0.0, 1.0);

    imageStore(color_image, pixel_coords, clear_color);
}
)");

    buffer_group_set_buffer_element_count(particle_group, "Particles", TOTAL_PARTICLES);
    buffer_group_set_buffer_element_count(window_group, "WindowStates", 1);
    buffer_group_set_buffer_element_count(quad_group, "Quads", 1);
    Image* color_image = buffer_group_define_image_2D(image_group, "color_image", window_width, window_height);

    buffer_group_initialize(particle_group);
    buffer_group_initialize(window_group);
    buffer_group_initialize(image_group);
    buffer_group_initialize(quad_group);

    AABB<float, 2> bounds(
        {(window_width / 2.f) - (window_width / 4.f), (window_height / 2.f) - (window_height / 4.f)}, // min
        {(window_width / 2.f) + (window_width / 4.f), (window_height / 2.f) + (window_height / 4.f)} // max
    );
    for (uint32_t i = 0; i < TOTAL_PARTICLES; ++i)
    {
        auto particles_view = buffer_group_get_buffer_element_view(particle_group, "Particles", i);
        auto& p = particles_view.template as_struct<Particle>();
        p.reset(bounds);
    }

    auto state_view = buffer_group_get_buffer_element_view(window_group, "WindowStates", 0);
    auto& state = state_view.template as_struct<WindowState>();
    window_set_keys_pointer(window, state.keys);
    window_set_buttons_pointer(window, state.buttons);
    window_set_frametime_pointer(window, &state.frametime);
    window_set_cursor_pointer(window, (float*)&state.cursor);
    
    auto quad_view = buffer_group_get_buffer_element_view(quad_group, "Quads", 0);
    auto& quad = quad_view.template as_struct<Quad>();

    uint32_t workgroupSize = 64;
    uint32_t dispatchX = (TOTAL_PARTICLES + workgroupSize - 1) / workgroupSize;

    while (window_poll_events(window))
    {
        shader_dispatch(image_clear_compute_shader, {(window_width + 15)/16, (window_height + 15)/16, 1});
        shader_dispatch(particle_motion_compute_shader, {dispatchX, 1, 1});
        window_render(window);
    }

    window_free(window);
    return 0;
}
