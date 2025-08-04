#pragma once
#include "Directz.cpp.hpp"

namespace dz {
    struct Image
    {
        uint32_t width;
        uint32_t height;
        uint32_t depth;
        VkFormat format;
        VkImageUsageFlags usage;
        VkImageType image_type;
        VkImageViewType view_type;
        VkImageTiling tiling;
        VkMemoryPropertyFlags memory_properties;
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VkImageLayout current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        VkSampleCountFlagBits multisampling;
        std::shared_ptr<void> data;
        SurfaceType surfaceType = SurfaceType::BaseColor;
    };

    struct ImageCreateInfoInternal
    {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;
        VkImageUsageFlags usage;
        VkImageType image_type = VK_IMAGE_TYPE_2D;
        VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkMemoryPropertyFlags memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkSampleCountFlagBits multisampling = VK_SAMPLE_COUNT_1_BIT;
        void* data = nullptr;
        SurfaceType surfaceType = SurfaceType::BaseColor;
    };

    void upload_image_data(Image*);
    void init_empty_image_data(Image*);
    uint32_t image_get_aspect_mask(Image*);

    Image* image_create_internal(const ImageCreateInfoInternal& info);
}