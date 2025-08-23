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
        // VkImageView imageView = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        std::vector<VkImageLayout> current_layouts;

        VkSampleCountFlagBits multisampling;

        bool is_framebuffer_attachment = false;
        
        std::vector<std::shared_ptr<void>> datas;

        SurfaceType surfaceType = SurfaceType::BaseColor;
        uint32_t mip_levels = 1;

        std::vector<VkImageView> imageViews;

        bool data_is_cpu_side = false;
        bool data_is_gpu_side = false;
        bool data_synced_gpu_to_cpu = false;

        SharedMemoryPtr<Image>* image_shm = nullptr;

        size_t id = 0;

        unsigned long long creator_pid = 0;
        void reset_layouts() {
            current_layouts.resize(mip_levels);
            for (auto& current_layout : current_layouts)
                current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
        }
    };

    struct ImageCreateInfoInternal
    {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        VkFormat format = (VkFormat)ColorSpace::SRGB;
        VkImageUsageFlags usage;
        VkImageType image_type = VK_IMAGE_TYPE_2D;
        VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkMemoryPropertyFlags memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkSampleCountFlagBits multisampling = VK_SAMPLE_COUNT_1_BIT;
        bool is_framebuffer_attachment = false;
        std::vector<std::shared_ptr<void>> datas;
        SurfaceType surfaceType = SurfaceType::BaseColor;
        uint32_t mip_levels = 1;
        bool create_shared = false;
    };

    void init_empty_image_data(Image*, uint32_t mip = 0);
    uint32_t image_get_aspect_mask(Image*);

    Image* image_create_internal(const ImageCreateInfoInternal& info);
}