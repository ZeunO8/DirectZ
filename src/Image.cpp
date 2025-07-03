namespace dz {
    struct Image
    {
        VkImage image = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        VkSampler sampler = VK_NULL_HANDLE;
        VkImageLayout current_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    };

    struct ImageCreateInfo
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
        void* data = nullptr;
    };

    void upload_image_data(VkImage image, const ImageCreateInfo& info, void* src_data);

    Image* image_create(const ImageCreateInfo& info)
    {
        auto direct_registry = get_direct_registry();
        Image* result = new Image{};

        // VkImageCreateInfo setup
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = info.image_type;
        imageInfo.extent.width = info.width;
        imageInfo.extent.height = info.height;
        imageInfo.extent.depth = info.depth;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = 1;
        imageInfo.format = info.format;
        imageInfo.tiling = info.tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = info.usage;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateImage(direct_registry->device, &imageInfo, nullptr, &result->image);

        // Allocate memory
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(direct_registry->device, result->image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = find_memory_type(memRequirements.memoryTypeBits, info.memory_properties);

        vkAllocateMemory(direct_registry->device, &allocInfo, nullptr, &result->memory);
        vkBindImageMemory(direct_registry->device, result->image, result->memory, 0);

        // Upload data if provided
        if (info.data)
        {
            upload_image_data(result->image, info, info.data);
        }

        // Create ImageView
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = result->image;
        viewInfo.viewType = info.view_type;
        viewInfo.format = info.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        vkCreateImageView(direct_registry->device, &viewInfo, nullptr, &result->imageView);

        // Conditionally create sampler if image will be sampled
        if (info.usage & VK_IMAGE_USAGE_SAMPLED_BIT)
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

            vkCreateSampler(direct_registry->device, &samplerInfo, nullptr, &result->sampler);
        }

        return result;
    }

    void upload_image_data(VkImage image, const ImageCreateInfo& info, void* src_data)
    {
        auto direct_registry = get_direct_registry();

        VkDeviceSize image_size = info.width * info.height * info.depth * 4; // Assuming 4 bytes per texel (e.g., RGBA8)

        VkBuffer staging_buffer;
        VkDeviceMemory staging_buffer_memory;

        VkBufferCreateInfo buffer_info{};
        buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        buffer_info.size = image_size;
        buffer_info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        vkCreateBuffer(direct_registry->device, &buffer_info, nullptr, &staging_buffer);

        VkMemoryRequirements mem_requirements;
        vkGetBufferMemoryRequirements(direct_registry->device, staging_buffer, &mem_requirements);

        VkMemoryAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        alloc_info.allocationSize = mem_requirements.size;
        alloc_info.memoryTypeIndex = find_memory_type(mem_requirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        vkAllocateMemory(direct_registry->device, &alloc_info, nullptr, &staging_buffer_memory);
        vkBindBufferMemory(direct_registry->device, staging_buffer, staging_buffer_memory, 0);

        void* mapped_data;
        vkMapMemory(direct_registry->device, staging_buffer_memory, 0, image_size, 0, &mapped_data);
        memcpy(mapped_data, src_data, static_cast<size_t>(image_size));
        vkUnmapMemory(direct_registry->device, staging_buffer_memory);

        VkCommandBuffer command_buffer = begin_single_time_commands();

        VkImageMemoryBarrier barrier_to_transfer{};
        barrier_to_transfer.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier_to_transfer.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier_to_transfer.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier_to_transfer.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier_to_transfer.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier_to_transfer.image = image;
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

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {info.width, info.height, info.depth};

        vkCmdCopyBufferToImage(command_buffer, staging_buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        VkImageMemoryBarrier barrier_to_shader{};
        barrier_to_shader.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier_to_shader.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier_to_shader.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier_to_shader.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier_to_shader.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier_to_shader.image = image;
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

        end_single_time_commands(command_buffer);

        vkDestroyBuffer(direct_registry->device, staging_buffer, nullptr);
        vkFreeMemory(direct_registry->device, staging_buffer_memory, nullptr);
    }

    void image_free(Image* image)
    {
        auto direct_registry = get_direct_registry();
        auto& device = direct_registry->device;
        if (device == VK_NULL_HANDLE)
            return;
        if (image->image != VK_NULL_HANDLE)
            vkDestroyImage(device, image->image, 0);
        if (image->imageView != VK_NULL_HANDLE)
            vkDestroyImageView(device, image->imageView, 0);
        if (image->memory != VK_NULL_HANDLE)
            vkFreeMemory(device, image->memory, 0);
        if(image->sampler != VK_NULL_HANDLE)
            vkDestroySampler(device, image->sampler, 0);
        delete image;
    }
}