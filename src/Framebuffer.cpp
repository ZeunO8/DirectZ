#include <dz/Framebuffer.hpp>
#include <DirectZ.hpp>
#include "Directz.cpp.hpp"
#include "Framebuffer.cpp.hpp"
#include "Image.cpp.hpp"

namespace dz {

    void transitionDepthLayoutForWriting(Framebuffer&);
    void transitionDepthLayoutForReading(Framebuffer&);
    void transitionColorLayoutForWriting(Framebuffer&);
    void transitionColorLayoutForReading(Framebuffer&);
    void transitionColorResolveLayoutForWriting(Framebuffer&);
    void transitionColorResolveLayoutForReading(Framebuffer&);
    void transitionDepthResolveLayoutForWriting(Framebuffer&);
    void transitionDepthResolveLayoutForReading(Framebuffer&);

    Image* getDepthImage(const Framebuffer&);
    Image* getColorImage(const Framebuffer&);
    Image* getColorResolveImage(const Framebuffer&);
    Image* getDepthResolveImage(const Framebuffer&);

    bool hasColorResolveAttachment(const Framebuffer&);

    Framebuffer* framebuffer_create(const FramebufferInfo& info) {
        auto copy_pImages = (Image**)malloc(info.imagesCount * sizeof(Image*));
        memcpy(copy_pImages, info.pImages, info.imagesCount * sizeof(Image*));
    
        auto copy_pAttachmentTypes = (AttachmentType*)malloc(info.attachmentTypesCount * sizeof(AttachmentType));
        memcpy(copy_pAttachmentTypes, info.pAttachmentTypes, info.attachmentTypesCount * sizeof(AttachmentType));

        auto framebuffer_ptr = new Framebuffer{
            .pImages = copy_pImages,
            .imagesCount = info.imagesCount,
            .pAttachmentTypes = copy_pAttachmentTypes,
            .attachmentTypesCount = info.attachmentTypesCount,
            .blendState = info.blendState,
            .own_images = info.own_images
        };
        auto& framebuffer = *framebuffer_ptr;

        VkCommandBufferAllocateInfo allocInfo = {};

        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = dr.commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        vk_check("vkAllocateCommandBuffers",
            vkAllocateCommandBuffers(dr.device, &allocInfo, &framebuffer.commandBuffer));

        VkEventCreateInfo eventCreateInfo{};

        eventCreateInfo.sType = VK_STRUCTURE_TYPE_EVENT_CREATE_INFO;
        eventCreateInfo.flags = 0;

        vkCreateEvent(dr.device, &eventCreateInfo, 0, &framebuffer.event);

        std::vector<UsingAttachmentDescription> clearAttachments;
        std::vector<UsingAttachmentReference> clearColorAttachmentRefs;
        std::vector<UsingAttachmentReference> clearResolveAttachmentRefs;
        
        std::vector<UsingAttachmentDescription> loadAttachments;
        std::vector<UsingAttachmentReference> loadColorAttachmentRefs;
        std::vector<UsingAttachmentReference> loadResolveAttachmentRefs;
        UsingAttachmentReference depthStencilRef{};
#ifdef USING_VULKAN_1_2
        VkSubpassDescriptionDepthStencilResolve subpassDepthStencilResolve{};
        subpassDepthStencilResolve.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_DEPTH_STENCIL_RESOLVE;
#endif
        bool requiresDepthResolve = false;
        UsingAttachmentReference depthResolveRef{};
#ifdef USING_VULKAN_1_2
        depthStencilRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
#endif
        bool hasDepthStencil = false;
        uint32_t attachmentIndex = 0;
        VkPipelineStageFlags inputSrcStageMask = 0;
        VkPipelineStageFlags inputDstStageMask = 0;
        VkAccessFlags inputDstAccessMask = 0;
        VkPipelineStageFlags outputSrcStageMask = 0;
        VkPipelineStageFlags outputDstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
        VkAccessFlags outputSrcAccessMask = 0;
        VkAccessFlags outputDstAccessMask = 0;
        std::vector<VkImageView> vkImageViews;
        vkImageViews.reserve(framebuffer.attachmentTypesCount);
        for (
            auto attachment_index = 0;
            attachment_index < framebuffer.imagesCount &&
            attachment_index < framebuffer.attachmentTypesCount;
            attachment_index++
        ) {
            auto& image = *framebuffer.pImages[attachment_index];
            auto& attachmentType = framebuffer.pAttachmentTypes[attachment_index];
            
            assert(image.mip_levels == 1);

            auto& imageView = image.imageViews[0];
            if (imageView == VK_NULL_HANDLE)
            {
                throw std::runtime_error("Framebuffer attachment texture image view is null!");
            }

            vkImageViews.push_back(imageView);

            UsingAttachmentDescription clearAttachment{};
            UsingAttachmentDescription loadAttachment{};

#if USING_VULKAN_1_2
            loadAttachment.sType = clearAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
#endif
            clearAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
            loadAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
            loadAttachment.storeOp = clearAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
            loadAttachment.stencilLoadOp = clearAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
            loadAttachment.stencilStoreOp = clearAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;

            if (framebuffer.width != image.width || framebuffer.height != image.height)
            {
                framebuffer.width = image.width;
                framebuffer.height = image.height;
            }

            VkImageLayout subpassLayout = VK_IMAGE_LAYOUT_UNDEFINED;

            loadAttachment.format = clearAttachment.format = image.format;
            
            if (clearAttachment.format == VK_FORMAT_UNDEFINED)
                throw std::runtime_error("Attachment format is undefined!");
            switch (attachmentType)
            {
            case AttachmentType::ColorResolve:
            {
                loadAttachment.samples = clearAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                loadAttachment.initialLayout = clearAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                loadAttachment.finalLayout = clearAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                subpassLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                UsingAttachmentReference _ref_{};
#ifdef USING_VULKAN_1_2
                _ref_.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
#endif
                _ref_.attachment = attachmentIndex;
                _ref_.layout = subpassLayout;
                clearResolveAttachmentRefs.push_back(_ref_);
                break;
            }
            case AttachmentType::DepthResolve:
            {
                loadAttachment.samples = clearAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
                loadAttachment.initialLayout = clearAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
                loadAttachment.finalLayout = clearAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
                subpassLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
#ifdef USING_VULKAN_1_2
                depthResolveRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
#endif
                depthResolveRef.attachment = attachmentIndex;
                depthResolveRef.layout = subpassLayout;
#ifdef USING_VULKAN_1_2
                subpassDepthStencilResolve.depthResolveMode = VK_RESOLVE_MODE_AVERAGE_BIT;
                subpassDepthStencilResolve.pDepthStencilResolveAttachment = &depthResolveRef;
#endif
                requiresDepthResolve = true;
                break;
            }
            case AttachmentType::Color:
            {
                loadAttachment.samples = clearAttachment.samples = image.multisampling;
                // if (loadAttachment.samples = clearAttachment.samples > maxMSAASamples)
                //     loadAttachment.samples = clearAttachment.samples = maxMSAASamples;
                loadAttachment.initialLayout = clearAttachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                loadAttachment.finalLayout = clearAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                subpassLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
                UsingAttachmentReference _ref_{};
#ifdef USING_VULKAN_1_2
                _ref_.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
#endif
                _ref_.attachment = attachmentIndex;
                _ref_.layout = subpassLayout;
                clearColorAttachmentRefs.push_back(_ref_);
                inputSrcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                inputDstStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                inputDstAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
                outputSrcStageMask |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
                outputSrcAccessMask |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
                outputDstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
                outputDstAccessMask = VK_ACCESS_SHADER_READ_BIT;
                break;
            }

            case AttachmentType::Depth:
            case AttachmentType::DepthStencil:
            case AttachmentType::Stencil:
            {
                if (hasDepthStencil)
                    throw std::runtime_error("Framebuffer cannot have multiple depth/stencil attachments!");
                loadAttachment.samples = clearAttachment.samples = image.multisampling;
                // if (loadAttachment.samples = clearAttachment.samples > maxMSAASamples)
                //     loadAttachment.samples = clearAttachment.samples = maxMSAASamples;
                loadAttachment.initialLayout = clearAttachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                loadAttachment.finalLayout = clearAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;//(framebuffer.hasDepthResolveAttachment() ? VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                subpassLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
                if (attachmentType != AttachmentType::Depth)
                {
                    loadAttachment.stencilLoadOp = clearAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
                    loadAttachment.stencilStoreOp = clearAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
                }
                UsingAttachmentReference _ref_{};
#ifdef USING_VULKAN_1_2
                _ref_.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
#endif
                _ref_.attachment = attachmentIndex;
                _ref_.layout = subpassLayout;
                depthStencilRef = _ref_;
                hasDepthStencil = true;
                inputSrcStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                inputDstStageMask |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                inputDstAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
                outputSrcStageMask |= VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
                outputSrcAccessMask |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                // Adjust output dependency if final layout is ATTACHMENT_OPTIMAL
                // Next stage could still be early fragment tests if reused immediately
                outputDstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
                outputDstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
                break;
            }

            default:
                throw std::runtime_error("Unsupported attachment type");
            }
            clearAttachments.push_back(clearAttachment);
            loadAttachments.push_back(loadAttachment);

            attachmentIndex++;
        }

        // --- Define Subpass ---
        UsingSubpassDescription subpass{};
#ifdef USING_VULKAN_1_2
        subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
#endif
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = static_cast<uint32_t>(clearColorAttachmentRefs.size());
        subpass.pColorAttachments = clearColorAttachmentRefs.empty() ? nullptr : clearColorAttachmentRefs.data();
        subpass.pDepthStencilAttachment = hasDepthStencil ? &depthStencilRef : nullptr;
        subpass.pResolveAttachments = clearResolveAttachmentRefs.data();
#ifdef USING_VULKAN_1_2
        if (requiresDepthResolve)
            subpass.pNext = &subpassDepthStencilResolve;
#endif

        // --- Define Dependencies ---
        std::array<UsingSubpassDependency, 2> dependencies;
        memset(dependencies.data(), 0, sizeof(UsingSubpassDependency) * 2);
        // Input Dependency (External -> 0)
#ifdef USING_VULKAN_1_2
        dependencies[0].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
#endif
        dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[0].dstSubpass = 0;
        dependencies[0].srcStageMask = inputSrcStageMask;
        dependencies[0].dstStageMask = inputDstStageMask;
        dependencies[0].srcAccessMask = 0;
        dependencies[0].dstAccessMask = inputDstAccessMask;
        dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
        // Output Dependency (0 -> External)
#ifdef USING_VULKAN_1_2
        dependencies[1].sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
#endif
        dependencies[1].srcSubpass = 0;
        dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
        dependencies[1].srcStageMask = outputSrcStageMask;
        dependencies[1].dstStageMask = outputDstStageMask; // Updated based on finalLayout
        dependencies[1].srcAccessMask = outputSrcAccessMask;
        dependencies[1].dstAccessMask = outputDstAccessMask; // Updated based on finalLayout
        dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;
    
        UsingRenderPassCreateInfo clearRenderPassInfo{};
#ifdef USING_VULKAN_1_2
        clearRenderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
#endif
        clearRenderPassInfo.attachmentCount = static_cast<uint32_t>(clearAttachments.size());
        clearRenderPassInfo.pAttachments = clearAttachments.data();
        clearRenderPassInfo.subpassCount = 1;
        clearRenderPassInfo.pSubpasses = &subpass;
        clearRenderPassInfo.dependencyCount = static_cast<uint32_t>(dependencies.size());
        clearRenderPassInfo.pDependencies = dependencies.data();

#ifdef USING_VULKAN_1_2
        vk_check("vkCreateRenderPass2",
            vkCreateRenderPass2(dr.device, &clearRenderPassInfo, nullptr, &framebuffer.clearRenderPass));
#else
        vk_check("vkCreateRenderPass",
            vkCreateRenderPass(dr.device, &clearRenderPassInfo, nullptr, &framebuffer.clearRenderPass));
#endif
        
        auto loadRenderPassInfo = clearRenderPassInfo;
        loadRenderPassInfo.pAttachments = loadAttachments.data();

#ifdef USING_VULKAN_1_2
        vk_check("vkCreateRenderPass2",
            vkCreateRenderPass2(dr.device, &loadRenderPassInfo, nullptr, &framebuffer.loadRenderPass));
#else
        vk_check("vkCreateRenderPass",
            vkCreateRenderPass(dr.device, &loadRenderPassInfo, nullptr, &framebuffer.loadRenderPass));
#endif

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = framebuffer.clearRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(vkImageViews.size());
        framebufferInfo.pAttachments = vkImageViews.data();
        framebufferInfo.width = framebuffer.width;
        framebufferInfo.height = framebuffer.height;
        framebufferInfo.layers = 1;

        vk_check("vkCreateFramebuffer",
            vkCreateFramebuffer(dr.device, &framebufferInfo, nullptr, &framebuffer.framebuffer));

        framebuffer_set_clear_color(framebuffer_ptr);
        framebuffer_set_clear_depth_stencil(framebuffer_ptr);

        return framebuffer_ptr;
    }
    
