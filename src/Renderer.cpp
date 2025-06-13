// #define VK_INSTANCE(N, PFN, NAME) renderer->N = (PFN)vkGetInstanceProcAddr(instance, NAME)
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
    uint32_t imageIndex = -1;
	VkCommandBufferBeginInfo beginInfo;
    VkSubmitInfo submitInfo;
    VkPresentInfoKHR presentInfo;
    VkPipelineStageFlags waitStages[1];
    VkSemaphore signalSemaphores[1];
	std::map<size_t, std::pair<VkBuffer, VkDeviceMemory>> drawBuffers;
	std::map<size_t, std::pair<VkBuffer, VkDeviceMemory>> countBuffers;
    // PFN_vkCreateDebugUtilsMessengerEXT _vkCreateDebugUtilsMessengerEXT;
};
constexpr int MAX_FRAMES_IN_FLIGHT = 4;
struct QueueFamilyIndices
{
    int32_t graphicsAndComputeFamily = -1;
    int32_t presentFamily = -1;
    bool isComplete() { return graphicsAndComputeFamily > -1 && presentFamily > -1; };
};
struct SwapChainSupportDetails
{
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};
#ifndef NDEBUG
bool check_validation_layers_support();
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
);
void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo);
#endif
void create_surface(Renderer* renderer);

VkSampleCountFlagBits get_max_usable_sample_count(DirectRegistry* direct_registry, Renderer* renderer);
void direct_registry_ensure_physical_device(DirectRegistry* direct_registry, Renderer* renderer);
uint32_t rate_device_suitability(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device);
bool is_device_suitable(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device);
QueueFamilyIndices find_queue_families(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device);
void direct_registry_ensure_logical_device(DirectRegistry* direct_registry, Renderer* renderer);
bool create_swap_chain(Renderer* renderer);
SwapChainSupportDetails query_swap_chain_support(Renderer* renderer, VkPhysicalDevice device);
VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
VkPresentModeKHR choose_swap_present_mode(Renderer* renderer, const std::vector<VkPresentModeKHR>& availablePresentModes);
VkExtent2D choose_swap_extent(Renderer* renderer, VkSurfaceCapabilitiesKHR capabilities);
void ensure_command_pool(Renderer* renderer);
void ensure_command_buffers(Renderer* renderer);
void create_image_views(Renderer* renderer);
void ensure_render_pass(Renderer* renderer);
void create_framebuffers(Renderer* renderer);
void create_sync_objects(Renderer* renderer);
bool vk_check(const char* fn, VkResult result);
void pre_begin_render_pass(Renderer* renderer);
void begin_render_pass(Renderer* renderer);
void post_render_pass(Renderer* renderer);
bool swap_buffers(Renderer* renderer);
void renderer_draw_commands(Renderer* renderer, Shader* shader, const std::vector<DrawIndirectCommand>& commands);
void renderer_destroy(Renderer* renderer);
void destroy_swap_chain(Renderer* renderer);
Renderer* renderer_init(WINDOW* window)
{
    auto renderer = new Renderer{
        .window = window
    };
    //
	renderer->beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	renderer->waitStages[0] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
	renderer->submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	renderer->submitInfo.pNext = 0;
	renderer->submitInfo.waitSemaphoreCount = 1;
	renderer->submitInfo.commandBufferCount = 1;
	renderer->presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	renderer->presentInfo.pNext = 0;
	renderer->presentInfo.waitSemaphoreCount = 1;
	renderer->presentInfo.swapchainCount = 1;
	renderer->presentInfo.pResults = 0;
    //
    create_surface(renderer);
    direct_registry_ensure_physical_device(DZ_RGY.get(), renderer);
    direct_registry_ensure_logical_device(DZ_RGY.get(), renderer);
    create_swap_chain(renderer);
    ensure_command_pool(renderer);
    ensure_command_buffers(renderer);
	create_image_views(renderer);
	ensure_render_pass(renderer);
	// createDepthResources();
	create_framebuffers(renderer);
    create_sync_objects(renderer);
    return renderer;
}

void renderer_free(Renderer* renderer)
{
	renderer_destroy(renderer);
    delete renderer;
}

#ifndef NDEBUG
bool check_validation_layers_support()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, 0);
	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
	static std::vector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;
		for (const auto& layerProperties : availableLayers)
		{
			if (strcmp(layerName, layerProperties.layerName) == 0)
			{
				layerFound = true;
				break;
			}
		}
		if (!layerFound)
		{
			return false;
		}
	}
	return true;
}
VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData
)
{
	if (messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ||
			messageSeverity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT)
		throw std::runtime_error(std::string(pCallbackData->pMessage));
	return VK_FALSE;
}
void populate_debug_messenger_create_info(VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
	createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
	createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
	createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
		VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
	createInfo.pfnUserCallback = debug_callback;
}
#endif

