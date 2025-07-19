#pragma once
#define MAX_FRAMES_IN_FLIGHT 4
struct Renderer
{
    WINDOW* window = 0;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkRenderPass renderPass = VK_NULL_HANDLE;
    VkSwapchainKHR swapChain;
    VkSwapchainKHR swapChains[1];
    int imageCount = MAX_FRAMES_IN_FLIGHT;
    std::vector<VkImage> swapChainImages;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;
    VkExtent2D swapChainExtent;
    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkCommandBuffer> commandBuffers;
    uint32_t currentFrame = 0;
    uint32_t imageIndex = 0;
	VkCommandBufferBeginInfo beginInfo;
    VkSubmitInfo submitInfo;
    VkPresentInfoKHR presentInfo;
    VkPipelineStageFlags waitStages[1];
    VkSemaphore signalSemaphores[1];
	std::map<size_t, std::pair<VkBuffer, VkDeviceMemory>> drawBuffers;
	std::map<size_t, std::pair<VkBuffer, VkDeviceMemory>> countBuffers;
	VkSurfaceTransformFlagBitsKHR currentTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

    std::vector<DrawInformation*> vec_draw_information;
    std::vector<std::pair<ShaderDrawList*, std::function<void()>>> screen_draw_lists;
    std::vector<std::tuple<Framebuffer*, ShaderDrawList*, std::function<void()>>> fb_draw_lists;
#ifdef __ANDROID__
	bool supportsIndirectCount = false;
#else
    bool supportsIndirectCount = true;
#endif
    // PFN_vkCreateDebugUtilsMessengerEXT _vkCreateDebugUtilsMessengerEXT;
	void destroy_surface();
	void cleanup_swapchain();
};