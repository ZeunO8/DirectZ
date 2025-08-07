#include <dz/ECS/Material.hpp>
#include <dz/GlobalUID.hpp>

dz::ecs::Material::MaterialReflectable::MaterialReflectable(const std::function<Material*()>& get_material_function):
    get_material_function(get_material_function),
    uid(int(GlobalUID::GetNew("Reflectable"))),
    name("Material")
{}

int dz::ecs::Material::MaterialReflectable::GetID() {
    return uid;
}

std::string& dz::ecs::Material::MaterialReflectable::GetName() {
    return name;
}

void* dz::ecs::Material::MaterialReflectable::GetVoidPropertyByIndex(int prop_index) {
    auto material_ptr = get_material_function();
    assert(material_ptr);
    auto& material = *material_ptr;
    switch (prop_index) {
    case 0: return &material.albedo_color;
    case 1: return &material.metalness;
    case 2: return &material.roughness;
    default: return nullptr;
    }
}

void dz::ecs::Material::MaterialReflectable::NotifyChange(int prop_index) {}

BufferGroup* ensure_brdfLUT_buffer_group() {
    static BufferGroup* buffer_group = nullptr;
    if (buffer_group)
        return buffer_group;
    buffer_group = buffer_group_create("brdfLUT_Group");
    buffer_group_restrict_to_keys(buffer_group, {"brdfLUT"});
    return buffer_group;
}

std::string Generate_brdfLUT_Compute_Shader();

Shader* dz::ecs::Material::ensure_brdfLUT_shader(Image* brdfLUT_image) {
    static Shader* shader = nullptr;

    if (shader)
        return shader;

    shader = shader_create();

    auto buffer_group = ensure_brdfLUT_buffer_group();

    shader_add_buffer_group(shader, buffer_group);

    shader_add_module(shader, ShaderModuleType::Compute, Generate_brdfLUT_Compute_Shader());

    shader_use_image(shader, "brdfLUT", brdfLUT_image);

    shader_initialize(shader);

    shader_update_descriptor_sets(shader);

    return shader;
}

static Image* brdfLUT_image = nullptr;

void dz::ecs::Material::StaticInitialize() {
    brdfLUT_image = image_create({
        .width = 512,
        .height = 512,
        .format = VK_FORMAT_R16G16B16A16_SFLOAT,
        .usage = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT
    });
    Shader* brdfLUT_shader = ensure_brdfLUT_shader(brdfLUT_image);
    transition_image_layout(brdfLUT_image, VK_IMAGE_LAYOUT_GENERAL);
    shader_dispatch(brdfLUT_shader, 512 / 16, 512 / 16, 1);
    transition_image_layout(brdfLUT_image, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    return;
}

void dz::ecs::Material::ShaderTweak(Shader* shader) {
    shader_use_image(shader, "brdfLUT", brdfLUT_image);
}

std::string Generate_brdfLUT_Compute_Shader() {
    std::string shader_string;

    shader_string += R"(
#version 450

layout (local_size_x = 16, local_size_y = 16) in;

layout (binding = 0, rgba16f) uniform image2D brdfLUT;

const float PI = 3.14159265359;

vec3 ImportanceSampleGGX(vec2 Xi, float Roughness, vec3 N)
{
    float a = Roughness * Roughness;
    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    vec3 up        = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangentX  = normalize(cross(up, N));
    vec3 tangentY  = cross(N, tangentX);
    vec3 sampleVec = tangentX * H.x + tangentY * H.y + N * H.z;

    return normalize(sampleVec);
}

float GeometrySchlickGGX(float NdotV, float Roughness)
{
    float a = Roughness;
    float k = (a * a) / 2.0;
    float denom = NdotV * (1.0 - k) + k;
    return NdotV / denom;
}

float GeometrySmith(float NdotV, float NdotL, float Roughness)
{
    float ggx2 = GeometrySchlickGGX(NdotV, Roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, Roughness);
    return ggx1 * ggx2;
}

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec2 IntegrateBRDF(float NdotV, float Roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, Roughness, N);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G = GeometrySmith(NdotV, NdotL, Roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }

    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main()
{
    ivec2 pixelCoord = ivec2(gl_GlobalInvocationID.xy);
    if (pixelCoord.x >= 512 || pixelCoord.y >= 512)
        return;

    vec2 uv = vec2(pixelCoord) / vec2(511.0, 511.0);
    float NdotV = uv.x;
    float Roughness = uv.y;

    vec2 integratedBRDF = IntegrateBRDF(NdotV, Roughness);

    imageStore(brdfLUT, pixelCoord, vec4(integratedBRDF, 0.0, 1.0));
}
)";

    return shader_string;
}