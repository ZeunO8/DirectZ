#pragma once

#include "Directz.cpp.hpp"

namespace dz {
    void buffer_group_destroy(BufferGroup* bg);

    VkImageUsageFlags infer_image_usage_flags(const std::unordered_map<Shader*, VkDescriptorType>& types);

    bool buffer_group_resize_gpu_buffer(const std::string& name, ShaderBuffer& buffer);
}