void direct_registry_create_instance(DirectRegistry* direct_registry)
{
	VkApplicationInfo appInfo{};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "DirectZ Application";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "DirectZ";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_2;
	VkInstanceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	//
	std::vector<const char*> extensions;
	extensions.push_back("VK_KHR_surface");
	auto& windowType = direct_registry->windowType;
	switch (windowType)
	{
	case WINDOW_TYPE_XCB:
		{
			extensions.push_back("VK_KHR_xcb_surface");
			break;
		}
	case WINDOW_TYPE_X11:
		{
			extensions.push_back("VK_KHR_xlib_surface");
			break;
		}
	case WINDOW_TYPE_WAYLAND:
		{
			extensions.push_back("VK_KHR_wayland_surface");
			break;
		}
	case WINDOW_TYPE_WIN32:
		{
			extensions.push_back("VK_KHR_win32_surface");
			break;
		}
	case WINDOW_TYPE_ANDROID:
		{
			extensions.push_back("VK_KHR_android_surface");
			break;
		}
	case WINDOW_TYPE_MACOS:
		{
			extensions.push_back("VK_MVK_macos_surface");
			break;
		}
	case WINDOW_TYPE_IOS:
		{
			extensions.push_back("VK_MVK_ios_surface");
			break;
		}
	}
#if defined(MACOS)
	extensions.push_back("VK_KHR_portability_enumeration");
	createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
#ifndef NDEBUG
	extensions.push_back("VK_EXT_debug_utils");
#endif
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	//
	VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
	std::vector<const char*> layers;
#if !defined(NDEBUG)
	if (check_validation_layers_support())
	{
		layers.push_back("VK_LAYER_KHRONOS_validation");
		createInfo.enabledLayerCount = layers.size();
		createInfo.ppEnabledLayerNames = layers.data();
		populate_debug_messenger_create_info(debugCreateInfo);
		createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
	}
	else
	{
		std::cout << "Validation layers requested, but not available" << std::endl;
		createInfo.enabledLayerCount = 0;
	}
#endif
	vk_check("vkCreateInstance", vkCreateInstance(&createInfo, 0, &direct_registry->instance));
}

void create_surface(Renderer* renderer)
{
    auto& window = *renderer->window;
	auto& dr = *window.registry;
	auto& windowType = dr.windowType;
#ifdef __linux__
	VkXcbSurfaceCreateInfoKHR surfaceCreateInfo{};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.connection = window.connection;
	surfaceCreateInfo.window = window.window;
	vk_check("vkCreateXcbSurfaceKHR",
		vkCreateXcbSurfaceKHR(dr.instance, &surfaceCreateInfo, 0, &renderer->surface));
#elif defined(ANDROID)
#elif defined(_WIN32)
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = window.hInstance;
	surfaceCreateInfo.hwnd = window.hwnd;
	vk_check("vkCreateWin32SurfaceKHR",
		vkCreateWin32SurfaceKHR(dr.instance, &surfaceCreateInfo, 0, &renderer->surface));
#elif defined(MACOS)
    VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo{};
    surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
    surfaceCreateInfo.pView = window.nsView;
    vk_check("vkCreateMacOSSurfaceMVK",
		vkCreateMacOSSurfaceMVK(dr.instance, &surfaceCreateInfo, 0, &renderer->surface));
#endif
}

