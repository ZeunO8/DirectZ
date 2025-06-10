#pragma once
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__) || defined(ANDROID)
#define RENDERER_VULKAN
#endif
#include <dz/BufferGroup.hpp>
#include <dz/Shader.hpp>
#include <dz/Window.hpp>
struct DirectRegistry;
namespace dz
{
    std::shared_ptr<DirectRegistry> make_direct_registry();
    inline static std::shared_ptr<DirectRegistry> DZ_RGY = make_direct_registry();
}
#include <dz/math.hpp>