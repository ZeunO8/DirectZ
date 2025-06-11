#pragma once
#include <memory>
#include <string>
#include <vector>
#include "ReflectedStructView.hpp"
#include "Image.hpp"
namespace dz
{
    struct BufferGroup;

    BufferGroup* buffer_group_create(const std::string& group_name);

    void buffer_group_restrict_to_keys(BufferGroup* buffer_group, const std::vector<std::string>& restruct_keys);

    void buffer_group_initialize(BufferGroup* buffer_group);
    
    Image* buffer_group_define_image_2D(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t image_width, uint32_t image_height, void* data_pointer = 0);
    Image* buffer_group_define_image_3D(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t image_width, uint32_t image_height, uint32_t image_depth, void* data_pointer = 0);

    void buffer_group_set_buffer_element_count(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t element_count);
    uint32_t buffer_group_get_buffer_element_count(BufferGroup* buffer_group, const std::string& buffer_name);

    std::shared_ptr<uint8_t> buffer_group_get_buffer_data_ptr(BufferGroup* buffer_group, const std::string& buffer_name);

    ReflectedStructView buffer_group_get_buffer_element_view(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t index);
}