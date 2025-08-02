#include <dz/Image.hpp>
#include "Directz.cpp.hpp"
#include "Image.cpp.hpp"

namespace dz {

    void upload_image_data(Image* image);

    Image* image_create_internal(const ImageCreateInfoInternal& info);
    
    void image_pre_resize_2D_internal(Image* image_ptr, uint32_t width, uint32_t height) {
        auto& image = *image_ptr;
        image.width = width;
        image.height = height;
        image.current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    void image_pre_resize_3D_internal(Image* image_ptr, uint32_t width, uint32_t height, uint32_t depth) {
        auto& image = *image_ptr;
        image.width = width;
        image.height = height;
        image.depth = depth;
        image.current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    void image_init(Image* image);
    
    Image* image_create(const ImageCreateInfo& info) {
        auto usage = info.usage;
        switch (info.format)
        {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            if (info.is_framebuffer_attachment)
                usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
            break;
        case VK_FORMAT_D32_SFLOAT:
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
        case VK_FORMAT_R8_UINT:
            if (info.is_framebuffer_attachment)
                usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
            break;
        default:
            break;
        }
        ImageCreateInfoInternal internal_info{
            .width = info.width,
            .height = info.height,
            .depth = info.depth,
            .format = info.format,
            .usage = usage,
            .image_type = info.image_type,
            .view_type = info.view_type,
            .tiling = info.tiling,
            .memory_properties = info.memory_properties,
            .multisampling = info.multisampling,
            .data = info.data
        };
        return image_create_internal(internal_info);
    }

    void image_cpy_data(Image* image, void* data) {
        if (!data)
            return;
        auto image_byte_size = image->width * image->height * image->depth * 4 * sizeof(char);
        image->data = std::shared_ptr<void>(malloc(image_byte_size), free);
        memcpy(image->data.get(), data, image_byte_size);
    }

    Image* image_create_internal(const ImageCreateInfoInternal& info) {
        Image* result = new Image{
            .width = info.width,
            .height = info.height,
            .depth = info.depth,
            .format = info.format,
            .usage = info.usage,
            .image_type = info.image_type,
            .view_type = info.view_type,
            .tiling = info.tiling,
            .memory_properties = info.memory_properties,
            .multisampling = info.multisampling
        };

        image_cpy_data(result, info.data);

        image_init(result);

        return result;
    }

    void image_free_internal(Image* image_ptr) {
        auto& image = *image_ptr;
        auto& device = dr.device;
        if (image.image != VK_NULL_HANDLE) {
            vkDestroyImage(device, image.image, 0);
            image.image = nullptr;
        }
        if (image.imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, image.imageView, 0);
            image.imageView = nullptr;
        }
        if (image.memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, image.memory, 0);
            image.memory = nullptr;
        }
        if(image.sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, image.sampler, 0);
            image.sampler = nullptr;
        }
    }

    void image_resize_2D(Image*& image, uint32_t width, uint32_t height, void* data, bool create_new) {
        if (!image)
            return;
        if (create_new) {
            Image* new_image = new Image{
                .width = width,
                .height = height,
                .depth = image->depth,
                .format = image->format,
                .usage = image->usage,
                .image_type = image->image_type,
                .view_type = image->view_type,
                .tiling = image->tiling,
                .memory_properties = image->memory_properties,
                .multisampling = image->multisampling,
                .data = image->data
            };

            image_init(new_image);

            image = new_image;
            return;
        }
        image_free_internal(image);
        image_pre_resize_2D_internal(image, width, height);
        image_cpy_data(image, data);
        image_init(image);
    }

    void image_resize_3D(Image*& image, uint32_t width, uint32_t height, uint32_t depth, void* data, bool create_new) {
        if (!image)
            return;
        if (create_new) {
            Image* new_image = new Image{
                .width = width,
                .height = height,
                .depth = depth,
                .format = image->format,
                .usage = image->usage,
                .image_type = image->image_type,
                .view_type = image->view_type,
                .tiling = image->tiling,
                .memory_properties = image->memory_properties,
                .multisampling = image->multisampling,
                .data = image->data
            };

            image_init(new_image);

            image = new_image;
            return;
        }
        image_free_internal(image);
        image_pre_resize_3D_internal(image, width, height, depth);
        image_cpy_data(image, data);
        image_init(image);
    }

    void image_init(Image* image_ptr) {
        auto& image = *image_ptr;
        // VkImageCreateInfoInternal setup
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = image.image_type;
        imageInfo.extent.width = image.width;
        imageInfo.extent.height = image.height;
        imageInfo.extent.depth = image.depth;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = image.format;
        imageInfo.tiling = image.tiling;
        imageInfo.initialLayout = image.current_layout;
        imageInfo.usage = image.usage;
        imageInfo.samples = image.multisampling;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateImage(dr.device, &imageInfo, nullptr, &image.image);

        // Allocate memory
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(dr.device, image.image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, image.memory_properties);

        vkAllocateMemory(dr.device, &allocInfo, nullptr, &image.memory);
        vkBindImageMemory(dr.device, image.image, image.memory, 0);

        // Upload data if provided
        if (image.data)
        {
            upload_image_data(image_ptr);
        }

        // Create ImageView
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image.image;
        viewInfo.viewType = image.view_type;
        viewInfo.format = image.format;
        switch (image.format) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            break;
        case VK_FORMAT_D32_SFLOAT:
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            viewInfo.subresourceRange.aspectMask =
                VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        case VK_FORMAT_R8_UINT:
            viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        default:
            break;
        }
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(dr.device, &viewInfo, nullptr, &image.imageView);

        // Conditionally create sampler if image will be sampled
        if (image.usage & VK_IMAGE_USAGE_SAMPLED_BIT)
        {
            VkSamplerCreateInfo samplerInfo{};
            samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
            samplerInfo.magFilter = VK_FILTER_LINEAR;
            samplerInfo.minFilter = VK_FILTER_LINEAR;
            samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
            samplerInfo.anisotropyEnable = VK_FALSE;
            samplerInfo.maxAnisotropy = 1.0f;
            samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            samplerInfo.unnormalizedCoordinates = VK_FALSE;
            samplerInfo.compareEnable = VK_FALSE;
            samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
            samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
            samplerInfo.mipLodBias = 0.0f;
            samplerInfo.minLod = 0.0f;
            samplerInfo.maxLod = 0.0f;

            vkCreateSampler(dr.device, &samplerInfo, nullptr, &image.sampler);
        }
    }

    void upload_image_data(Image* image_ptr)
    {
        auto& image = *image_ptr;
        VkDeviceSize image_size = image.width * image.height * image.depth * 4; // Assuming 4 bytes per texel (e.g., RGBA8)

        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = image_size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(dr.device, &buffer_info, nullptr, &staging_buffer);

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(dr.device, staging_buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkAllocateMemory(dr.device, &alloc_info, nullptr, &staging_buffer_memory);
        vkBindBufferMemory(dr.device, staging_buffer, staging_buffer_memory, 0);

        void* mapped_data;
        vkMapMemory(dr.device, staging_buffer_memory, 0, image_size, 0, &mapped_data);
        memcpy(mapped_data, image.data.get(), static_cast<size_t>(image_size));
        vkUnmapMemory(dr.device, staging_buffer_memory);

        VkCommandBuffer command_buffer = begin_single_time_commands();

        auto new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        VkImageMemoryBarrier barrier_to_transfer{};
        barrier_to_transfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier_to_transfer.oldLayout = image.current_layout;
        barrier_to_transfer.newLayout = new_layout;
        barrier_to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier_to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier_to_transfer.image = image.image;
        barrier_to_transfer.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier_to_transfer.subresourceRange.baseMipLevel = 0;
        barrier_to_transfer.subresourceRange.levelCount = 1;
        barrier_to_transfer.subresourceRange.baseArrayLayer = 0;
        barrier_to_transfer.subresourceRange.layerCount = 1;
        barrier_to_transfer.srcAccessMask = 0;
        barrier_to_transfer.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        vkCmdPipelineBarrier(
            command_buffer,
            VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier_to_transfer
        );

        image.current_layout = new_layout;

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {image.width, image.height, image.depth};

        vkCmdCopyBufferToImage(command_buffer, staging_buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VkImageMemoryBarrier barrier_to_shader{};
        barrier_to_shader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier_to_shader.oldLayout = image.current_layout;
        new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier_to_shader.newLayout = new_layout;
        barrier_to_shader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier_to_shader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier_to_shader.image = image.image;
        barrier_to_shader.subresourceRange = barrier_to_transfer.subresourceRange;
        barrier_to_shader.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier_to_shader.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            command_buffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier_to_shader
        );

        image.current_layout = new_layout;

        end_single_time_commands(command_buffer);

        vkDestroyBuffer(dr.device, staging_buffer, nullptr);
        vkFreeMemory(dr.device, staging_buffer_memory, nullptr);
    }

    void image_free(Image* image)
    {
        if (!image)
            return;
        auto& device = dr.device;
        if (device == VK_NULL_HANDLE)
            return;
        if (image->image != VK_NULL_HANDLE) {
            vkDestroyImage(device, image->image, 0);
            image->image = nullptr;
        }
        if (image->imageView != VK_NULL_HANDLE) {
            vkDestroyImageView(device, image->imageView, 0);
            image->imageView = nullptr;
        }
        if (image->memory != VK_NULL_HANDLE) {
            vkFreeMemory(device, image->memory, 0);
            image->memory = nullptr;
        }
        if(image->sampler != VK_NULL_HANDLE) {
            vkDestroySampler(device, image->sampler, 0);
            image->sampler = nullptr;
        }
        delete image;
        return;
    }

    std::pair<VkDescriptorSetLayout, VkDescriptorSet> image_create_descriptor_set(Image* image) {
        assert(image && "Image* is null");

        VkDescriptorSetLayoutBinding binding = {};
        binding.binding = 0;
        binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 1;
        layoutInfo.pBindings = &binding;

        VkDescriptorSetLayout layout;
        vkCreateDescriptorSetLayout(dr.device, &layoutInfo, nullptr, &layout);

        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = dr.imguiLayer.DescriptorPool;
        allocInfo.descriptorSetCount = 1;
        allocInfo.pSetLayouts = &layout;

        VkDescriptorSet descriptorSet;
        vkAllocateDescriptorSets(dr.device, &allocInfo, &descriptorSet);

        VkDescriptorImageInfo imageInfo = {};
        imageInfo.imageView = image->imageView;
        imageInfo.sampler = image->sampler;
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        VkWriteDescriptorSet descriptorWrite = {};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = descriptorSet;
        descriptorWrite.dstBinding = 0;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrite.pImageInfo = &imageInfo;

        vkUpdateDescriptorSets(dr.device, 1, &descriptorWrite, 0, nullptr);

        dr.layoutQueue.push(layout);

        return {layout, descriptorSet};
    }
    
    std::vector<float> image_get_channels_size_of_t(Image* image)
    {
        int channels = 0;
        float sizeoftype = 0;

        switch (image->format)
        {
            case VK_FORMAT_R4G4_UNORM_PACK8:
                channels = 2;
                sizeoftype = 0.5;
                break;

            case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
            case VK_FORMAT_A4R4G4B4_UNORM_PACK16_EXT:
            case VK_FORMAT_A4B4G4R4_UNORM_PACK16_EXT:
                channels = 4;
                sizeoftype = 0.5;
                break;

            case VK_FORMAT_R5G6B5_UNORM_PACK16:
            case VK_FORMAT_B5G6R5_UNORM_PACK16:
                channels = 3;
                sizeoftype = 2;
                break;

            case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
            case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
            case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
                channels = 4;
                sizeoftype = 2;
                break;

            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_SRGB:
            case VK_FORMAT_R8_UINT:
            case VK_FORMAT_S8_UINT:
            case VK_FORMAT_R8_SNORM:
            case VK_FORMAT_R8_USCALED:
            case VK_FORMAT_R8_SSCALED:
            case VK_FORMAT_R8_SINT:
                channels = 1;
                sizeoftype = sizeof(uint8_t);
                break;

            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R8G8_SRGB:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R8G8_SINT:
                channels = 2;
                sizeoftype = sizeof(uint8_t);
                break;

            case VK_FORMAT_R8G8B8_UNORM:
            case VK_FORMAT_R8G8B8_UINT:
            case VK_FORMAT_R8G8B8_SRGB:
            case VK_FORMAT_R8G8B8_SNORM:
            case VK_FORMAT_R8G8B8_SINT:
                channels = 3;
                sizeoftype = sizeof(uint8_t);
                break;

            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_R8G8B8A8_SINT:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SRGB:
                channels = 4;
                sizeoftype = sizeof(uint8_t);
                break;

            case VK_FORMAT_R16_UNORM:
            case VK_FORMAT_R16_UINT:
            case VK_FORMAT_R16_SFLOAT:
                channels = 1;
                sizeoftype = sizeof(uint16_t);
                break;

            case VK_FORMAT_R16G16_UNORM:
            case VK_FORMAT_R16G16_UINT:
            case VK_FORMAT_R16G16_SFLOAT:
                channels = 2;
                sizeoftype = sizeof(uint16_t);
                break;

            case VK_FORMAT_R16G16B16_UNORM:
            case VK_FORMAT_R16G16B16_UINT:
            case VK_FORMAT_R16G16B16_SFLOAT:
                channels = 3;
                sizeoftype = sizeof(uint16_t);
                break;

            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                channels = 4;
                sizeoftype = sizeof(uint16_t);
                break;

            case VK_FORMAT_R32_UINT:
            case VK_FORMAT_R32_SFLOAT:
                channels = 1;
                sizeoftype = sizeof(uint32_t);
                break;

            case VK_FORMAT_R32G32_UINT:
            case VK_FORMAT_R32G32_SFLOAT:
                channels = 2;
                sizeoftype = sizeof(uint32_t);
                break;

            case VK_FORMAT_R32G32B32_UINT:
            case VK_FORMAT_R32G32B32_SFLOAT:
                channels = 3;
                sizeoftype = sizeof(uint32_t);
                break;

            case VK_FORMAT_R32G32B32A32_UINT:
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                channels = 4;
                sizeoftype = sizeof(uint32_t);
                break;

            case VK_FORMAT_D16_UNORM:
                channels = 1;
                sizeoftype = sizeof(uint16_t);
                break;

            case VK_FORMAT_D24_UNORM_S8_UINT:
                return { 3, 1 };

            case VK_FORMAT_D32_SFLOAT:
                channels = 1;
                sizeoftype = sizeof(float);
                break;

            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return { sizeof(float), sizeof(uint8_t) };

            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
                return { 4 };

            case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
                return { 4 };

            default:
                break;
        }

        std::vector<float> vec;
        vec.reserve(channels);
        for (int i = 0; i < channels; ++i)
        {
            vec.push_back(sizeoftype);
        }

        return vec;
    }

    void image_upload_data(Image* image, void* data) {
        
    }

    size_t image_get_sizeof_channels(const std::vector<float>& channels) {
        float size = 0;
        for (auto& c : channels)
            size += c;
        return size_t(size);
    }
}