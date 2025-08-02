#pragma once
#include <dz/Framebuffer.hpp>
#include "Directz.cpp.hpp"

namespace dz {
    struct Framebuffer {
        Image** pImages;
        int imagesCount;
        AttachmentType* pAttachmentTypes;
        int attachmentTypesCount;
        BlendState blendState;
        bool own_images;

		VkCommandBuffer commandBuffer;
		VkRenderPass renderPass;
		VkFramebuffer framebuffer;
		uint32_t attachmentsSize;
		uint32_t width;
		uint32_t height;
		VkEvent event;
        
        VkRenderPassBeginInfo renderPassInfo{};
        bool render_pass_info_changed = true;
        
        VkCommandBufferBeginInfo beginInfo = {};
        bool begin_info_changed = true;

        std::vector<VkClearValue> clearValues;
        bool clear_changed = true;
        VkClearColorValue clear_color;
        VkClearDepthStencilValue clear_depth_stencil;

	    VkSubmitInfo submitInfo = {};
        bool submit_info_changed = true;

        Image** new_pImages = 0;
        VkFramebuffer new_framebuffer = VK_NULL_HANDLE;
    };
}