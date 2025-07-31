/**
 * @file Image.hpp
 * @brief Provides image resizing utilities for 2D and 3D image buffers.
 */
#pragma once
#include "Renderer.hpp"
#include <cstdint>

namespace dz
{
    struct Image;
    struct ImageCreateInfo {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VkImageType image_type = VK_IMAGE_TYPE_2D;
        VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkMemoryPropertyFlags memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkSampleCountFlagBits multisampling = VK_SAMPLE_COUNT_1_BIT;
        bool is_framebuffer_attachment = false;
        void* data = nullptr;
    };

    /**
    * @brief Creates a image given the passed info
    *
    * @returns an Image pointer used in various image_X, framebuffer_Y functions
    */
    Image* image_create(const ImageCreateInfo&);

    /**
    * @brief Uploads new data to an Image
    *
    * @note data must contain the bytes in the correct format
    */
    void image_upload_data(Image* image, void* data);

    /**
     * @brief Resizes a 2D image to the specified dimensions.
     * 
     * @param image Pointer to the Image structure.
     * @param image_width Width of the image in pixels.
     * @param image_height Height of the image in pixels.
     */
    void image_resize_2D(Image*& image, uint32_t image_width, uint32_t image_height, void* data = nullptr, bool create_new = false);

    /**
     * @brief Resizes a 3D image to the specified dimensions.
     * 
     * @param image Pointer to the Image structure.
     * @param image_width Width of the image in pixels.
     * @param image_height Height of the image in pixels.
     * @param image_depth Depth of the image in pixels.
     */
    void image_resize_3D(Image*& image, uint32_t image_width, uint32_t image_height, uint32_t image_depth, void* data = nullptr, bool create_new = false);

    /**
    * @brief Frees an image, should only be called for self owned Images. Images created by BufferGroups should not be passed to this function
    */
    void image_free(Image* image);
    
    /**
    * @brief Creates a single descriptor set for a given Image*
    *
    * @note no need to free the descriptor as this is done by the DirectRegistry
    *
    * @returns a pair containing the SetLayout and Set for usage
    */
    std::pair<VkDescriptorSetLayout, VkDescriptorSet> image_create_descriptor_set(Image* image);

    /**
    * @brief Gets the Channels and Size Of Type as a pair
    */
    std::vector<float> image_get_channels_size_of_t(Image* image);

    /**
    * @brief Computes the total size of a channels vector
    *
    * @returns the channels pixel stride in bytes
    */
    size_t image_get_sizeof_channels(const std::vector<float>& channels);
}