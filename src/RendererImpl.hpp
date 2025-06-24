struct Renderer
{
    WINDOW* window = 0;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkSwapchainKHR swapChain;
    VkSwapchainKHR swapChains[1];
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
#ifdef __ANDROID__
	bool supportsIndirectCount = false;
#else
    bool supportsIndirectCount = true;
#endif
    // PFN_vkCreateDebugUtilsMessengerEXT _vkCreateDebugUtilsMessengerEXT;
	void destroy_surface();
	void cleanup_swapchain();
};