    void framebuffer_set_clear_color(Framebuffer* framebuffer_ptr, float r, float g, float b, float a) {
        if (!framebuffer_ptr)
            return;
        auto& framebuffer = *framebuffer_ptr;
        framebuffer.clear_color.float32[0] = r;
        framebuffer.clear_color.float32[1] = g;
        framebuffer.clear_color.float32[2] = b;
        framebuffer.clear_color.float32[3] = a;
        framebuffer.clear_changed = true;
    }

    void framebuffer_set_clear_depth_stencil(Framebuffer* framebuffer_ptr, float depth, uint32_t stencil) {
        if (!framebuffer_ptr)
            return;
        auto& framebuffer = *framebuffer_ptr;
        framebuffer.clear_depth_stencil.depth = depth;
        framebuffer.clear_depth_stencil.stencil = stencil;
        framebuffer.clear_changed = true;
    }

    void framebuffer_bind(Framebuffer* framebuffer_ptr, bool clear) {
        if (!framebuffer_ptr) {
            return;
        }

        auto& framebuffer = *framebuffer_ptr;

        if (framebuffer.new_pImages) {
            assert(framebuffer.new_framebuffer);
            vkDestroyFramebuffer(dr.device, framebuffer.framebuffer, 0);
            framebuffer.framebuffer = framebuffer.new_framebuffer;
            framebuffer.new_framebuffer = VK_NULL_HANDLE;
            for (auto image_index = 0; image_index < framebuffer.imagesCount; image_index++)
                image_free(framebuffer.pImages[image_index]);
            free(framebuffer.pImages);
            framebuffer.pImages = framebuffer.new_pImages;
            framebuffer.new_pImages = nullptr;
            framebuffer.render_pass_info_changed = true;
        }
        
        if (framebuffer.clear != clear || framebuffer.render_pass_info_changed) {
            framebuffer.renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
            framebuffer.renderPassInfo.renderPass = clear ? framebuffer.clearRenderPass : framebuffer.loadRenderPass;
            framebuffer.renderPassInfo.framebuffer = framebuffer.framebuffer;
            framebuffer.renderPassInfo.renderArea.offset = {0, 0};
            framebuffer.renderPassInfo.renderArea.extent.width = framebuffer.width;
            framebuffer.renderPassInfo.renderArea.extent.height = framebuffer.height;
            framebuffer.render_pass_info_changed = false;
            framebuffer.clear = clear;
        }

        if (framebuffer.clear_changed) {
            for (
                auto attachment_index = 0;
                attachment_index < framebuffer.attachmentTypesCount &&
                attachment_index < framebuffer.imagesCount;
                attachment_index++
            ) {
                auto& image = framebuffer.pImages[attachment_index];
                auto& attachmentType = framebuffer.pAttachmentTypes[attachment_index];

                VkClearValue clearValue;
                switch (attachmentType)
                {
                case AttachmentType::Depth:
                case AttachmentType::DepthStencil:
                case AttachmentType::Stencil:
                {
                    clearValue.depthStencil = framebuffer.clear_depth_stencil;
                    break;
                }
                case AttachmentType::Color:
                {
                    clearValue.color = framebuffer.clear_color;
                    break;
                }
                default: break;
                }

                framebuffer.clearValues.push_back(clearValue);
            }
            framebuffer.renderPassInfo.clearValueCount = framebuffer.clearValues.size();
            framebuffer.renderPassInfo.pClearValues = framebuffer.clearValues.data();
            framebuffer.clear_changed = false;
        }
        if (framebuffer.begin_info_changed) {
            framebuffer.beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
            framebuffer.beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
            framebuffer.beginInfo.pInheritanceInfo = 0;
            framebuffer.begin_info_changed = false;
        }

        vk_check("vkBeginCommandBuffer",
            vkBeginCommandBuffer(framebuffer.commandBuffer, &framebuffer.beginInfo));

        transitionColorLayoutForWriting(framebuffer);
        transitionDepthLayoutForWriting(framebuffer);
        transitionColorResolveLayoutForWriting(framebuffer);
        transitionDepthResolveLayoutForWriting(framebuffer);
        vkCmdBeginRenderPass(framebuffer.commandBuffer, &framebuffer.renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

        auto framebuffer_width = framebuffer.width;
        auto framebuffer_height = framebuffer.height;

        vec<float, 4> viewportData;

        auto renderer = dr.currentRenderer;
        assert(renderer);

        switch (renderer->currentTransform) {
            case VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR:
                viewportData = {renderer->swapChainExtent.width - framebuffer_height - 0, 0, framebuffer_height, framebuffer_width};
                break;
            case VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR:
                viewportData = {renderer->swapChainExtent.width - framebuffer_width - 0, renderer->swapChainExtent.height - framebuffer_height - 0, framebuffer_width, framebuffer_height};
                break;
            case VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR:
                viewportData = {0, renderer->swapChainExtent.height - framebuffer_width - 0, framebuffer_height, framebuffer_width};
                break;
            default:
                viewportData = {0, 0, framebuffer_width, framebuffer_height};
                break;
        }

        const VkViewport viewport = {
            .x = viewportData[0],
            .y = viewportData[1],
            .width = viewportData[2],
            .height = viewportData[3],
            .minDepth = 0.0F,
            .maxDepth = 1.0F,
        };
        vkCmdSetViewport(framebuffer.commandBuffer, 0, 1, &viewport);

        const VkRect2D scissor = {
            .offset =
                {
                    .x = (int32_t)viewportData[0],
                    .y = (int32_t)viewportData[1],
                },
            .extent =
                {
                    .width = (uint32_t)viewportData[2],
                    .height = (uint32_t)viewportData[3],
                },
        };
        vkCmdSetScissor(framebuffer.commandBuffer, 0, 1, &scissor);

        dr.commandBuffer = &framebuffer.commandBuffer;
    }

    void framebuffer_unbind(Framebuffer* framebuffer_ptr) {
        if (!framebuffer_ptr) {
            return;
        }

	    auto& framebuffer = *framebuffer_ptr;
    	vkCmdEndRenderPass(framebuffer.commandBuffer);
        transitionColorLayoutForReading(framebuffer);
        transitionDepthLayoutForReading(framebuffer);
        transitionColorResolveLayoutForReading(framebuffer);
        transitionDepthResolveLayoutForReading(framebuffer);
        vk_check("vkEndCommandBuffer",
            vkEndCommandBuffer(framebuffer.commandBuffer));

        if (framebuffer.submit_info_changed) {
            framebuffer.submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
            framebuffer.submitInfo.commandBufferCount = 1;
            framebuffer.submitInfo.pCommandBuffers = &framebuffer.commandBuffer;
        }

	    vkQueueSubmit(dr.graphicsQueue, 1, &framebuffer.submitInfo, VK_NULL_HANDLE);	
        // currentFramebufferImpl = 0;
        vkQueueWaitIdle(dr.graphicsQueue);

        dr.commandBuffer = nullptr;
    }

    bool framebuffer_destroy(Framebuffer*& framebuffer_ptr) {
        if (!framebuffer_ptr) {
            return false;
        }

        {
            auto& framebuffer = *framebuffer_ptr;
            if (framebuffer.own_images)
                for (auto image_index = 0; image_index < framebuffer.imagesCount; image_index++)
                    image_free(framebuffer.pImages[image_index]);
            if (framebuffer.pImages)
                free(framebuffer.pImages);
            if (framebuffer.pAttachmentTypes)
                free(framebuffer.pAttachmentTypes);
            vkDestroyRenderPass(dr.device, framebuffer.clearRenderPass, 0);
            vkDestroyRenderPass(dr.device, framebuffer.loadRenderPass, 0);
            vkDestroyFramebuffer(dr.device, framebuffer.framebuffer, 0);
            vkDestroyEvent(dr.device, framebuffer.event, 0);
            vkFreeCommandBuffers(dr.device, dr.commandPool, 1, &framebuffer.commandBuffer);
        }

        delete framebuffer_ptr;
        framebuffer_ptr = nullptr;
        return true;
    }

    bool framebuffer_resize(Framebuffer* framebuffer_ptr, uint32_t width, uint32_t height) {
        if (!framebuffer_ptr)
            return false;

		vkDeviceWaitIdle(dr.device);

        auto& framebuffer = *framebuffer_ptr;

        if ((framebuffer.width == width && framebuffer.height == height) ||
            (width > 16384 || height > 16384 || width == 0 || height == 0))
            return false;

        auto sizeof_pImages = framebuffer.imagesCount * sizeof(Image*);
        framebuffer.new_pImages = (Image**)malloc(sizeof_pImages);
        memcpy(framebuffer.new_pImages, framebuffer.pImages, sizeof_pImages);

        // resize images
        for (auto image_index = 0; image_index < framebuffer.imagesCount; image_index++) {
            image_resize_2D(framebuffer.new_pImages[image_index], width, height, nullptr, true);
        }
        
        // init fb create info
        std::vector<VkImageView> vkImageViews;
        vkImageViews.reserve(framebuffer.attachmentTypesCount);

        for (
            auto attachment_index = 0;
            attachment_index < framebuffer.imagesCount &&
            attachment_index < framebuffer.attachmentTypesCount;
            attachment_index++
        ) {
            auto& image = *framebuffer.new_pImages[attachment_index];

            assert(image.mip_levels == 1);

            auto& imageView = image.imageViews[0];
            if (imageView == VK_NULL_HANDLE)
            {
                throw std::runtime_error("Framebuffer attachment texture image view is null!");
            }

            vkImageViews.push_back(imageView);

            if (framebuffer.width != image.width || framebuffer.height != image.height)
            {
                framebuffer.width = image.width;
                framebuffer.height = image.height;
            }
        }

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = framebuffer.clearRenderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(vkImageViews.size());
        framebufferInfo.pAttachments = vkImageViews.data();
        framebufferInfo.width = framebuffer.width;
        framebufferInfo.height = framebuffer.height;
        framebufferInfo.layers = 1;

        // create fb
        vk_check("vkCreateFramebuffer",
            vkCreateFramebuffer(dr.device, &framebufferInfo, nullptr, &framebuffer.new_framebuffer));

        return true;
    }
    
    bool framebuffer_changed(Framebuffer* framebuffer) {
        return framebuffer->new_pImages;
    }
    
    Image* framebuffer_get_image(Framebuffer* framebuffer_ptr, AttachmentType attachmentType, bool new_image) {
        if (!framebuffer_ptr)
            return nullptr;
        auto& framebuffer = *framebuffer_ptr;
        for (
            auto attachment_index = 0;
            attachment_index < framebuffer.attachmentTypesCount &&
            attachment_index < framebuffer.imagesCount;
            attachment_index++
        ) {
            if (framebuffer.pAttachmentTypes[attachment_index] == attachmentType) {
                if (new_image && framebuffer.new_pImages) {
                    return framebuffer.new_pImages[attachment_index];
                }
                else {
                    return framebuffer.pImages[attachment_index];
                }
            }
        }
        return nullptr;
    }

    BlendState BlendState::Disabled = {
        .enable = false
    };
    BlendState BlendState::MainFramebuffer = {
        true,
        BlendFactor::One,
        BlendFactor::Zero,
        BlendFactor::One,
        BlendFactor::Zero
    };
    BlendState BlendState::Layout = {
        true,
        BlendFactor::One,
        BlendFactor::One,
        BlendFactor::One,
        BlendFactor::One
    };
    BlendState BlendState::Text = {
        true,
        BlendFactor::SrcAlpha,
        BlendFactor::OneMinusSrcAlpha,
        BlendFactor::One,
        BlendFactor::One
    };
    BlendState BlendState::SrcAlpha = {
        true,
        BlendFactor::SrcAlpha,
        BlendFactor::OneMinusSrcAlpha,
        BlendFactor::SrcAlpha,
        BlendFactor::OneMinusSrcAlpha
    };
    
    void prepareImageBarrier(
        VkCommandBuffer commandBuffer, VkImage image, VkFormat format,
        VkImageLayout oldLayout, VkImageLayout newLayout,
        VkImageAspectFlags aspectMask, VkPipelineStageFlags& sourceStage,
        VkPipelineStageFlags& destinationStage, VkImageMemoryBarrier& barrier)
    {
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        
        barrier.srcQueueFamilyIndex = dr.graphicsAndComputeFamily;
        barrier.dstQueueFamilyIndex = dr.graphicsAndComputeFamily;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = aspectMask;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.pNext = nullptr; // Initialize pNext

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;

        // Determine Source Access Mask and Stage based on oldLayout
        switch (oldLayout)
        {
        case VK_IMAGE_LAYOUT_UNDEFINED:
            barrier.srcAccessMask = 0;
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            break;
        case VK_IMAGE_LAYOUT_PREINITIALIZED:
            barrier.srcAccessMask = VK_ACCESS_HOST_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_HOST_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // Or other relevant shader stage
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            sourceStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            break;
        default:
            barrier.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            sourceStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            break;
        }

        // Determine Destination Access Mask and Stage based on newLayout
        switch (newLayout)
        {
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            break;
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
            destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
            break;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT; // Or SHADER_READ
            destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT; // Or FRAGMENT_SHADER
            break;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // Or other relevant shader stage
            break;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
            destinationStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
            break;
        default:
            barrier.dstAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT;
            destinationStage = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT;
            break;
        }

        // If transitioning from UNDEFINED, srcStage must be TOP_OF_PIPE_BIT
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
    }

    void transitionDepthLayoutForWriting(Framebuffer& framebuffer)
    {
        auto DepthImage_ptr = getDepthImage(framebuffer);
        if (DepthImage_ptr)
        {
            auto& DepthImage = *DepthImage_ptr;
            static auto new_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            auto& current_layout = DepthImage.current_layouts[0];
            if (current_layout != new_layout)
            {
                VkImageMemoryBarrier barrier;
                VkPipelineStageFlags sourceStage;
                VkPipelineStageFlags destinationStage;
                prepareImageBarrier(framebuffer.commandBuffer,
                    DepthImage.image, DepthImage.format,
                    current_layout,
                    new_layout,
                    VK_IMAGE_ASPECT_DEPTH_BIT, sourceStage, destinationStage, barrier);
                vkCmdPipelineBarrier(framebuffer.commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
                current_layout = new_layout;
            }
        }
    }

    void transitionDepthLayoutForReading(Framebuffer& framebuffer)
    {
        auto DepthImage_ptr = getDepthImage(framebuffer);
        if (DepthImage_ptr)
        {
            auto& DepthImage = *DepthImage_ptr;
            // shadowMapImage->isDirty = true;
            // auto& textureImpl = *static_cast<VulkanImageImpl*>(shadowMapImage->rendererData);

            auto& current_layout = DepthImage.current_layouts[0];
            if (current_layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
                current_layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                std::cerr << "Warning: Directional shadow map layout was not ATTACHMENT_OPTIMAL before transition!" << std::endl;
            }

            auto new_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
            
            vkCmdSetEvent(framebuffer.commandBuffer, framebuffer.event, VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT);
            
            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = current_layout;
            barrier.newLayout = new_layout;
            barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = DepthImage.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            vkCmdWaitEvents(
                framebuffer.commandBuffer,
                1, &framebuffer.event,
                VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            current_layout = new_layout;
        }
    }

    void transitionColorLayoutForWriting(Framebuffer& framebuffer)
    {
        auto ColorImage_ptr = getColorImage(framebuffer);
        if (ColorImage_ptr)
        {
            auto& ColorImage = *ColorImage_ptr;

            static auto new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            auto& current_layout = ColorImage.current_layouts[0];
            if (current_layout != new_layout)
            {
                VkImageMemoryBarrier colorBarrier = {};
                VkPipelineStageFlags sourceStage;
                VkPipelineStageFlags destinationStage;
                prepareImageBarrier(
                    framebuffer.commandBuffer,
                    ColorImage.image, ColorImage.format,
                    current_layout, new_layout,
                    VK_IMAGE_ASPECT_COLOR_BIT, sourceStage, destinationStage, colorBarrier);
                vkCmdPipelineBarrier(framebuffer.commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &colorBarrier);
                current_layout = new_layout;
            }
        }
    }

    void transitionColorLayoutForReading(Framebuffer& framebuffer)
    {
        auto ColorImage_ptr = getColorImage(framebuffer);
        if (ColorImage_ptr && !hasColorResolveAttachment(framebuffer))
        {
            auto& ColorImage = *ColorImage_ptr;
            // colorImage->isDirty = true;

            auto& current_layout = ColorImage.current_layouts[0];
            auto new_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            VkImageMemoryBarrier barrier{};
            barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
            barrier.oldLayout = current_layout;
            barrier.newLayout = new_layout;
            barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
            barrier.image = ColorImage.image;
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            barrier.subresourceRange.baseMipLevel = 0;
            barrier.subresourceRange.levelCount = 1;
            barrier.subresourceRange.baseArrayLayer = 0;
            barrier.subresourceRange.layerCount = 1;

            vkCmdPipelineBarrier(
                framebuffer.commandBuffer,
                VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
                0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            current_layout = new_layout;
        }
    }

    void transitionColorResolveLayoutForWriting(Framebuffer& framebuffer)
    {
        auto ColorResolveImage_ptr = getColorResolveImage(framebuffer);
        if (ColorResolveImage_ptr)
        {
            auto& ColorResolveImage = *ColorResolveImage_ptr;

            static auto new_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
            auto& current_layout = ColorResolveImage.current_layouts[0];
            if (current_layout != new_layout)
            {
                VkImageMemoryBarrier colorBarrier = {};
                VkPipelineStageFlags sourceStage;
                VkPipelineStageFlags destinationStage;
                prepareImageBarrier(framebuffer.commandBuffer,
                    ColorResolveImage.image, ColorResolveImage.format,
                    current_layout, new_layout,
                    VK_IMAGE_ASPECT_COLOR_BIT, sourceStage, destinationStage, colorBarrier);
                vkCmdPipelineBarrier(framebuffer.commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &colorBarrier);
                current_layout = new_layout;
            }
        }
    }

    void transitionColorResolveLayoutForReading(Framebuffer& framebuffer)
    {
        auto ColorResolveImage_ptr = getColorResolveImage(framebuffer);
        if (ColorResolveImage_ptr)
        {
            auto& ColorResolveImage = *ColorResolveImage_ptr;
            // colorResolveImage->isDirty = true;

            ColorResolveImage.current_layouts[0] = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        }
    }

    void transitionDepthResolveLayoutForWriting(Framebuffer& framebuffer)
    {
        auto DepthResolveImage_ptr = getDepthResolveImage(framebuffer);
        if (DepthResolveImage_ptr)
        {
            auto& DepthResolveImage = *DepthResolveImage_ptr;

            static auto new_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
            auto& current_layout = DepthResolveImage.current_layouts[0];
            if (current_layout != new_layout)
            {
                VkImageMemoryBarrier depthBarrier = {};
                VkPipelineStageFlags sourceStage;
                VkPipelineStageFlags destinationStage;
                prepareImageBarrier(framebuffer.commandBuffer,
                    DepthResolveImage.image, DepthResolveImage.format,
                    current_layout, new_layout,
                    VK_IMAGE_ASPECT_DEPTH_BIT, sourceStage, destinationStage, depthBarrier);
                vkCmdPipelineBarrier(framebuffer.commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &depthBarrier);
                current_layout = new_layout;
            }
        }
    }

    void transitionDepthResolveLayoutForReading(Framebuffer& framebuffer)
    {
        auto DepthResolveImage_ptr = getDepthResolveImage(framebuffer);
        if (DepthResolveImage_ptr)
        {
            auto& DepthResolveImage = *DepthResolveImage_ptr;
            // depthResolveImage->isDirty = true;

            auto& current_layout = DepthResolveImage.current_layouts[0];
            current_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

            // Ensure the layout we are transitioning *from* is correct
            if (current_layout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL &&
                current_layout != VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
            {
                // Log warning or throw? Indicates layout tracking might be off.
                // For now, let's assume it *should* be ATTACHMENT_OPTIMAL here.
                std::cerr << "Warning: Directional shadow map layout was not ATTACHMENT_OPTIMAL before transition!" << std::endl;
            }

            VkImageLayout oldLayout = current_layout;
            VkImageLayout newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;

            VkImageMemoryBarrier barrier;
            VkPipelineStageFlags sourceStage;
            VkPipelineStageFlags destinationStage;

            // Prepare barrier details: ATTACHMENT_OPTIMAL -> READ_ONLY_OPTIMAL
            prepareImageBarrier(framebuffer.commandBuffer, DepthResolveImage.image, DepthResolveImage.format,
                                                    oldLayout, // Old layout
                                                    newLayout, // New layout for sampling
                                                    VK_IMAGE_ASPECT_DEPTH_BIT, // Assuming depth only
                                                    sourceStage, destinationStage, barrier);

            // Record the barrier in the current command buffer
            vkCmdPipelineBarrier(framebuffer.commandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

            // Update tracked layout
            current_layout = newLayout;
        }
    }

    Image* getDepthImage(const Framebuffer& framebuffer) {
        for (
            auto attachment_index = 0;
            attachment_index < framebuffer.attachmentTypesCount &&
            attachment_index < framebuffer.imagesCount;
            attachment_index++
        ) {
            auto& attachmentType = framebuffer.pAttachmentTypes[attachment_index];
            if (attachmentType == AttachmentType::Depth) {
                return framebuffer.pImages[attachment_index];
            }
        }
        return nullptr;
    }

    Image* getColorImage(const Framebuffer& framebuffer) {
        for (
            auto attachment_index = 0;
            attachment_index < framebuffer.attachmentTypesCount &&
            attachment_index < framebuffer.imagesCount;
            attachment_index++
        ) {
            auto& attachmentType = framebuffer.pAttachmentTypes[attachment_index];
            if (attachmentType == AttachmentType::Color) {
                return framebuffer.pImages[attachment_index];
            }
        }
        return nullptr;
    }

    Image* getColorResolveImage(const Framebuffer& framebuffer) {
        for (
            auto attachment_index = 0;
            attachment_index < framebuffer.attachmentTypesCount &&
            attachment_index < framebuffer.imagesCount;
            attachment_index++
        ) {
            auto& attachmentType = framebuffer.pAttachmentTypes[attachment_index];
            if (attachmentType == AttachmentType::ColorResolve) {
                return framebuffer.pImages[attachment_index];
            }
        }
        return nullptr;
    }

    Image* getDepthResolveImage(const Framebuffer& framebuffer) {
        for (
            auto attachment_index = 0;
            attachment_index < framebuffer.attachmentTypesCount &&
            attachment_index < framebuffer.imagesCount;
            attachment_index++
        ) {
            auto& attachmentType = framebuffer.pAttachmentTypes[attachment_index];
            if (attachmentType == AttachmentType::DepthResolve) {
                return framebuffer.pImages[attachment_index];
            }
        }
        return nullptr;
    }

    bool hasColorResolveAttachment(const Framebuffer& framebuffer) {
        for (
            auto attachment_index = 0;
            attachment_index < framebuffer.attachmentTypesCount &&
            attachment_index < framebuffer.imagesCount;
            attachment_index++
        ) {
            auto& attachmentType = framebuffer.pAttachmentTypes[attachment_index];
            if (attachmentType == AttachmentType::ColorResolve) {
                return true;
            }
        }
        return false;
    }
}
