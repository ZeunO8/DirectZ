#pragma once
#if defined(_WIN32) || defined(__linux__) || defined(__APPLE__) || defined(ANDROID)
#define RENDERER_VULKAN
#endif
#include <filesystem>
#include <dz/BufferGroup.hpp>
#include <dz/Shader.hpp>
#include <dz/Window.hpp>
#include <dz/DrawListManager.hpp>
#include <dz/math.hpp>
#include <dz/FileHandle.hpp>
#include <dz/internal/memory_stream.hpp>
#include <dz/size_ptr.hpp>
#include <dz/AssetPack.hpp>
#include <dz/KeyValueStream.hpp>
#include <dz/ProgramArgs.hpp>
namespace dz
{
    void set_env(const std::string& key, const std::string& value);
    std::string get_env(const std::string& key);
    std::filesystem::path getUserDirectoryPath();
    std::filesystem::path getProgramDirectoryPath();
    std::filesystem::path getProgramDataPath();
    std::filesystem::path getExecutableName();
}
#ifdef _WIN32
#define DZ_EXPORT extern "C" __declspec(dllexport)
#else
#define DZ_EXPORT extern "C" __attribute__((visibility("default")))
#endif
using namespace dz;
DZ_EXPORT int dz_init(WINDOW* &window);
DZ_EXPORT void dz_update();