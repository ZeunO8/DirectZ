#include <dz/Image.hpp>
#include "Directz.cpp.hpp"
#include "Image.cpp.hpp"

namespace dz {
    
    void image_pre_resize_2D_internal(Image* image_ptr, uint32_t width, uint32_t height) {
        auto& image = *image_ptr;
        image.width = width;
        image.height = height;
        image.reset_layouts();
    }

    void image_pre_resize_3D_internal(Image* image_ptr, uint32_t width, uint32_t height, uint32_t depth) {
        auto& image = *image_ptr;
        image.width = width;
        image.height = height;
        image.depth = depth;
        image.reset_layouts();
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
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB:
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
            .is_framebuffer_attachment = info.is_framebuffer_attachment,
            .datas = info.datas,
            .surfaceType = info.surfaceType,
            .mip_levels = info.mip_levels
        };
        return image_create_internal(internal_info);
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
            .multisampling = info.multisampling,
            .is_framebuffer_attachment = info.is_framebuffer_attachment,
            .datas = info.datas,
            .surfaceType = info.surfaceType,
            .mip_levels = info.mip_levels
        };

        image_init(result);

        return result;
    }

    void image_free_internal(Image* image_ptr);

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
                .datas = image->datas,
                .surfaceType = image->surfaceType,
                .mip_levels = image->mip_levels
            };

            image_init(new_image);

            image = new_image;
            return;
        }
        image_free_internal(image);
        image_pre_resize_2D_internal(image, width, height);
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
                .datas = image->datas,
                .surfaceType = image->surfaceType,
                .mip_levels = image->mip_levels
            };

            image_init(new_image);

            image = new_image;
            return;
        }
        image_free_internal(image);
        image_pre_resize_3D_internal(image, width, height, depth);
        image_init(image);
    }

    void image_init(Image* image_ptr) {
        auto& image = *image_ptr;
        image.reset_layouts();
        // VkImageCreateInfoInternal setup
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = image.image_type;
        imageInfo.extent.width = image.width;
        imageInfo.extent.height = image.height;
        imageInfo.extent.depth = image.depth;
        imageInfo.mipLevels = image.mip_levels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = image.format;
        imageInfo.tiling = image.tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
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

        image.datas.resize(image.mip_levels);
        for (auto mip = 0; mip < image.mip_levels; mip++) {
            // Upload data if provided
            if (!image.datas[mip]) {
                init_empty_image_data(image_ptr, mip);
            }
            image_upload_data(image_ptr, mip);
            image.datas[mip].reset();
        }
        image_ptr->data_is_cpu_side = false;

        // Create ImageView
        image.imageViews.resize(image.mip_levels);
        for (auto mip = 0; mip < image.mip_levels; ++mip) {
            VkImageViewCreateInfo mipViewInfo{
                .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image = image.image,
                .viewType = image.view_type,
                .format = image.format,
                .subresourceRange = {
                    .aspectMask = image_get_aspect_mask(image_ptr),
                    .baseMipLevel = uint32_t(mip),
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1
                }
            };

            vkCreateImageView(dr.device, &mipViewInfo, nullptr, &image.imageViews[mip]);
        }
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
            samplerInfo.maxLod = float(image.mip_levels);

            vkCreateSampler(dr.device, &samplerInfo, nullptr, &image.sampler);
        }
    }
    
    void init_empty_image_data(Image* image_ptr, uint32_t mip) {
        auto& image = *image_ptr;
        auto channels = image_get_channels_size_of_t(image_ptr);
        auto pixel_stride = image_get_sizeof_channels(channels);
        uint32_t mipWidth = (std::max)(1u, image.width >> mip);
        uint32_t mipHeight = (std::max)(1u, image.height >> mip);
        auto image_size = mipWidth * mipHeight * image.depth * pixel_stride;
        auto& ptr = image.datas[mip];
        ptr = std::shared_ptr<char>((char*)malloc(image_size), free);
        memset(ptr.get(), 0, image_size);
    }

    uint32_t image_get_aspect_mask(Image* image_ptr) {
        switch (image_ptr->format) {
        case VK_FORMAT_R8_UNORM:
        case VK_FORMAT_R8G8_UNORM:
        case VK_FORMAT_R8G8B8_UNORM:
        case VK_FORMAT_R8G8B8A8_UNORM:
        case VK_FORMAT_R8_SRGB:
        case VK_FORMAT_R8G8_SRGB:
        case VK_FORMAT_R8G8B8_SRGB:
        case VK_FORMAT_R8G8B8A8_SRGB:
        case VK_FORMAT_R16_SFLOAT:
        case VK_FORMAT_R16G16_SFLOAT:
        case VK_FORMAT_R16G16B16_SFLOAT:
        case VK_FORMAT_R16G16B16A16_SFLOAT:
        case VK_FORMAT_R32_SFLOAT:
        case VK_FORMAT_R32G32_SFLOAT:
        case VK_FORMAT_R32G32B32_SFLOAT:
        case VK_FORMAT_R32G32B32A32_SFLOAT:
            return VK_IMAGE_ASPECT_COLOR_BIT;
            break;
        case VK_FORMAT_D32_SFLOAT:
            return VK_IMAGE_ASPECT_DEPTH_BIT;
            break;
        case VK_FORMAT_D32_SFLOAT_S8_UINT:
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        case VK_FORMAT_R8_UINT:
            return VK_IMAGE_ASPECT_STENCIL_BIT;
            break;
        default:
            break;
        }
        return 0;
    }

    void image_upload_data(Image* image_ptr, uint32_t mip, void* new_data)
    {
        auto& image = *image_ptr;
        auto channels = image_get_channels_size_of_t(image_ptr);
        auto pixel_stride = image_get_sizeof_channels(channels);
        auto image_mip_width = (std::max)(1u, uint32_t(image.width) >> mip);
        auto image_mip_height = (std::max)(1u, uint32_t(image.height) >> mip);
        auto image_mip_depth = (std::max)(1u, uint32_t(image.depth) >> mip);
        VkDeviceSize image_size = image_mip_width * image_mip_height * image_mip_depth * pixel_stride;

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

        auto& sh_ptr = image.datas[mip];
        auto ptr = sh_ptr.get();
        if (new_data) {
            memcpy(new_data, ptr, image_size);
        }

        void* mapped_data;
        vkMapMemory(dr.device, staging_buffer_memory, 0, image_size, 0, &mapped_data);
        memcpy(mapped_data, ptr, static_cast<size_t>(image_size));
        vkUnmapMemory(dr.device, staging_buffer_memory);

        VkCommandBuffer command_buffer = begin_single_time_commands();

        auto new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        auto& current_layout = image.current_layouts[mip];

        VkImageMemoryBarrier barrier_to_transfer{};
        barrier_to_transfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier_to_transfer.oldLayout = current_layout;
        barrier_to_transfer.newLayout = new_layout;
        barrier_to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier_to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier_to_transfer.image = image.image;
        barrier_to_transfer.subresourceRange.aspectMask = image_get_aspect_mask(image_ptr);
        barrier_to_transfer.subresourceRange.baseMipLevel = mip;
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

        current_layout = new_layout;

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = image_get_aspect_mask(image_ptr);
        region.imageSubresource.mipLevel = mip;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {image_mip_width, image_mip_height, image_mip_depth};

        vkCmdCopyBufferToImage(command_buffer, staging_buffer, image.image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VkImageMemoryBarrier barrier_to_shader{};
        barrier_to_shader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier_to_shader.oldLayout = current_layout;
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

        current_layout = new_layout;

        end_single_time_commands(command_buffer);

        vkDestroyBuffer(dr.device, staging_buffer, nullptr);
        vkFreeMemory(dr.device, staging_buffer_memory, nullptr);

        image.data_is_gpu_side = true;
    }
    
    void image_free_internal(Image* image_ptr) {
        if (!image_ptr)
            return;
        auto& image = *image_ptr;
        auto& device = dr.device;
        if (device == VK_NULL_HANDLE)
            return;
        if (image.image != VK_NULL_HANDLE) {
            vkDestroyImage(device, image.image, 0);
            image.image = nullptr;
        }
        for (auto& imageView : image.imageViews) {
            if (!imageView)
                continue;
            vkDestroyImageView(device, imageView, 0);
            imageView = nullptr;
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

    void image_free(Image* image_ptr)
    {
        image_free_internal(image_ptr);
        delete image_ptr;
        return;
    }

    std::pair<VkDescriptorSetLayout, VkDescriptorSet> image_create_descriptor_set(Image* image, uint32_t mip_level) {
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
        imageInfo.imageView = image->imageViews[mip_level];
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

    std::vector<float> format_get_channels_size_of_t(VkFormat format) {
        int channels = 0;
        float sizeoftype = 0;

        switch (format)
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
    
    std::vector<float> image_get_channels_size_of_t(Image* image)
    {
        return format_get_channels_size_of_t(image->format);
    }

    size_t image_get_sizeof_channels(const std::vector<float>& channels) {
        float size = 0;
        for (auto& c : channels)
            size += c;
        return size_t(size);
    }

    SurfaceType image_get_surface_type(Image* image_ptr) {
        return image_ptr->surfaceType;
    }

    uint32_t image_get_width(Image* image_ptr) {
        return image_ptr->width;
    }

    uint32_t image_get_height(Image* image_ptr) {
        return image_ptr->height;
    }

    uint32_t image_get_depth(Image* image_ptr) {
        return image_ptr->depth;
    }

    VkImageLayout image_get_layout(Image* image_ptr, int mip) {
        return image_ptr->current_layouts[mip];
    }

    VkFormat image_get_format(Image* image_ptr) {
        return image_ptr->format;
    }

    size_t get_format_pixel_size(VkFormat format) {
        switch (format)
        {
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_SRGB:
            case VK_FORMAT_R8_UINT:
            case VK_FORMAT_R8_SNORM:
            case VK_FORMAT_R8_SINT:
            case VK_FORMAT_S8_UINT:
                return 1;

            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_SRGB:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R8G8_SINT:
                return 2;

            case VK_FORMAT_R8G8B8_UNORM:
            case VK_FORMAT_R8G8B8_SRGB:
            case VK_FORMAT_R8G8B8_UINT:
            case VK_FORMAT_R8G8B8_SNORM:
            case VK_FORMAT_R8G8B8_SINT:
                return 3;

            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_R8G8B8A8_SINT:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_R8G8B8A8_SRGB:
                return 4;

            case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_R16G16B16A16_SINT:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
                return 8;

            case VK_FORMAT_R32G32B32A32_UINT:
            case VK_FORMAT_R32G32B32A32_SINT:
            case VK_FORMAT_R32G32B32A32_SFLOAT:
                return 16;

            case VK_FORMAT_D32_SFLOAT:
                return 4;

            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return 5;

            case VK_FORMAT_R32_UINT:
                return 4;

            case VK_FORMAT_R5G6B5_UNORM_PACK16:
            case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
                return 2;

            default:
                throw std::runtime_error("Unsupported VkFormat"); // fallback
        }
    }

    void image_copy_begin() {
        static VkCommandBufferBeginInfo beginInfo{
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT
        };

        vkBeginCommandBuffer(dr.copyCommandBuffer, &beginInfo);

        dr.copyRegions.clear();
        dr.copySrcImages.clear();
        dr.copyDstImages.clear();
    }

    void image_copy_reserve_regions(uint32_t count) {
        dr.copyRegions.reserve(count);
        dr.copySrcImages.reserve(count);
        dr.copyDstImages.reserve(count);
    }

    void image_copy_image(Image* dstImage, Image* srcImage, VkImageCopy region) {
        auto index = dr.copyRegions.size();
        dr.copyRegions.push_back(region);
        auto dst_mip = region.dstSubresource.mipLevel;
        auto src_mip = region.srcSubresource.mipLevel;
        auto& dst_current_layout = dstImage->current_layouts[dst_mip];
        auto& src_current_layout = srcImage->current_layouts[src_mip];
        auto copy_dst_it = std::find_if(dr.copyDstImages.begin(), dr.copyDstImages.end(), [&](auto& tuple) {
            auto& image_ptr = std::get<0>(tuple);
            auto mip = std::get<2>(tuple);
            return image_ptr == dstImage && mip == dst_mip;
        });
        auto copy_src_it = std::find_if(dr.copySrcImages.begin(), dr.copySrcImages.end(), [&](auto& tuple) {
            auto& image_ptr = std::get<0>(tuple);
            auto mip = std::get<2>(tuple);
            return image_ptr == srcImage && mip == src_mip;
        });
        auto dst_original_layout = copy_dst_it != dr.copyDstImages.end() ? std::get<1>(*copy_dst_it) : dst_current_layout;
        auto src_original_layout = copy_src_it != dr.copySrcImages.end() ? std::get<1>(*copy_src_it) : src_current_layout;
        dr.copyDstImages.push_back({dstImage, dst_original_layout, dst_mip});
        dr.copySrcImages.push_back({srcImage, src_original_layout, src_mip});
        static auto src_new_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        static auto dst_new_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        if (dst_current_layout != dst_new_layout)
            transition_image_layout(dstImage, dst_new_layout, dst_mip);
        if (src_current_layout != src_new_layout)
            transition_image_layout(srcImage, src_new_layout, src_mip);
        vkCmdCopyImage(
            dr.copyCommandBuffer,
            srcImage->image,
            src_new_layout,
            dstImage->image,
            dst_new_layout,
            1,
            dr.copyRegions.data() + index
        );
    }

    void image_copy_end() {
        vkEndCommandBuffer(dr.copyCommandBuffer);

        static VkSubmitInfo submitInfo{
            .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1
        };
        submitInfo.pCommandBuffers = &dr.copyCommandBuffer;

        vkQueueSubmit(dr.copyQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(dr.copyQueue);

        for (auto& [image_ptr, original_layout, original_mip] : dr.copySrcImages) {
            if (image_ptr->current_layouts[original_mip] != original_layout)
                transition_image_layout(image_ptr, original_layout, original_mip);
        }

        for (auto& [image_ptr, original_layout, original_mip] : dr.copyDstImages) {
            if (image_ptr->current_layouts[original_mip] != original_layout)
                transition_image_layout(image_ptr, original_layout, original_mip);
        }

        dr.copyRegions.clear();
        dr.copySrcImages.clear();
        dr.copyDstImages.clear();
    }

    bool serialize_ImageCreateInfo(Serial& serial, const ImageCreateInfo& info) {
        serial << info.width << info.height << info.depth
               << info.format << info.usage << info.image_type
               << info.view_type << info.tiling << info.memory_properties
               << info.multisampling << info.is_framebuffer_attachment
               << info.surfaceType << info.mip_levels;
        auto channels = format_get_channels_size_of_t(info.format);
        auto pixel_stride = image_get_sizeof_channels(channels);
        assert(info.datas.size() == info.mip_levels);
        auto info_datas_data = info.datas.data();
        auto mip = 0;
        for (auto& data_ptr : info.datas) {
            uint32_t mipWidth = (std::max)(1u, info.width >> mip);
            uint32_t mipHeight = (std::max)(1u, info.height >> mip);
            uint32_t mipDepth = (std::max)(1u, info.depth >> mip);
            auto mip_byte_size = mipWidth * mipHeight * mipDepth * pixel_stride;
            auto& bytes = info_datas_data[mip];
            serial.writeBytes((char*)(bytes.get()), mip_byte_size);
            mip++;
        }
        return true;
    }

    ImageCreateInfo deserialize_ImageCreateInfo(Serial& serial) {
        ImageCreateInfo info;
        serial >> info.width >> info.height >> info.depth
               >> info.format >> info.usage >> info.image_type
               >> info.view_type >> info.tiling >> info.memory_properties
               >> info.multisampling >> info.is_framebuffer_attachment
               >> info.surfaceType >> info.mip_levels;
        auto channels = format_get_channels_size_of_t(info.format);
        auto pixel_stride = image_get_sizeof_channels(channels);
        info.datas.resize(info.mip_levels);
        auto info_datas_data = info.datas.data();
        auto mip = 0;
        for (auto& data_ptr : info.datas) {
            uint32_t mipWidth = (std::max)(1u, info.width >> mip);
            uint32_t mipHeight = (std::max)(1u, info.height >> mip);
            uint32_t mipDepth = (std::max)(1u, info.depth >> mip);
            auto mip_byte_size = mipWidth * mipHeight * mipDepth * pixel_stride;
            auto& bytes = (info_datas_data[mip] = std::shared_ptr<void>(malloc(mip_byte_size), free));
            serial.readBytes((char*)(bytes.get()), mip_byte_size);
            mip++;
        }
        return info;
    }

    bool image_serialize(Image* image_ptr, Serial& serial) {
        auto info = image_to_info(image_ptr);
        return serialize_ImageCreateInfo(serial, info);
    }

    Image* image_from_serial(Serial& serial) {
        auto info = deserialize_ImageCreateInfo(serial);
        return image_create(info);
    }

    ImageCreateInfo image_to_info(Image* image_ptr) {
        ImageCreateInfo info{
            .width = image_ptr->width,
            .height = image_ptr->height,
            .depth = image_ptr->depth,
            .format = image_ptr->format,
            .usage = image_ptr->usage,
            .image_type = image_ptr->image_type,
            .view_type = image_ptr->view_type,
            .tiling = image_ptr->tiling,
            .memory_properties = image_ptr->memory_properties,
            .multisampling = image_ptr->multisampling,
            .is_framebuffer_attachment = image_ptr->is_framebuffer_attachment,
            .surfaceType = image_ptr->surfaceType,
            .mip_levels = image_ptr->mip_levels
        };
        info.datas.reserve(info.mip_levels);
        for (auto mip = 0; mip < info.mip_levels; mip++) {
            auto mip_data = image_get_data(image_ptr, mip);
            info.datas.push_back(std::shared_ptr<void>(mip_data, image_free_copied_data));
        }
        return info;
    }

    void* image_get_data(Image* image_ptr, int mip)
    {
        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingBufferMemory = VK_NULL_HANDLE;
        VkMemoryRequirements imageMemRequirements;
        vkGetImageMemoryRequirements(dr.device, image_ptr->image, &imageMemRequirements);
        VkDeviceSize bufferSize = imageMemRequirements.size;
        char* imageData = (char*)malloc(bufferSize);

        // 2. Create staging buffer
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = bufferSize;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT; // We copy from image TO this buffer
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        vk_check("vkCreateBuffer", vkCreateBuffer(dr.device, &bufferInfo, nullptr, &stagingBuffer));

        // 3. Allocate memory for staging buffer
        VkMemoryRequirements stagingMemRequirements;
        vkGetBufferMemoryRequirements(dr.device, stagingBuffer, &stagingMemRequirements);
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = stagingMemRequirements.size; // Use actual requirements for staging buffer
        allocInfo.memoryTypeIndex = find_memory_type(stagingMemRequirements.memoryTypeBits,
                VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
        vk_check("vkAllocateMemory", vkAllocateMemory(dr.device, &allocInfo, nullptr, &stagingBufferMemory));

        // 4. Bind staging buffer memory
        vk_check("vkBindBufferMemory", vkBindBufferMemory(dr.device, stagingBuffer, stagingBufferMemory, 0));

        // 5. Record and execute copy commands
        VkCommandBuffer commandBuffer = begin_single_time_commands();

        auto aspect_mask = image_get_aspect_mask(image_ptr);

        auto& current_layout = image_ptr->current_layouts[mip];
        auto originalLayout = current_layout;
        auto transferLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;

        VkImageMemoryBarrier copyBarrier{};
        copyBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        copyBarrier.oldLayout = current_layout;
        copyBarrier.newLayout = transferLayout;
        copyBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        copyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copyBarrier.image = image_ptr->image;
        copyBarrier.subresourceRange.aspectMask = aspect_mask;
        copyBarrier.subresourceRange.baseMipLevel = mip;
        copyBarrier.subresourceRange.levelCount = 1;
        copyBarrier.subresourceRange.baseArrayLayer = 0;
        copyBarrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &copyBarrier
        );

        current_layout = transferLayout;

        transferLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

        copyBarrier.oldLayout = current_layout;
        copyBarrier.newLayout = transferLayout;
        copyBarrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        copyBarrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
        copyBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copyBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        copyBarrier.image = image_ptr->image;
        copyBarrier.subresourceRange.aspectMask = aspect_mask;
        copyBarrier.subresourceRange.baseMipLevel = mip;
        copyBarrier.subresourceRange.levelCount = 1;
        copyBarrier.subresourceRange.baseArrayLayer = 0;
        copyBarrier.subresourceRange.layerCount = 1;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
            VK_PIPELINE_STAGE_TRANSFER_BIT,
            0,
            0, nullptr,
            0, nullptr,
            1, &copyBarrier
        );

        current_layout = transferLayout;

        // Setup the copy region for base mip level, all array layers
        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;   // 0 indicates tightly packed
        region.bufferImageHeight = 0; // 0 indicates tightly packed
        region.imageSubresource.aspectMask = aspect_mask;
        region.imageSubresource.mipLevel = mip;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {uint32_t(image_ptr->width), uint32_t(image_ptr->height), 1};

        vkCmdCopyImageToBuffer(commandBuffer, image_ptr->image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                                stagingBuffer, 1, &region);

                                
        end_single_time_commands(commandBuffer);
        commandBuffer = VK_NULL_HANDLE;

        transition_image_layout(image_ptr, originalLayout, mip);
        
        void* mappedMemory = nullptr;
        vk_check("vkMapMemory", vkMapMemory(dr.device, stagingBufferMemory, 0, bufferSize, 0, &mappedMemory));

        memcpy(imageData, mappedMemory, bufferSize);

        vkUnmapMemory(dr.device, stagingBufferMemory);
        vkFreeMemory(dr.device, stagingBufferMemory, 0);
        vkDestroyBuffer(dr.device, stagingBuffer, 0);
        mappedMemory = nullptr;
        return imageData;
    }
    void image_free_copied_data(void* ptr)
    {
        free(ptr);
    }
}