VkSampleCountFlagBits get_max_usable_sample_count(DirectRegistry* direct_registry, Renderer* renderer)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(direct_registry->physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts &
		physicalDeviceProperties.limits.framebufferDepthSampleCounts;
	if (counts & VK_SAMPLE_COUNT_64_BIT)
		return VK_SAMPLE_COUNT_64_BIT;
	if (counts & VK_SAMPLE_COUNT_32_BIT)
		return VK_SAMPLE_COUNT_32_BIT;
	if (counts & VK_SAMPLE_COUNT_16_BIT)
		return VK_SAMPLE_COUNT_16_BIT;
	if (counts & VK_SAMPLE_COUNT_8_BIT)
		return VK_SAMPLE_COUNT_8_BIT;
	if (counts & VK_SAMPLE_COUNT_4_BIT)
		return VK_SAMPLE_COUNT_4_BIT;
	if (counts & VK_SAMPLE_COUNT_2_BIT)
		return VK_SAMPLE_COUNT_2_BIT;
	return VK_SAMPLE_COUNT_1_BIT;
}
void direct_registry_ensure_physical_device(DirectRegistry* direct_registry, Renderer* renderer)
{
	if (direct_registry->physicalDevice)
		return;
	uint32_t deviceCount = 0;
	vk_check("vkEnumeratePhysicalDevices", vkEnumeratePhysicalDevices(direct_registry->instance, &deviceCount, 0));
	if (deviceCount == 0)
		throw std::runtime_error("VulkanRenderer-getPhysicalDevice: failed to find GPUs with Vulkan support!");
	std::vector<VkPhysicalDevice> devices;
	devices.resize(deviceCount);
	vk_check("vkEnumeratePhysicalDevices", vkEnumeratePhysicalDevices(direct_registry->instance, &deviceCount, devices.data()));
	std::map<uint32_t, VkPhysicalDevice> physicalDeviceScores;
	for (auto& device : devices)
	{
		physicalDeviceScores[rate_device_suitability(direct_registry, renderer, device)] = device;
	}
	auto end = physicalDeviceScores.rend();
	auto begin = physicalDeviceScores.rbegin();
	uint32_t selectedDeviceScore;
	for (auto iter = begin; iter != end; ++iter)
	{
		auto device = iter->second;
		if (is_device_suitable(direct_registry, renderer, device))
		{
			direct_registry->physicalDevice = device;
			direct_registry->maxMSAASamples = get_max_usable_sample_count(direct_registry, renderer);
			selectedDeviceScore = iter->first;
			break;
		}
		continue;
	}
	if (direct_registry->physicalDevice == VK_NULL_HANDLE)
	{
		throw std::runtime_error("VulkanRenderer-getPhysicalDevice: failed to find a suitable GPU!");
	}
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(direct_registry->physicalDevice, &physicalDeviceProperties);
	std::cout << "Selected Physical Device: '" << physicalDeviceProperties.deviceName
						<< "' with score of: " << selectedDeviceScore << std::endl;
	return;
}
uint32_t rate_device_suitability(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device)
{
	uint32_t score = 0;
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device, &properties);
	VkPhysicalDeviceFeatures features;
	vkGetPhysicalDeviceFeatures(device, &features);
	if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
	{
		score += 1000;
	}
	else if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
	{
		score += 500;
	}
	score += properties.limits.maxImageDimension2D;
	if (!features.geometryShader)
	{
		return 0;
	}
	auto indices = find_queue_families(direct_registry, renderer, device);
	if (indices.graphicsAndComputeFamily != indices.presentFamily)
	{
		score += 1000;
	}
	std::cout << "Rated physical device [" << properties.deviceName << "] a score of: " << score << std::endl;
	return score;
}
bool is_device_suitable(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device)
{
	QueueFamilyIndices indices = find_queue_families(direct_registry, renderer, device);
	return indices.isComplete();
}
QueueFamilyIndices find_queue_families(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device)
{
	QueueFamilyIndices indices;
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, 0);
	std::vector<VkQueueFamilyProperties> queueFamilies;
	queueFamilies.resize(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());
	int32_t index = 0;
	for (auto& queueFamily : queueFamilies)
	{
		if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT))
		{
			indices.graphicsAndComputeFamily = index;
		}
		VkBool32 presentSupport = false;
		vk_check("vkGetPhysicalDeviceSurfaceSupportKHR", vkGetPhysicalDeviceSurfaceSupportKHR(device, index, renderer->surface, &presentSupport));
		if (presentSupport)
		{
			indices.presentFamily = index;
		}
		if (indices.isComplete())
		{
			break;
		}
		index++;
		continue;
	}
	return indices;
}

