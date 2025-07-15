// Signed Distance Map
//3g
// Sample Distance
//1u
// Point at which particle exists inside sign
// at const & pluj + plug u (O)
// Gravitate towarju326412851210244096variable8
// everyone that uin>?
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
        velocity = {0, 0, 0, 0};//{Random::value<float>(-2.0f, 2.0f), Random::value<float>(-2.0f, 2.0f), 0.0f, 0.0f};
        color = {Random::value<float>(0.5f, 1.0f), Random::value<float>(0.5f, 1.0f), Random::value<float>(0.5f, 1.0f), 1.0f};
        lifetime = std::numeric_limits<float>::infinity();//Random::value<float>(12.0f, 124.0f);
        mass = Random::value<float>(10.0f, 200.f);
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
    AABB<float, 2> spawn_bounds;
    float width;
    float height;
};

struct Quad
{
    vec<float, 4> viewport;
};

struct GridDimensions
{
    int width;
    int height;
    float cellWidth;
    float cellHeight;
};

#define TOTAL_PARTICLES 32'000

int main()
{
    auto window = window_create({.title = "Particle Storm", .width = 1280, .height = 720, .vsync = false});

    auto& window_width_ptr = window_get_width_ref(window);
    auto& window_width = *window_width_ptr;
    auto& window_height_ptr = window_get_height_ref(window);
    auto& window_height = *window_height_ptr;


    auto quad_group = buffer_group_create("quads");
    buffer_group_restrict_to_keys(quad_group, {"Quads"});

    auto particle_group = buffer_group_create("particles");
    buffer_group_restrict_to_keys(particle_group, {"Particles"});

    auto window_group = buffer_group_create("window_group");
    buffer_group_restrict_to_keys(window_group, {"WindowStates"});

    auto image_group = buffer_group_create("image_group");
    buffer_group_restrict_to_keys(image_group, {"color_image"});

    auto density_group = buffer_group_create("density_group");
    buffer_group_restrict_to_keys(density_group, {"DensityField"});

    auto grids_group = buffer_group_create("grids_group");
    buffer_group_restrict_to_keys(grids_group, {"Grids"});
    
    auto force_field_group = buffer_group_create("force_field_group");
    buffer_group_restrict_to_keys(force_field_group, {"ForceField"});

    auto image_clear_compute_shader = shader_create();
    auto clear_density_shader = shader_create();
    auto deposit_mass_shader = shader_create();
    auto gradient_force_shader = shader_create();
    auto gravity_motion_shader = shader_create();
    auto render_shader = shader_create();
    auto diffuse_density_shader = shader_create();
    
    DrawListManager<Quad> quad_draw_list_mg("Quads", [&](auto buffer_group, auto& quad) -> DrawTuple {
        return {render_shader, 6};
    });

    window_add_drawn_buffer_group(window, &quad_draw_list_mg, quad_group);

    shader_add_buffer_group(clear_density_shader, density_group);
    
    shader_add_buffer_group(deposit_mass_shader, density_group);
    shader_add_buffer_group(deposit_mass_shader, particle_group);
    shader_add_buffer_group(deposit_mass_shader, grids_group);

    shader_add_buffer_group(gradient_force_shader, density_group);
    shader_add_buffer_group(gradient_force_shader, force_field_group);
    shader_add_buffer_group(gradient_force_shader, grids_group);

    shader_add_buffer_group(gravity_motion_shader, particle_group);
    shader_add_buffer_group(gravity_motion_shader, window_group);
    shader_add_buffer_group(gravity_motion_shader, image_group);
    shader_add_buffer_group(gravity_motion_shader, force_field_group);
    shader_add_buffer_group(gravity_motion_shader, grids_group);

    shader_add_buffer_group(image_clear_compute_shader, image_group);

    shader_add_buffer_group(render_shader, image_group);
    shader_add_buffer_group(render_shader, quad_group);

    shader_add_buffer_group(diffuse_density_shader, density_group);
    shader_add_buffer_group(diffuse_density_shader, grids_group);

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

    std::string density_struct_def = R"(
struct DensityFieldCell {
    float mass;
    float padding1;
    float padding2;
    float padding3;
};

layout(std430, binding = 3) buffer DensityBuffer {
    DensityFieldCell field[];
} DensityField;
    )";

    std::string force_field_struct_def = R"(
