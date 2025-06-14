#pragma once
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__) || defined(ANDROID)
#define RENDERER_VULKAN
#endif
#include <dz/BufferGroup.hpp>
#include <dz/Shader.hpp>
#include <dz/Window.hpp>
#include <dz/DrawListManager.hpp>
#include <dz/math.hpp>
namespace dz
{
    void set_env(const std::string& key, const std::string& value);
    std::string get_env(const std::string& key);
}