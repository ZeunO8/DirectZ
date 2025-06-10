#pragma once
#include <memory>
#include <string>
#include "ReflectedStructView.hpp"
namespace dz
{
    struct BufferGroup;
    BufferGroup* buffer_group_create(const std::string& group_name);
    void buffer_group_initialize(BufferGroup* buffer_group);
    void buffer_group_set_buffer_element_count(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t element_count);
    uint32_t buffer_group_get_buffer_element_count(BufferGroup* buffer_group, const std::string& buffer_name);
    std::shared_ptr<uint8_t> buffer_group_get_buffer_data_ptr(BufferGroup* buffer_group, const std::string& buffer_name);
    ReflectedStructView buffer_group_get_buffer_element_view(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t index);
}