#include <dz/Shader.hpp>
using namespace dz;
#include <atomic>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <vector>
#include <fstream>
#include <optional>
#include <array>
#include <chrono>
#include <set>
#include <dz/GlobalUID.hpp>
#include <spirv_reflect.h>
#include <shaderc/shaderc.hpp>
#if defined(_WIN32)
#define VK_USE_PLATFORM_WIN32_KHR
#elif defined(__linux__) && !defined(__ANDROID__)
#define VK_USE_PLATFORM_XCB_KHR
#elif defined(MACOS)
#define VK_USE_PLATFORM_MACOS_MVK
#elif defined(__ANDROID__)
#define VK_USE_PLATFORM_ANDROID_KHR
#endif
#include <vulkan/vulkan.h>
#if defined(_WIN32)
#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
#include <windows.h>
#elif defined(__linux__) && !defined(__ANDROID__)
#include <xcb/xcb.h>
#include <xcb/xcb_keysyms.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/XKBlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/keysymdef.h>
#include <xcb/xfixes.h>
#include <xkbcommon/xkbcommon.h>
#include <dlfcn.h>
#elif defined(MACOS)
#include <dlfcn.h>
#elif defined(__ANDROID__)
#include <dlfcn.h>
#endif
#undef min
#undef max