void direct_registry_ensure_logical_device(DirectRegistry* direct_registry, Renderer* renderer)
{
	if (direct_registry->device)
		return;
	QueueFamilyIndices indices = find_queue_families(direct_registry, renderer, direct_registry->physicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::vector<int32_t> uniqueQueueFamilies({indices.graphicsAndComputeFamily, indices.presentFamily});
	float queuePriority = 1;
	int32_t index = 0;
	for (auto& queueFamily : uniqueQueueFamilies)
	{
		VkDeviceQueueCreateInfo queueCreateInfo{};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
		if (index == 0 && queueFamily == uniqueQueueFamilies[1])
		{
			break;
		}
		continue;
	}
	VkDeviceCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = queueCreateInfos.size();
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	std::vector<const char*> extensions;
	// check extensions
	uint32_t extensionCount = 0;
	// Query number of available extensions
	vk_check("vkEnumerateDeviceExtensionProperties",
		vkEnumerateDeviceExtensionProperties(direct_registry->physicalDevice, 0, &extensionCount, 0));
	std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
	vk_check("vkEnumerateDeviceExtensionProperties",
		vkEnumerateDeviceExtensionProperties(direct_registry->physicalDevice, 0, &extensionCount, deviceExtensions.data()));
	for (const auto& ext : deviceExtensions)
	{
		if (strcmp(ext.extensionName, "VK_KHR_portability_subset") == 0)
		{
			extensions.push_back("VK_KHR_portability_subset");
		}
	}
	extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
	extensions.push_back("VK_KHR_maintenance1");
	extensions.push_back("VK_KHR_swapchain");
	// extensions[2] = VK_KHR_ACCELERATION_STRUCTURE_EXTENSION_NAME;
	// extensions[3] = VK_KHR_RAY_TRACING_PIPELINE_EXTENSION_NAME;
	// extensions[4] = VK_KHR_DEFERRED_HOST_OPERATIONS_EXTENSION_NAME;
	createInfo.enabledExtensionCount = extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = 0;
	VkPhysicalDeviceDescriptorIndexingFeatures descriptorIndexingFeatures{};
	descriptorIndexingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
	descriptorIndexingFeatures.pNext = 0;
	// descriptorIndexingFeatures.robustBufferAccessUpdateAfterBind = VK_FALSE;
	descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind = VK_FALSE;
	descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind = VK_FALSE;
	descriptorIndexingFeatures.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_FALSE;
	descriptorIndexingFeatures.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_FALSE;
	// assert(descriptorIndexingFeatures.shaderSampledImageArrayNonUniformIndexing);
	// assert(descriptorIndexingFeatures.descriptorBindingSampledImageUpdateAfterBind);
	// assert(descriptorIndexingFeatures.shaderUniformBufferArrayNonUniformIndexing);
	// #ifndef MACOS
	// 	assert(descriptorIndexingFeatures.descriptorBindingUniformBufferUpdateAfterBind);
	// #endif
	// 	assert(descriptorIndexingFeatures.shaderStorageBufferArrayNonUniformIndexing);
	// 	assert(descriptorIndexingFeatures.descriptorBindingStorageBufferUpdateAfterBind);
	VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParamsFeatures_query = {};
	shaderDrawParamsFeatures_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;
	shaderDrawParamsFeatures_query.shaderDrawParameters = VK_TRUE;
    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
	vulkan12Features.pNext = &shaderDrawParamsFeatures_query;
	// vulkan12Features.pNext = &descriptorIndexingFeatures;
    vulkan12Features.drawIndirectCount = VK_TRUE;
	VkPhysicalDeviceFeatures2 deviceFeatures{};
	deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	deviceFeatures.pNext = &vulkan12Features;
	vkGetPhysicalDeviceFeatures2(direct_registry->physicalDevice, &deviceFeatures);
	deviceFeatures.features.sampleRateShading = VK_TRUE;
	deviceFeatures.features.depthClamp = VK_TRUE;
	deviceFeatures.features.depthBiasClamp = VK_TRUE;
	deviceFeatures.features.samplerAnisotropy = VK_TRUE;
	deviceFeatures.features.robustBufferAccess = VK_TRUE;
	deviceFeatures.features.multiDrawIndirect = VK_TRUE;
	deviceFeatures.features.drawIndirectFirstInstance = VK_TRUE;
	createInfo.pNext = &deviceFeatures;
	auto createdDevice = vk_check("vkCreateDevice", vkCreateDevice(direct_registry->physicalDevice, &createInfo, 0, &direct_registry->device));
	if (!createdDevice)
	{
		throw std::runtime_error("VulkanRenderer-createLogicalDevice: failed to create device");
	}
	vkGetDeviceQueue(direct_registry->device, indices.graphicsAndComputeFamily, 0, &direct_registry->graphicsQueue);
	vkGetDeviceQueue(direct_registry->device, indices.presentFamily, 0, &direct_registry->presentQueue);
	vkGetDeviceQueue(direct_registry->device, indices.graphicsAndComputeFamily, 0, &direct_registry->computeQueue);
	return;
}

bool create_swap_chain(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	SwapChainSupportDetails swapChainSupport = query_swap_chain_support(renderer, direct_registry->physicalDevice);
	if (direct_registry->firstSurfaceFormat.format == 0)
	{
		VkSurfaceFormatKHR surfaceFormat = choose_swap_surface_format(swapChainSupport.formats);
		direct_registry->firstSurfaceFormat = surfaceFormat;
	}
	VkPresentModeKHR presentMode = choose_swap_present_mode(renderer, swapChainSupport.presentModes);
	VkExtent2D extent = choose_swap_extent(renderer, swapChainSupport.capabilities);
	if (extent.height == 0 || extent.width == 0)
	{

		return false;
	}
	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}
	VkSwapchainCreateInfoKHR createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = renderer->surface;
	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = direct_registry->firstSurfaceFormat.format;
	createInfo.imageColorSpace = direct_registry->firstSurfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
	QueueFamilyIndices indices = find_queue_families(direct_registry, renderer, direct_registry->physicalDevice);
	uint32_t queueFamilyIndices[] = {(uint32_t)indices.graphicsAndComputeFamily, (uint32_t)indices.presentFamily};
	if (indices.graphicsAndComputeFamily != indices.presentFamily)
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	}
	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vk_check("vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(direct_registry->physicalDevice, renderer->surface, &surfaceCapabilities));
	if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR)
	{
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	}
	else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR)
	{
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	}
	else if (surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR)
	{
		createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	}
	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;
	createInfo.oldSwapchain = VK_NULL_HANDLE;
	vk_check("vkCreateSwapchainKHR", vkCreateSwapchainKHR(direct_registry->device, &createInfo, 0, &renderer->swapChain));
	vk_check("vkGetSwapchainImagesKHR", vkGetSwapchainImagesKHR(direct_registry->device, renderer->swapChain, &imageCount, 0));
	renderer->swapChainImages.resize(imageCount);
	vk_check("vkGetSwapchainImagesKHR",
		vkGetSwapchainImagesKHR(direct_registry->device, renderer->swapChain, &imageCount, renderer->swapChainImages.data()));
	renderer->swapChainExtent = extent;
	return true;
}