struct ForceFieldCell {
    vec4 force;
};
layout(std430, binding = 5) buffer ForceBuffer {
    ForceFieldCell field[];
} ForceField;
    )";

    std::string grid_dim_struct_def = R"(
struct GridDimensions {
    int width;
    int height;
    float cellWidth;
    float cellHeight;
};

layout(std430, binding = 4) buffer GridsBuffer {
    GridDimensions grids[];
} Grids;
    )";

    std::string input_state = R"(
struct AABB {
    vec2 min;
    vec2 max;
};
struct WindowState {
    int keys[256];
    int buttons[8];
    vec2 cursor;
    float frametime;
    float padding;
    AABB spawn_bounds;
    float width;
    float height;
};

layout(std430, binding = 1) buffer WindowStatesBuffer {
    WindowState states[];
} WindowStates;
    )";

    // clear density
    shader_add_module(clear_density_shader, ShaderModuleType::Compute,
version +
density_struct_def + R"(

layout(local_size_x = 128) in;

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= DensityField.field.length()) return;
    DensityField.field[id].mass = 0.0;
}
)");

    // deposit mass
    shader_add_module(deposit_mass_shader, ShaderModuleType::Compute,
version + 
"#extension GL_EXT_shader_atomic_float : require\n" +
density_struct_def +
struct_def +
grid_dim_struct_def + R"(

layout(local_size_x = 64) in;

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= Particles.particles.length()) return;

    Particle p = Particles.particles[id];

    ivec2 gridCoords = ivec2(p.position.xy / vec2(Grids.grids[0].cellWidth, Grids.grids[0].cellHeight));

    int x = clamp(gridCoords.x, 0, Grids.grids[0].width - 1);
    int y = clamp(gridCoords.y, 0, Grids.grids[0].height - 1);
    int idx = y * Grids.grids[0].width + x;

    atomicAdd(DensityField.field[idx].mass, p.mass);
}
)");

    shader_add_module(diffuse_density_shader, ShaderModuleType::Compute,
    version + density_struct_def + grid_dim_struct_def + R"(

layout(local_size_x = 32, local_size_y = 32) in;

void main() {
    ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
    int width = Grids.grids[0].width;
    int height = Grids.grids[0].height;

    if (coords.x >= width || coords.y >= height) return;

    float sum_mass = 0.0;
    float count = 0.0;

    // Iterate over 3x3 neighborhood (including self)
    for (int dy = -1; dy <= 1; ++dy) {
        for (int dx = -1; dx <= 1; ++dx) {
            ivec2 neighbor_coords = coords + ivec2(dx, dy);
            // Clamp neighbor coordinates to grid bounds to avoid out-of-bounds access
            int clamped_x = clamp(neighbor_coords.x, 0, width - 1);
            int clamped_y = clamp(neighbor_coords.y, 0, height - 1);
            
            sum_mass += DensityField.field[clamped_y * width + clamped_x].mass;
            count += 1.0;
        }
    }

    // Store the averaged mass back into the density field.
    DensityField.field[coords.y * width + coords.x].mass = sum_mass / count;
}
)");

    shader_add_module(gradient_force_shader, ShaderModuleType::Compute,
    version + density_struct_def + force_field_struct_def + grid_dim_struct_def + R"(
layout(local_size_x = 32, local_size_y = 32) in;

void main() {
    ivec2 coords = ivec2(gl_GlobalInvocationID.xy);
    int width = Grids.grids[0].width;
    int height = Grids.grids[0].height;

    if (coords.x >= width || coords.y >= height) return;

    // Sample mass from neighboring cells, clamping at the edges to avoid out-of-bounds access.
    float mass_left   = DensityField.field[ (coords.y * width) + clamp(coords.x - 1, 0, width - 1) ].mass;
    float mass_right  = DensityField.field[ (coords.y * width) + clamp(coords.x + 1, 0, width - 1) ].mass;
    float mass_down   = DensityField.field[ clamp(coords.y - 1, 0, height - 1) * width + coords.x ].mass;
    float mass_up     = DensityField.field[ clamp(coords.y + 1, 0, height - 1) * width + coords.x ].mass;

    // The force is the gradient of the density field. It points from low density to high density.
    vec2 force = vec2(mass_right - mass_left, mass_up - mass_down);

    int idx = coords.y * width + coords.x;
    ForceField.field[idx].force = vec4(force, 0.0, 0.0);
}
)");

    shader_add_module(gravity_motion_shader, ShaderModuleType::Compute,
    version + "layout(local_size_x = 64) in;\n" +
    struct_def + input_state + grid_dim_struct_def + force_field_struct_def + R"(
layout(binding = 2, rgba32f) uniform image2D color_image;

float rand(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

// Bilinear interpolation function to smoothly sample the force field
vec2 bilerp(vec2 p, vec2 f00, vec2 f10, vec2 f01, vec2 f11) {
    vec2 f_y1 = mix(f00, f10, p.x);
    vec2 f_y2 = mix(f01, f11, p.x);
    return mix(f_y1, f_y2, p.y);
}

void main() {
    uint id = gl_GlobalInvocationID.x;
    if (id >= Particles.particles.length()) return;

    WindowState state = WindowStates.states[0];
    float dt = state.frametime;
    Particle p = Particles.particles[id];

    // Particle Recycling logic (unchanged)
    p.age += dt;
    if (p.age > p.lifetime) {
        vec2 spawn_range = state.spawn_bounds.max - state.spawn_bounds.min;
        p.position.xy = state.spawn_bounds.min + spawn_range * vec2(rand(p.position.xy + id), rand(p.position.yx + id));
        p.velocity = vec4(0.0);
        p.age = 0.0;
        p.lifetime = 12.0 + rand(p.position.xy) * 112.0;
        p.mass = 10.0 + rand(p.position.yx) * 190.0;
    }
    
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

    // --- FIX: Force calculation now samples the pre-computed force field ---
    // This constant will need tuning. It has a different scale than the old gravity constant.
    const float G = 0.05; 

    vec2 cellSize = vec2(Grids.grids[0].cellWidth, Grids.grids[0].cellHeight);
    int width = Grids.grids[0].width;
    int height = Grids.grids[0].height;
    
    // Find the particle's position in grid coordinates (including the fractional part)
    vec2 gridPos = p.position.xy / cellSize;
    // Get the integer coordinates of the bottom-left cell
    ivec2 p00_coords = ivec2(floor(gridPos - 0.5));
    // Get the fractional part for interpolation
    vec2 fract_part = fract(gridPos - 0.5);

    // Get the force vectors from the four surrounding grid cells
    ivec2 p00 = clamp(p00_coords, ivec2(0), ivec2(width-1, height-1));
    ivec2 p10 = clamp(p00_coords + ivec2(1, 0), ivec2(0), ivec2(width-1, height-1));
    ivec2 p01 = clamp(p00_coords + ivec2(0, 1), ivec2(0), ivec2(width-1, height-1));
    ivec2 p11 = clamp(p00_coords + ivec2(1, 1), ivec2(0), ivec2(width-1, height-1));

    vec2 f00 = ForceField.field[p00.y * width + p00.x].force.xy;
    vec2 f10 = ForceField.field[p10.y * width + p10.x].force.xy;
    vec2 f01 = ForceField.field[p01.y * width + p01.x].force.xy;
    vec2 f11 = ForceField.field[p11.y * width + p11.x].force.xy;
    
    // Bilinearly interpolate the force to get a smooth value at the particle's exact position
    vec2 gForce = bilerp(fract_part, f00, f10, f01, f11);

    p.velocity.xy += G * gForce * dt;
    p.velocity.xy *= 0.999; 
    p.position.xyz += p.velocity.xyz * dt;

    if (p.position.x <= 8 || p.position.x >= (WindowStates.states[0].width - 8))
    {
        p.velocity.x *= -1.0;
    }
    else if (p.position.y <= 8 || p.position.y >= (WindowStates.states[0].height - 8))
    {
        p.velocity.y *= -1.0;
    }
    
    imageStore(color_image, ivec2(p.position.xy), p.color);
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

layout(local_size_x = 32, local_size_y = 32) in;

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

#define GRID_RESOLUTION_DIVISOR 64
    auto density_field_width = window_width / GRID_RESOLUTION_DIVISOR;
    auto density_field_height = window_height / GRID_RESOLUTION_DIVISOR;
    auto density_field_size = density_field_width * density_field_height;
    buffer_group_set_buffer_element_count(density_group, "DensityField", density_field_size);
    buffer_group_set_buffer_element_count(force_field_group, "ForceField", density_field_size);
    buffer_group_set_buffer_element_count(grids_group, "Grids", 1);

    buffer_group_initialize(particle_group);
    buffer_group_initialize(window_group);
    buffer_group_initialize(image_group);
    buffer_group_initialize(quad_group);
    buffer_group_initialize(density_group);
    buffer_group_initialize(force_field_group);
    buffer_group_initialize(grids_group);

    AABB<float, 2> bounds(
        {(window_width / 2.f) - (window_width / 3.f), (window_height / 2.f) - (window_height / 3.f)}, // min
        {(window_width / 2.f) + (window_width / 3.f), (window_height / 2.f) + (window_height / 3.f)} // max
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
    window_set_float_frametime_pointer(window, &state.frametime);
    window_set_cursor_pointer(window, (float*)&state.cursor);
    state.width = window_width;
    state.height = window_height;
    window_set_width_pointer(window, &state.width);
    window_set_height_pointer(window, &state.height);
    state.spawn_bounds = bounds;
    
    auto quad_view = buffer_group_get_buffer_element_view(quad_group, "Quads", 0);
    auto& quad = quad_view.template as_struct<Quad>();

    
    auto grid_view = buffer_group_get_buffer_element_view(grids_group, "Grids", 0);
    auto& grid = grid_view.template as_struct<GridDimensions>();

    grid.width = density_field_width;
    grid.height = density_field_height;
    grid.cellWidth = static_cast<float>(*window_width_ptr) / density_field_width;
    grid.cellHeight = static_cast<float>(*window_height_ptr) / density_field_height;

    uint32_t workgroupSize = 64;
    uint32_t dispatchX = (TOTAL_PARTICLES + workgroupSize - 1) / workgroupSize;

    const uint32_t numDiffusionPasses = density_field_width / 2.f;

    while (window_poll_events(window))
    {
        shader_dispatch(image_clear_compute_shader, {(*window_width_ptr + 31)/32, (*window_height_ptr + 31)/32, 1});
        shader_dispatch(clear_density_shader, {(density_field_size + 127)/128, 1, 1});
        shader_dispatch(deposit_mass_shader, {dispatchX, 1, 1});
        for (uint32_t i = 0; i < numDiffusionPasses; ++i) {
            shader_dispatch(diffuse_density_shader, {(uint32_t)(density_field_width + 31)/32, (uint32_t)(density_field_height + 31)/32, 1});
        }
        shader_dispatch(gradient_force_shader, {(uint32_t)(density_field_width + 31)/32, (uint32_t)(density_field_height + 31)/32, 1});
        shader_dispatch(gravity_motion_shader, {dispatchX, 1, 1});
        window_render(window);
    }
    return 0;
}
