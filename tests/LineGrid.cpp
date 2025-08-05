#include <DirectZ.hpp>

// #define DOUBLE_PRECISION
#if defined(DOUBLE_PRECISION)
using Real = double;
#else
using Real = float;
#endif
using vec3 = vec<Real, 3>;
using vec4 = vec<Real, 4>;
using mat4 = mat<Real, 4, 4>;

struct Line
{
    int vert_offset;
    int vert_count;
};

int main()
{
    auto window = window_create({
        .title = "Line Grid",
        .x = 128,
        .y = 128,
        .width = 640,
        .height = 480,
        .borderless = false,
        .vsync = true
    });

    auto line_buffer_group = buffer_group_create("line_buffer_group");
    buffer_group_restrict_to_keys(line_buffer_group, {"Lines", "LinesData"});

    auto render_line_grid_shader = shader_create(ShaderTopology::LineStrip);

    DrawListManager<Line> line_draw_list_mg("Lines", [&](auto buffer_group, auto& line) -> DrawTuple {
        return { render_line_grid_shader, line.vert_count };
    });

    window_add_drawn_buffer_group(window, &line_draw_list_mg, line_buffer_group);

    shader_add_buffer_group(render_line_grid_shader, line_buffer_group);

    shader_add_module(render_line_grid_shader, ShaderModuleType::Vertex,
    R"(
#version 450

struct Line
{
    int vert_offset;
    int vert_count;
};

layout(std430, binding = 0) buffer LinesBuffer
{
    Line lines[];
} Lines;

layout(std430, binding = 1) buffer LinesDataBuffer
{
    vec4 data[];
} LinesData;

void main()
{
    Line line = Lines.lines[gl_InstanceIndex];
    vec4 position = LinesData.data[line.vert_offset + gl_VertexIndex];
    gl_Position = position;
}
    )");

    shader_add_module(render_line_grid_shader, ShaderModuleType::Fragment,
    R"(
#version 450

layout(location = 0) out vec4 FragColor;

void main()
{
    FragColor = vec4(1, 1, 1, 1);
}
    )");

    std::vector<Line*> line_ptrs;
    line_ptrs.resize(1);
    buffer_group_set_buffer_element_count(line_buffer_group, "Lines", line_ptrs.size());

    size_t lines_data_size;
    lines_data_size = 2;
    buffer_group_set_buffer_element_count(line_buffer_group, "LinesData", lines_data_size);

    buffer_group_initialize(line_buffer_group);
    
    for (size_t i = 0; i < line_ptrs.size(); ++i)
    {
        auto line_view = buffer_group_get_buffer_element_view(line_buffer_group, "Lines", i);
        auto& line = line_view.as_struct<Line>();
        line_ptrs[i] = &line;
    }
    
    auto lines_data = (vec4*)(buffer_group_get_buffer_data_ptr(line_buffer_group, "LinesData").get());

    auto& line = *line_ptrs[0];
    line.vert_offset = 0;
    line.vert_count = 2;

    float x = 0, y = 0, z = 0;
    for (size_t i = 0; i < lines_data_size; i++)
    {
        auto& line_data = lines_data[i];
        line_data[0] = x;
        line_data[1] = y;
        line_data[2] = z;
        line_data[3] = 1;

        x += 0.5;
        y += 0.25;
        z += 0.125;
    }

    while (window_poll_events(window))
    {
        window_render(window);
    }
}