SwapChainSupportDetails query_swap_chain_support(Renderer* renderer, VkPhysicalDevice device)
{
	auto direct_registry = DZ_RGY.get();
	SwapChainSupportDetails details;
	vk_check("vkGetPhysicalDeviceSurfaceCapabilitiesKHR",
		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(direct_registry->physicalDevice, renderer->surface, &details.capabilities));
	uint32_t formatCount;
	vk_check("vkGetPhysicalDeviceSurfaceFormatsKHR",
		vkGetPhysicalDeviceSurfaceFormatsKHR(direct_registry->physicalDevice, renderer->surface, &formatCount, 0));
	if (formatCount != 0)
	{
		details.formats.resize(formatCount);
		vk_check("vkGetPhysicalDeviceSurfaceFormatsKHR",
			vkGetPhysicalDeviceSurfaceFormatsKHR(direct_registry->physicalDevice, renderer->surface, &formatCount, details.formats.data()));
	}
	uint32_t presentModeCount;
	vk_check("vkGetPhysicalDeviceSurfacePresentModesKHR",
		vkGetPhysicalDeviceSurfacePresentModesKHR(direct_registry->physicalDevice, renderer->surface, &presentModeCount, 0));
	if (presentModeCount != 0)
	{
		details.presentModes.resize(presentModeCount);
		vk_check("vkGetPhysicalDeviceSurfacePresentModesKHR",
			vkGetPhysicalDeviceSurfacePresentModesKHR(direct_registry->physicalDevice, renderer->surface, &presentModeCount, details.presentModes.data()));
	}
	return details;
}
VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	for (auto& availableFormat : availableFormats)
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM)
		{
			return availableFormat;
		}
	}
	return availableFormats[0];
}
VkPresentModeKHR choose_swap_present_mode(Renderer* renderer, const std::vector<VkPresentModeKHR>& availablePresentModes)
{
	auto& window = *renderer->window;
	for (auto& availablePresentMode : availablePresentModes)
	{
		if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR && !window.vsync)
		{
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_FIFO_KHR && window.vsync)
		{
			return availablePresentMode;
		}
	}
	return VK_PRESENT_MODE_MAILBOX_KHR;
}
VkExtent2D choose_swap_extent(Renderer* renderer, VkSurfaceCapabilitiesKHR capabilities)
{
	auto& window = *renderer->window;
	VkExtent2D actualExtent = {static_cast<uint32_t>(window.width), static_cast<uint32_t>(window.height)};
	actualExtent.width =
		std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
	actualExtent.height =
		std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
	return actualExtent;
}

void ensure_command_pool(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	if (direct_registry->commandPool != VK_NULL_HANDLE)
		return;
	QueueFamilyIndices queueFamilyIndices = find_queue_families(direct_registry, renderer, direct_registry->physicalDevice);
	VkCommandPoolCreateInfo poolInfo{};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily;
	vk_check("vkCreateCommandPool", vkCreateCommandPool(direct_registry->device, &poolInfo, 0, &direct_registry->commandPool));
	return;
}

void ensure_command_buffers(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	if (!renderer->commandBuffers.empty())
		return;
	renderer->commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
	VkCommandBufferAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = direct_registry->commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = renderer->commandBuffers.size();
	vk_check("vkAllocateCommandBuffers", vkAllocateCommandBuffers(direct_registry->device, &allocInfo, &renderer->commandBuffers[0]));

	VkCommandBufferAllocateInfo computeAllocInfo{};
	computeAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	computeAllocInfo.commandPool = direct_registry->commandPool;
	computeAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	computeAllocInfo.commandBufferCount = 1;
	vk_check("vkAllocateCommandBuffers", vkAllocateCommandBuffers(direct_registry->device, &computeAllocInfo, &direct_registry->computeCommandBuffer));
	return;
}

void create_image_views(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	auto swapChainImagesSize = renderer->swapChainImages.size();
	renderer->swapChainImageViews.resize(swapChainImagesSize);
	for (uint32_t index = 0; index < swapChainImagesSize; index++)
	{
		VkImageViewCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		createInfo.image = renderer->swapChainImages[index];
		createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		createInfo.format = direct_registry->firstSurfaceFormat.format;
		createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		createInfo.subresourceRange.baseMipLevel = 0;
		createInfo.subresourceRange.levelCount = 1;
		createInfo.subresourceRange.baseArrayLayer = 0;
		createInfo.subresourceRange.layerCount = 1;
		vk_check("vkCreateImageView", vkCreateImageView(direct_registry->device, &createInfo, 0, &(renderer->swapChainImageViews[index])));
	}
	return;
}

void ensure_render_pass(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	if (direct_registry->surfaceRenderPass != VK_NULL_HANDLE)
		return;
	VkAttachmentDescription2 colorAttachment{};
	colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	colorAttachment.format = direct_registry->firstSurfaceFormat.format;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;//maxMSAASamples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	VkAttachmentReference2 colorAttachmentRef{};
	colorAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	VkSubpassDescription2 subpass{};
	subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	VkSubpassDependency2 dependency{};
	dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	std::array<VkAttachmentDescription2, 1> attachments = {colorAttachment};
	VkRenderPassCreateInfo2 renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
	renderPassInfo.attachmentCount = attachments.size();
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;
	vk_check("vkCreateRenderPass2", vkCreateRenderPass2(direct_registry->device, &renderPassInfo, 0, &direct_registry->surfaceRenderPass));
	return;
}

