/**
 * @file BufferGroup.hpp
 * @brief Abstractions for managing GPU buffer groups with named buffers and images.
 */
#pragma once
#include <memory>
#include <string>
#include <vector>
#include "ReflectedStructView.hpp"
#include "Image.hpp"

namespace dz
{
    struct BufferGroup;

    /**
     * @brief Creates a new BufferGroup with the given name.
     * 
     * @param group_name Logical name of the buffer group.
     * @return Pointer to the created BufferGroup.
     */
    BufferGroup* buffer_group_create(const std::string& group_name);

    /**
     * @brief Restricts the buffer group to a subset of keys.
     * 
     * @param buffer_group Pointer to the BufferGroup.
     * @param restruct_keys List of keys to restrict to.
     */
    void buffer_group_restrict_to_keys(BufferGroup* buffer_group, const std::vector<std::string>& restruct_keys);

    /**
     * @brief Initializes GPU-side resources for the buffer group.
     * 
     * @param buffer_group Pointer to the BufferGroup.
     */
    void buffer_group_initialize(BufferGroup* buffer_group);

    /**
     * @brief Defines a 2D image in the buffer group.
     * 
     * @param buffer_group Pointer to the BufferGroup.
     * @param buffer_name Unique identifier for the buffer.
     * @param image_width Width of the image.
     * @param image_height Height of the image.
     * @param data_pointer Optional pointer to image data.
     * @return Pointer to the created Image.
     */
    Image* buffer_group_define_image_2D(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t image_width, uint32_t image_height, void* data_pointer = 0);

    /**
     * @brief Defines a 3D image in the buffer group.
     * 
     * @param buffer_group Pointer to the BufferGroup.
     * @param buffer_name Unique identifier for the buffer.
     * @param image_width Width of the image.
     * @param image_height Height of the image.
     * @param image_depth Depth of the image.
     * @param data_pointer Optional pointer to image data.
     * @return Pointer to the created Image.
     */
    Image* buffer_group_define_image_3D(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t image_width, uint32_t image_height, uint32_t image_depth, void* data_pointer = 0);

    /**
     * @brief Sets the number of elements in a named buffer.
     * 
     * @param buffer_group Pointer to the BufferGroup.
     * @param buffer_name Name of the buffer.
     * @param element_count Number of elements.
     */
    void buffer_group_set_buffer_element_count(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t element_count);

    /**
     * @brief Gets the number of elements in a named buffer.
     * 
     * @param buffer_group Pointer to the BufferGroup.
     * @param buffer_name Name of the buffer.
     * @return Number of elements.
     */
    uint32_t buffer_group_get_buffer_element_count(BufferGroup* buffer_group, const std::string& buffer_name);

    /**
    * @brief Gets the element size of a named buffer
    */
    uint32_t buffer_group_get_buffer_element_size(BufferGroup* buffer_group, const std::string& buffer_name);

    /**
     * @brief Gets the raw buffer data pointer for a named buffer.
     * 
     * @param buffer_group Pointer to the BufferGroup.
     * @param buffer_name Name of the buffer.
     * @return Shared pointer to raw buffer data.
     */
    std::shared_ptr<uint8_t> buffer_group_get_buffer_data_ptr(BufferGroup* buffer_group, const std::string& buffer_name);

    /**
     * @brief Returns a view into a single struct element in the named buffer.
     * 
     * @param buffer_group Pointer to the BufferGroup.
     * @param buffer_name Name of the buffer.
     * @param index Index of the element.
     * @return Struct view object allowing reflection.
     */
    ReflectedStructView buffer_group_get_buffer_element_view(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t index);
}