void create_framebuffers(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	auto swapChainImageViewsSize = renderer->swapChainImageViews.size();
	renderer->swapChainFramebuffers.resize(swapChainImageViewsSize);
	for (uint32_t index = 0; index < swapChainImageViewsSize; index++)
	{
		std::vector<VkImageView> attachments(1, VkImageView{});
		attachments[0] = renderer->swapChainImageViews[index];
		VkFramebufferCreateInfo framebufferInfo{};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = direct_registry->surfaceRenderPass;
		framebufferInfo.attachmentCount = attachments.size();
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = renderer->swapChainExtent.width;
		framebufferInfo.height = renderer->swapChainExtent.height;
		framebufferInfo.layers = 1;
		vk_check("vkCreateFramebuffer",
			vkCreateFramebuffer(direct_registry->device, &framebufferInfo, 0, &(renderer->swapChainFramebuffers[index])));
	}
	return;
}

void create_sync_objects(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	renderer->imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderer->renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
	renderer->inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
	VkSemaphoreCreateInfo semaphoreInfo{};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	VkFenceCreateInfo fenceInfo{};
	fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;
	for (size_t j = 0; j < MAX_FRAMES_IN_FLIGHT; j++)
	{
		vk_check("vkCreateSemaphore", vkCreateSemaphore(direct_registry->device, &semaphoreInfo, 0, &renderer->imageAvailableSemaphores[j]));
		vk_check("vkCreateSemaphore", vkCreateSemaphore(direct_registry->device, &semaphoreInfo, 0, &renderer->renderFinishedSemaphores[j]));
		vk_check("vkCreateFence", vkCreateFence(direct_registry->device, &fenceInfo, 0, &renderer->inFlightFences[j]));
	}
	return;
}

void pre_begin_render_pass(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	renderer->currentFrame = (renderer->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

	vk_check("vkWaitForFences", vkWaitForFences(direct_registry->device, 1,
		&renderer->inFlightFences[renderer->currentFrame], VK_TRUE, UINT64_MAX));

	vk_check("vkAcquireNextImageKHR", vkAcquireNextImageKHR(direct_registry->device,
		renderer->swapChain, UINT64_MAX, renderer->imageAvailableSemaphores[renderer->currentFrame],
		VK_NULL_HANDLE, &renderer->imageIndex));

	vk_check("vkResetFences", vkResetFences(direct_registry->device, 1, &renderer->inFlightFences[renderer->currentFrame]));
	direct_registry->commandBuffer = &renderer->commandBuffers[renderer->currentFrame];

	vk_check("vkResetCommandBuffer", vkResetCommandBuffer(*direct_registry->commandBuffer, 0));

	vk_check("vkBeginCommandBuffer", vkBeginCommandBuffer(*direct_registry->commandBuffer, &renderer->beginInfo));
}

void begin_render_pass(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	VkRenderPassBeginInfo renderPassInfo{};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = direct_registry->surfaceRenderPass;
    renderPassInfo.framebuffer = renderer->swapChainFramebuffers[renderer->imageIndex];
	renderPassInfo.renderArea.offset = {0, 0};
	renderPassInfo.renderArea.extent = renderer->swapChainExtent;
	std::array<VkClearValue, 2> clearValues{};
	clearValues[0].color = {{1.0f, 0.0f, 0.0f, 0.5f}};
	clearValues[1].depthStencil = {1.0f, 0};
	renderPassInfo.clearValueCount = clearValues.size();
	renderPassInfo.pClearValues = clearValues.data();
	vkCmdBeginRenderPass(*direct_registry->commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void post_render_pass(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	{
		vkCmdEndRenderPass(*direct_registry->commandBuffer);
		vk_check("vkEndCommandBuffer", vkEndCommandBuffer(*direct_registry->commandBuffer));
	}
	VkSemaphore waitSemaphores[] = {renderer->imageAvailableSemaphores[renderer->currentFrame]};
	{
		renderer->submitInfo.pWaitSemaphores = waitSemaphores;
		renderer->submitInfo.pWaitDstStageMask = renderer->waitStages;
		renderer->submitInfo.pCommandBuffers = direct_registry->commandBuffer;
		renderer->signalSemaphores[0] = renderer->renderFinishedSemaphores[renderer->currentFrame];
		renderer->submitInfo.signalSemaphoreCount = 1;
		renderer->submitInfo.pSignalSemaphores = renderer->signalSemaphores;
		vk_check("vkQueueSubmit", vkQueueSubmit(direct_registry->graphicsQueue, 1, &renderer->submitInfo, renderer->inFlightFences[renderer->currentFrame]));
	}
    // {
    //     vk_check("vkWaitForFences", vkWaitForFences(direct_registry->device, 1, &renderer->inFlightFences[renderer->currentFrame], VK_TRUE, UINT64_MAX));
    // }
	{
		renderer->presentInfo.pWaitSemaphores = renderer->signalSemaphores;
		renderer->swapChains[0] = {renderer->swapChain};
		renderer->presentInfo.pSwapchains = renderer->swapChains;
		renderer->presentInfo.pImageIndices = &renderer->imageIndex;
	}
}

bool swap_buffers(Renderer* renderer)
{    
	auto direct_registry = DZ_RGY.get();
	auto result = vkQueuePresentKHR(direct_registry->presentQueue, &renderer->presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
	{
		// if (!recreate_swap_chain(renderer))
		// 	return false;
		// *viewportResized = false;
	}
	else
	{
		vk_check("vkQueuePresentKHR", result);
	}
	return true;
}

void renderer_destroy(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	auto& window = *renderer->window;
	auto& device = direct_registry->device;
	vkDeviceWaitIdle(device);
	window.registry->uid_shader_map.clear();
	for (auto& drawPair : renderer->drawBuffers)
	{
		vkDestroyBuffer(device, drawPair.second.first, 0);
		vkFreeMemory(device, drawPair.second.second, 0);
	}
	for (auto& countPair : renderer->countBuffers)
	{
		vkDestroyBuffer(device, countPair.second.first, 0);
		vkFreeMemory(device, countPair.second.second, 0);
	}
	destroy_swap_chain(renderer);
	for (auto& imageAvailableSemaphore : renderer->imageAvailableSemaphores)
	{
		vkDestroySemaphore(device, imageAvailableSemaphore, 0);
	}
	for (auto& renderFinishedSemaphore : renderer->renderFinishedSemaphores)
	{
		vkDestroySemaphore(device, renderFinishedSemaphore, 0);
	}
	for (auto& inFlightFence : renderer->inFlightFences)
	{
		vkDestroyFence(device, inFlightFence, 0);
	}
	// callDestroyAtRenderPassEndOrDestroy();
}

void destroy_swap_chain(Renderer* renderer)
{
	auto direct_registry = DZ_RGY.get();
	auto& device = direct_registry->device;
	for (auto framebuffer : renderer->swapChainFramebuffers)
	{
		vkDestroyFramebuffer(device, framebuffer, 0);
	}
	renderer->swapChainFramebuffers.clear();
	for (auto imageView : renderer->swapChainImageViews)
	{
		vkDestroyImageView(device, imageView, 0);
	}
	renderer->swapChainImageViews.clear();
	if (renderer->swapChain)
	{
		vkDestroySwapchainKHR(device, renderer->swapChain, 0);
		renderer->swapChain = 0;
	}
}

uint32_t FindMemoryType(VkPhysicalDevice physicalDevice, uint32_t type_filter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(physicalDevice, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++) {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("failed to find suitable memory type!");
}


void createBuffer(Renderer* renderer,
    VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
    VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	auto direct_registry = DZ_RGY.get();
	VkBufferCreateInfo bufferInfo{};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (!vk_check("vkCreateBuffer", vkCreateBuffer(direct_registry->device, &bufferInfo, 0, &buffer)))
	{
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(direct_registry->device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo{};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(direct_registry->physicalDevice, memRequirements.memoryTypeBits, properties);

	if (!vk_check("vkAllocateMemory", vkAllocateMemory(direct_registry->device, &allocInfo, 0, &bufferMemory)))
	{
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(direct_registry->device, buffer, bufferMemory, 0);
}

bool vk_check(const char* fn, VkResult result)
{
	if (result == VK_SUCCESS)
	{
		return true;
	}

	std::string resultString;
	switch (result)
	{
	// Success Codes (should not normally be handled here unless logic changes)
	// case VK_SUCCESS: resultString = "VK_SUCCESS"; break; // Handled above
	case VK_NOT_READY:
		resultString = "VK_NOT_READY";
		break;
	case VK_TIMEOUT:
		resultString = "VK_TIMEOUT";
		break;
	case VK_EVENT_SET:
		resultString = "VK_EVENT_SET";
		break;
	case VK_EVENT_RESET:
		resultString = "VK_EVENT_RESET";
		break;
	case VK_INCOMPLETE:
		resultString = "VK_INCOMPLETE";
		break;
	case VK_SUBOPTIMAL_KHR:
		resultString = "VK_SUBOPTIMAL_KHR";
		break; // Often needs special handling (like recreating swapchain) but is not a fatal error
	case VK_PIPELINE_COMPILE_REQUIRED:
		resultString = "VK_PIPELINE_COMPILE_REQUIRED";
		break; // Or VK_PIPELINE_COMPILE_REQUIRED_EXT
	case VK_THREAD_IDLE_KHR:
		resultString = "VK_THREAD_IDLE_KHR";
		break;
	case VK_THREAD_DONE_KHR:
		resultString = "VK_THREAD_DONE_KHR";
		break;
	case VK_OPERATION_DEFERRED_KHR:
		resultString = "VK_OPERATION_DEFERRED_KHR";
		break;
	case VK_OPERATION_NOT_DEFERRED_KHR:
		resultString = "VK_OPERATION_NOT_DEFERRED_KHR";
		break;

	// Error Codes
	case VK_ERROR_OUT_OF_HOST_MEMORY:
		resultString = "VK_ERROR_OUT_OF_HOST_MEMORY";
		break;
	case VK_ERROR_OUT_OF_DEVICE_MEMORY:
		resultString = "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		break;
	case VK_ERROR_INITIALIZATION_FAILED:
		resultString = "VK_ERROR_INITIALIZATION_FAILED";
		break;
	case VK_ERROR_DEVICE_LOST:
		resultString = "VK_ERROR_DEVICE_LOST";
		break;
	case VK_ERROR_MEMORY_MAP_FAILED:
		resultString = "VK_ERROR_MEMORY_MAP_FAILED";
		break;
	case VK_ERROR_LAYER_NOT_PRESENT:
		resultString = "VK_ERROR_LAYER_NOT_PRESENT";
		break;
	case VK_ERROR_EXTENSION_NOT_PRESENT:
		resultString = "VK_ERROR_EXTENSION_NOT_PRESENT";
		break;
	case VK_ERROR_FEATURE_NOT_PRESENT:
		resultString = "VK_ERROR_FEATURE_NOT_PRESENT";
		break;
	case VK_ERROR_INCOMPATIBLE_DRIVER:
		resultString = "VK_ERROR_INCOMPATIBLE_DRIVER";
		break;
	case VK_ERROR_TOO_MANY_OBJECTS:
		resultString = "VK_ERROR_TOO_MANY_OBJECTS";
		break;
	case VK_ERROR_FORMAT_NOT_SUPPORTED:
		resultString = "VK_ERROR_FORMAT_NOT_SUPPORTED";
		break;
	case VK_ERROR_FRAGMENTED_POOL:
		resultString = "VK_ERROR_FRAGMENTED_POOL";
		break;
	case VK_ERROR_UNKNOWN:
		resultString = "VK_ERROR_UNKNOWN";
		break;
	case VK_ERROR_OUT_OF_POOL_MEMORY:
		resultString = "VK_ERROR_OUT_OF_POOL_MEMORY";
		break; // Alias for VK_ERROR_OUT_OF_POOL_MEMORY_KHR
	case VK_ERROR_INVALID_EXTERNAL_HANDLE:
		resultString = "VK_ERROR_INVALID_EXTERNAL_HANDLE";
		break; // Alias for VK_ERROR_INVALID_EXTERNAL_HANDLE_KHR
	case VK_ERROR_FRAGMENTATION:
		resultString = "VK_ERROR_FRAGMENTATION";
		break; // Alias for VK_ERROR_FRAGMENTATION_EXT
	case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
		resultString = "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
		break; // Alias for VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS_KHR
	case VK_ERROR_SURFACE_LOST_KHR:
		resultString = "VK_ERROR_SURFACE_LOST_KHR";
		break;
	case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
		resultString = "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
		break;
	case VK_ERROR_OUT_OF_DATE_KHR:
		resultString = "VK_ERROR_OUT_OF_DATE_KHR";
		break; // Often needs special handling (recreating swapchain)
	case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
		resultString = "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
		break;
	case VK_ERROR_VALIDATION_FAILED_EXT:
		resultString = "VK_ERROR_VALIDATION_FAILED_EXT";
		break;
	case VK_ERROR_INVALID_SHADER_NV:
		resultString = "VK_ERROR_INVALID_SHADER_NV";
		break;
	case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
		resultString = "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
		break;
	case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
		resultString = "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
		break;
	case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
		resultString = "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
		break;
	case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
		resultString = "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
		break;
	case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
		resultString = "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
		break;
	case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
		resultString = "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
		break;
	case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
		resultString = "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
		break;
	case VK_ERROR_NOT_PERMITTED_KHR:
		resultString = "VK_ERROR_NOT_PERMITTED_KHR";
		break; // Or VK_ERROR_NOT_PERMITTED_EXT
	case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
		resultString = "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
		break;
	case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
		resultString = "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
		break;
	case VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT:
		resultString = "VK_ERROR_INCOMPATIBLE_SHADER_BINARY_EXT";
		break;
		// Add other VkResult values as needed based on extensions you use

	default:
		resultString = "Unknown VkResult code: " + std::to_string(result);
		break;
	}

	// Print error message to standard error stream
	throw std::runtime_error("Vulkan Error: Function '" + std::string(fn) + "' failed with " + resultString + " (" + std::to_string(result) + ")");
}


uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
{
	auto direct_registry = DZ_RGY.get();
    VkPhysicalDeviceMemoryProperties mem_properties;
    vkGetPhysicalDeviceMemoryProperties(direct_registry->physicalDevice, &mem_properties);

    for (uint32_t i = 0; i < mem_properties.memoryTypeCount; i++)
    {
        if ((type_filter & (1 << i)) && (mem_properties.memoryTypes[i].propertyFlags & properties) == properties)
        {
            return i;
        }
    }

    throw std::runtime_error("Failed to find suitable memory type.");
}

VkCommandBuffer begin_single_time_commands()
{
	auto direct_registry = DZ_RGY.get();
    VkCommandBufferAllocateInfo alloc_info{};
    alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    alloc_info.commandPool = direct_registry->commandPool;
    alloc_info.commandBufferCount = 1;

    VkCommandBuffer command_buffer;
    vkAllocateCommandBuffers(direct_registry->device, &alloc_info, &command_buffer);

    VkCommandBufferBeginInfo begin_info{};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(command_buffer, &begin_info);

    return command_buffer;
}

void end_single_time_commands(VkCommandBuffer command_buffer)
{
	auto direct_registry = DZ_RGY.get();
    vkEndCommandBuffer(command_buffer);

    VkSubmitInfo submit_info{};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &command_buffer;

    vkQueueSubmit(direct_registry->graphicsQueue, 1, &submit_info, VK_NULL_HANDLE);
    vkQueueWaitIdle(direct_registry->graphicsQueue);

    vkFreeCommandBuffers(direct_registry->device, direct_registry->commandPool, 1, &command_buffer);
}
