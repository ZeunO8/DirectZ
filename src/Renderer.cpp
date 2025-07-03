namespace dz {
	#include "RendererImpl.hpp"
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
	VkSampleCountFlagBits get_max_usable_sample_count(DirectRegistry* direct_registry, Renderer* renderer);
	void direct_registry_ensure_physical_device(DirectRegistry* direct_registry, Renderer* renderer);
	uint32_t rate_device_suitability(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device);
	bool is_device_suitable(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device);
	QueueFamilyIndices find_queue_families(DirectRegistry* direct_registry, Renderer* renderer, VkPhysicalDevice device);
	void direct_registry_ensure_logical_device(DirectRegistry* direct_registry, Renderer* renderer);
	SwapChainSupportDetails query_swap_chain_support(Renderer* renderer, VkPhysicalDevice device);
	VkSurfaceFormatKHR choose_swap_surface_format(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR choose_swap_present_mode(Renderer* renderer, const std::vector<VkPresentModeKHR>& availablePresentModes);
	VkExtent2D choose_swap_extent(Renderer* renderer, VkSurfaceCapabilitiesKHR capabilities);
	void ensure_command_pool(Renderer* renderer);
	void ensure_command_buffers(Renderer* renderer);
	void ensure_render_pass(Renderer* renderer);
	void create_sync_objects(Renderer* renderer);
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
		auto direct_registry = get_direct_registry();
		direct_registry_ensure_instance(direct_registry);
		create_surface(renderer);
		direct_registry_ensure_physical_device(direct_registry, renderer);
		direct_registry_ensure_logical_device(direct_registry, renderer);
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


	std::string getLastErrorAsString()
	{
	#if defined(_WIN32)
		DWORD errorMessageID = GetLastError();
		if (errorMessageID == 0)
			return "No error"; // No error

		LPSTR messageBuffer = nullptr;
		size_t size =
			FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
											NULL, errorMessageID, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPSTR)&messageBuffer, 0, NULL);

		std::string message(messageBuffer, size);
		LocalFree(messageBuffer);
		return message;
	#else
		return dlerror();
	#endif
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
		{ 
			std::cerr << "VkDebug(" << messageSeverity << "): " << std::string(pCallbackData->pMessage) << std::endl <<
			getLastErrorAsString() << std::endl;
		}
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

	bool is_any_vulkan_implementation_available(const std::vector<const char*>& possible_extensions, std::vector<const char*>& available_extensions)
	{
		uint32_t count = 0;
		VkResult res = vkEnumerateInstanceExtensionProperties(nullptr, &count, nullptr);
		if (res != VK_SUCCESS || count == 0)
		{
			std::cerr << "Unable to enumerate instance extension properties" << std::endl;
			return false;
		}

		std::vector<VkExtensionProperties> extensions(count);
		res = vkEnumerateInstanceExtensionProperties(nullptr, &count, extensions.data());
		if (res != VK_SUCCESS)
		{
			std::cerr << "Unable to enumerate instance extension properties" << std::endl;
			return false;
		}

		for (const auto& ext : extensions)
		{
			for (auto& pos_ext : possible_extensions)
			{
				if (strcmp(ext.extensionName, pos_ext) == 0)
				{
					available_extensions.push_back(pos_ext);
					break;
				}
			}
		}

		return !available_extensions.empty();
	}

	void direct_registry_ensure_instance(DirectRegistry* direct_registry)
	{
		if (direct_registry->instance != VK_NULL_HANDLE)
			return;

		std::vector<const char*> possible_extensions;
		std::vector<const char*> extensions;
		possible_extensions.push_back(VK_KHR_SURFACE_EXTENSION_NAME);

	#if defined(VK_USE_PLATFORM_ANDROID_KHR)
		possible_extensions.push_back(VK_KHR_ANDROID_SURFACE_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_WAYLAND_KHR)
		possible_extensions.push_back(VK_KHR_WAYLAND_SURFACE_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_WIN32_KHR)
		possible_extensions.push_back(VK_KHR_WIN32_SURFACE_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_XCB_KHR)
		possible_extensions.push_back(VK_KHR_XCB_SURFACE_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_METAL_EXT)
		possible_extensions.push_back(VK_EXT_METAL_SURFACE_EXTENSION_NAME);
	#elif defined(VK_USE_PLATFORM_MACOS_MVK)
		possible_extensions.push_back(VK_MVK_MACOS_SURFACE_EXTENSION_NAME);
	#endif

	#if defined(MACOS)
		possible_extensions.push_back("VK_KHR_portability_enumeration");
	#endif

	#ifndef NDEBUG
	#ifndef __ANDROID__
		possible_extensions.push_back("VK_EXT_debug_utils");
	#endif
	#endif

		std::cout << "Possible Extensions:" << std::endl;
		for (auto& pos_ext : possible_extensions)
		{
			std::cout << "\t" << pos_ext << std::endl;
		}

		if (!is_any_vulkan_implementation_available(possible_extensions, extensions))
		{
			std::cout << "No Vulkan implementation found. Falling back to SwiftShader." << std::endl;
			append_vk_icd_filename((getProgramDirectoryPath() / "SwiftShader" / "vk_swiftshader_icd.json").string());
			direct_registry->swiftshader_fallback = true;
		}

		std::cout << std::endl << "Available Extensions:" << std::endl;
		for (auto& avail_ext : extensions)
		{
			std::cout << "\t" << avail_ext << std::endl;
		}

		VkApplicationInfo appInfo{};
		appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		appInfo.pApplicationName = "DirectZ Application";
		appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		appInfo.pEngineName = "DirectZ";
		appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	#ifdef __ANDROID__
		appInfo.apiVersion = VK_API_VERSION_1_1;
	#else
		appInfo.apiVersion = VK_API_VERSION_1_2;
	#endif

		VkInstanceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo = &appInfo;

	#if defined(MACOS)
		createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	#endif

		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();

		VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
		std::vector<const char*> layers;
	#ifndef NDEBUG
	#ifndef __ANDROID__
		if (get_env("DISABLE_VALIDATION_LAYERS").empty())
		{
			if (check_validation_layers_support())
			{
				layers.push_back("VK_LAYER_KHRONOS_validation");
				createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
				createInfo.ppEnabledLayerNames = layers.data();
				populate_debug_messenger_create_info(debugCreateInfo);
				createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*)&debugCreateInfo;
			}
			else
			{
				std::cout << "Validation layers requested, but not available" << std::endl;
				createInfo.enabledLayerCount = 0;
			}
		}
	#endif
	#endif
		auto instance_create_result = vkCreateInstance(&createInfo, nullptr, &direct_registry->instance);
		if (instance_create_result == VK_SUCCESS)
			return;
		vk_log("vkCreateInstance", instance_create_result);
		if (!direct_registry->swiftshader_fallback)
		{
			std::cout << "Vulkan instance creation failed. Falling back to SwiftShader." << std::endl;
			append_vk_icd_filename((getProgramDirectoryPath() / "SwiftShader" / "vk_swiftshader_icd.json").string());
			direct_registry->swiftshader_fallback = true;
			direct_registry_ensure_instance(direct_registry);
		}
		else
		{
			std::cerr << "Failed to create Vulkan instance even with SwiftShader fallback." << std::endl;
			assert(false);
		}
	}

	#ifndef MACOS
	void create_surface(Renderer* renderer)
	{
		auto& window = *renderer->window;
		auto& dr = *window.registry;
		auto& windowType = dr.windowType;
	#if defined(__linux__) && !defined(__ANDROID__)
		VkXcbSurfaceCreateInfoKHR surfaceCreateInfo{};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.connection = window.connection;
		surfaceCreateInfo.window = window.window;
		vk_check("vkCreateXcbSurfaceKHR",
			vkCreateXcbSurfaceKHR(dr.instance, &surfaceCreateInfo, 0, &renderer->surface));
	#elif defined(ANDROID)
		VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo{};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.window = window.android_window;
		vk_check("vkCreateAndroidSurfaceKHR",
			vkCreateAndroidSurfaceKHR(dr.instance, &surfaceCreateInfo, 0, &renderer->surface));
	#elif defined(_WIN32)
		VkWin32SurfaceCreateInfoKHR surfaceCreateInfo{};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
		surfaceCreateInfo.hinstance = window.hInstance;
		surfaceCreateInfo.hwnd = window.hwnd;
		vk_check("vkCreateWin32SurfaceKHR",
			vkCreateWin32SurfaceKHR(dr.instance, &surfaceCreateInfo, 0, &renderer->surface));
	#elif defined(MACOS)
		CAMetalLayer* metalLayer = (CAMetalLayer*)[(NSView*)window.nsView layer];

		VkMetalSurfaceCreateInfoEXT surfaceCreateInfo{};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
		surfaceCreateInfo.pNext = nullptr;
		surfaceCreateInfo.flags = 0;
		surfaceCreateInfo.pLayer = (__bridge void*)metalLayer;

		vk_check("vkCreateMetalSurfaceEXT",
			vkCreateMetalSurfaceEXT(dr.instance, &surfaceCreateInfo, nullptr, &renderer->surface));
		// VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo{};
		// surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
		// surfaceCreateInfo.pView = window.nsView;
		// vk_check("vkCreateMacOSSurfaceMVK",
		// 	vkCreateMacOSSurfaceMVK(dr.instance, &surfaceCreateInfo, 0, &renderer->surface));
	#endif
	}
	#endif

	void Renderer::destroy_surface()
	{
		auto& dr = *get_direct_registry();
		vkDeviceWaitIdle(dr.device);
		if (surface != VK_NULL_HANDLE)
		{
			vkDestroySurfaceKHR(dr.instance, surface, nullptr);
			surface = VK_NULL_HANDLE;
		}
		cleanup_swapchain();
	}

	void Renderer::cleanup_swapchain()
	{
		auto& dr = *get_direct_registry();
		for (auto framebuffer : swapChainFramebuffers)
		{
			vkDestroyFramebuffer(dr.device, framebuffer, 0);
		}
		swapChainFramebuffers.clear();
		for (auto imageView : swapChainImageViews)
		{
			vkDestroyImageView(dr.device, imageView, 0);
		}
		swapChainImageViews.clear();
		if (swapChain)
		{
			vkDestroySwapchainKHR(dr.device, swapChain, 0);
			swapChain = 0;
		}
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
		float queuePriority = 1.0f;
		std::unordered_set<int32_t> seen;
		for (auto& queueFamily : uniqueQueueFamilies)
		{
			if (seen.insert(queueFamily).second)
			{
				VkDeviceQueueCreateInfo queueCreateInfo{};
				queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueCreateInfo.queueFamilyIndex = queueFamily;
				queueCreateInfo.queueCount = 1;
				queueCreateInfo.pQueuePriorities = &queuePriority;
				queueCreateInfos.push_back(queueCreateInfo);
			}
		}

		std::vector<const char*> extensions;
		uint32_t extensionCount = 0;
		vk_check("vkEnumerateDeviceExtensionProperties", vkEnumerateDeviceExtensionProperties(direct_registry->physicalDevice, nullptr, &extensionCount, nullptr));
		std::vector<VkExtensionProperties> deviceExtensions(extensionCount);
		vk_check("vkEnumerateDeviceExtensionProperties", vkEnumerateDeviceExtensionProperties(direct_registry->physicalDevice, nullptr, &extensionCount, deviceExtensions.data()));

		auto is_supported = [&](const char* name) {
			return std::any_of(deviceExtensions.begin(), deviceExtensions.end(), [&](const VkExtensionProperties& p) {
				return strcmp(p.extensionName, name) == 0;
			});
		};

		if (is_supported("VK_KHR_portability_subset"))
			extensions.push_back("VK_KHR_portability_subset");
		if (is_supported(VK_KHR_SWAPCHAIN_EXTENSION_NAME))
			extensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
		if (is_supported("VK_KHR_maintenance1"))
			extensions.push_back("VK_KHR_maintenance1");
		if (is_supported("VK_KHR_swapchain"))
			extensions.push_back("VK_KHR_swapchain");

		VkPhysicalDeviceShaderDrawParametersFeatures shaderDrawParamsFeatures_query{};
		shaderDrawParamsFeatures_query.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETERS_FEATURES;

	#ifndef __ANDROID__
		VkPhysicalDeviceVulkan12Features vulkan12Features{};
		vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
		vulkan12Features.pNext = &shaderDrawParamsFeatures_query;
	#else
		void* pNextFeatureChain = &shaderDrawParamsFeatures_query;
	#endif

		VkPhysicalDeviceFeatures2 supportedFeatures{};
		supportedFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
	#ifndef __ANDROID__
		supportedFeatures.pNext = &vulkan12Features;
	#else
		supportedFeatures.pNext = pNextFeatureChain;
	#endif

		vkGetPhysicalDeviceFeatures2(direct_registry->physicalDevice, &supportedFeatures);

		// Prepare desired features but only enable if supported
		shaderDrawParamsFeatures_query.shaderDrawParameters = shaderDrawParamsFeatures_query.shaderDrawParameters ? VK_TRUE : VK_FALSE;
	#ifndef __ANDROID__
		renderer->supportsIndirectCount = (vulkan12Features.drawIndirectCount = vulkan12Features.drawIndirectCount ? VK_TRUE : VK_FALSE);
		vulkan12Features.descriptorBindingVariableDescriptorCount = VK_FALSE;
		vulkan12Features.descriptorBindingUniformBufferUpdateAfterBind = VK_FALSE;
		vulkan12Features.descriptorBindingPartiallyBound = VK_FALSE;
		vulkan12Features.descriptorBindingStorageBufferUpdateAfterBind = VK_FALSE;
		vulkan12Features.descriptorBindingUniformTexelBufferUpdateAfterBind = VK_FALSE;
		vulkan12Features.descriptorBindingStorageTexelBufferUpdateAfterBind = VK_FALSE;
	#endif

		supportedFeatures.features.sampleRateShading = supportedFeatures.features.sampleRateShading ? VK_TRUE : VK_FALSE;
		supportedFeatures.features.depthClamp = supportedFeatures.features.depthClamp ? VK_TRUE : VK_FALSE;
		supportedFeatures.features.depthBiasClamp = supportedFeatures.features.depthBiasClamp ? VK_TRUE : VK_FALSE;
		supportedFeatures.features.samplerAnisotropy = supportedFeatures.features.samplerAnisotropy ? VK_TRUE : VK_FALSE;
		supportedFeatures.features.robustBufferAccess = VK_FALSE;
		supportedFeatures.features.multiDrawIndirect = supportedFeatures.features.multiDrawIndirect ? VK_TRUE : VK_FALSE;
		supportedFeatures.features.drawIndirectFirstInstance = supportedFeatures.features.drawIndirectFirstInstance ? VK_TRUE : VK_FALSE;

		VkDeviceCreateInfo createInfo{};
		createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
		createInfo.pQueueCreateInfos = queueCreateInfos.data();
		createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		createInfo.ppEnabledExtensionNames = extensions.data();
		createInfo.enabledLayerCount = 0;
		createInfo.pNext = &supportedFeatures;

		vk_check("vkCreateDevice", vkCreateDevice(direct_registry->physicalDevice, &createInfo, nullptr, &direct_registry->device));

		direct_registry->graphicsAndComputeFamily = indices.graphicsAndComputeFamily;
		direct_registry->presentFamily = indices.presentFamily;

		vkGetDeviceQueue(direct_registry->device, direct_registry->graphicsAndComputeFamily, 0, &direct_registry->graphicsQueue);
		vkGetDeviceQueue(direct_registry->device, direct_registry->presentFamily, 0, &direct_registry->presentQueue);
		vkGetDeviceQueue(direct_registry->device, direct_registry->graphicsAndComputeFamily, 0, &direct_registry->computeQueue);
	}

	bool create_swap_chain(Renderer* renderer)
	{
		auto direct_registry = get_direct_registry();
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
		auto direct_registry = get_direct_registry();
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
		return availablePresentModes.empty() ? VK_PRESENT_MODE_FIFO_KHR : availablePresentModes[0];
	}
	VkExtent2D choose_swap_extent(Renderer* renderer, VkSurfaceCapabilitiesKHR capabilities)
	{
		auto& window = *renderer->window;
		VkExtent2D actualExtent = {static_cast<uint32_t>(*window.width), static_cast<uint32_t>(*window.height)};
		auto width = actualExtent.width =
			std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
		auto height = actualExtent.height =
			std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);
		if (capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
			capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR)
		{
			actualExtent.height = width;
			actualExtent.width = height;
		}
		renderer->currentTransform = capabilities.currentTransform;
		return actualExtent;
	}

	void ensure_command_pool(Renderer* renderer)
	{
		auto direct_registry = get_direct_registry();
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
		auto direct_registry = get_direct_registry();
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
		auto direct_registry = get_direct_registry();
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
		auto direct_registry = get_direct_registry();
		if (direct_registry->surfaceRenderPass != VK_NULL_HANDLE)
			return;
	#if defined(__ANDROID__)
		VkAttachmentDescription colorAttachment{};
	#else
		VkAttachmentDescription2 colorAttachment{};
		colorAttachment.sType = VK_STRUCTURE_TYPE_ATTACHMENT_DESCRIPTION_2;
	#endif
		colorAttachment.format = direct_registry->firstSurfaceFormat.format;
		colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;//maxMSAASamples;
		colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_STORE;
		colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	#if defined(__ANDROID__)
		VkAttachmentReference colorAttachmentRef{};
	#else
		VkAttachmentReference2 colorAttachmentRef{};
		colorAttachmentRef.sType = VK_STRUCTURE_TYPE_ATTACHMENT_REFERENCE_2;
	#endif
		colorAttachmentRef.attachment = 0;
		colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	#if defined(__ANDROID__)
		VkSubpassDescription subpass{};
	#else
		VkSubpassDescription2 subpass{};
		subpass.sType = VK_STRUCTURE_TYPE_SUBPASS_DESCRIPTION_2;
	#endif
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colorAttachmentRef;
	#if defined(__ANDROID__)
		VkSubpassDependency dependency{};
	#else
		VkSubpassDependency2 dependency{};
		dependency.sType = VK_STRUCTURE_TYPE_SUBPASS_DEPENDENCY_2;
	#endif
		dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
		dependency.dstSubpass = 0;
		dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.srcAccessMask = 0;
		dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
		dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	#if defined(__ANDROID__)
		std::array<VkAttachmentDescription, 1> attachments = {colorAttachment};
		VkRenderPassCreateInfo renderPassInfo{};
	#else
		std::array<VkAttachmentDescription2, 1> attachments = {colorAttachment};
		VkRenderPassCreateInfo2 renderPassInfo{};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO_2;
	#endif
		renderPassInfo.attachmentCount = attachments.size();
		renderPassInfo.pAttachments = attachments.data();
		renderPassInfo.subpassCount = 1;
		renderPassInfo.pSubpasses = &subpass;
		renderPassInfo.dependencyCount = 1;
		renderPassInfo.pDependencies = &dependency;
	#if defined(__ANDROID__)
		vk_check("vkCreateRenderPass", vkCreateRenderPass(direct_registry->device, &renderPassInfo, 0, &direct_registry->surfaceRenderPass));
	#else
		vk_check("vkCreateRenderPass2", vkCreateRenderPass2(direct_registry->device, &renderPassInfo, 0, &direct_registry->surfaceRenderPass));
	#endif
		return;
	}

	void create_framebuffers(Renderer* renderer)
	{
		auto direct_registry = get_direct_registry();
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
		auto direct_registry = get_direct_registry();
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
		auto direct_registry = get_direct_registry();
		renderer->currentFrame = (renderer->currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;

		vk_check("vkWaitForFences", vkWaitForFences(direct_registry->device, 1,
			&renderer->inFlightFences[renderer->currentFrame], VK_TRUE, UINT64_MAX));

		VkResult res = vkAcquireNextImageKHR(direct_registry->device,
			renderer->swapChain, UINT64_MAX,
			renderer->imageAvailableSemaphores[renderer->currentFrame],
			VK_NULL_HANDLE, &renderer->imageIndex);

		if (res == VK_ERROR_OUT_OF_DATE_KHR)
		{
			recreate_swap_chain(renderer);
			return;
		}
		else if (res != VK_SUCCESS && res != VK_SUBOPTIMAL_KHR)
		{
			vk_check("vkAcquireNextImageKHR", res);
		}

		vk_check("vkResetFences", vkResetFences(direct_registry->device, 1, &renderer->inFlightFences[renderer->currentFrame]));
		direct_registry->commandBuffer = &renderer->commandBuffers[renderer->currentFrame];

		vk_check("vkResetCommandBuffer", vkResetCommandBuffer(*direct_registry->commandBuffer, 0));

		vk_check("vkBeginCommandBuffer", vkBeginCommandBuffer(*direct_registry->commandBuffer, &renderer->beginInfo));
	}

	void begin_render_pass(Renderer* renderer)
	{
		auto direct_registry = get_direct_registry();
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
		auto direct_registry = get_direct_registry();
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
		{
			vk_check("vkWaitForFences", vkWaitForFences(direct_registry->device, 1, &renderer->inFlightFences[renderer->currentFrame], VK_TRUE, UINT64_MAX));
		}
		{
			renderer->presentInfo.pWaitSemaphores = renderer->signalSemaphores;
			renderer->swapChains[0] = {renderer->swapChain};
			renderer->presentInfo.pSwapchains = renderer->swapChains;
			renderer->presentInfo.pImageIndices = &renderer->imageIndex;
		}
	}

	void recreate_swap_chain(Renderer* renderer)
	{
		destroy_swap_chain(renderer);
		create_swap_chain(renderer);
		create_image_views(renderer);
		create_framebuffers(renderer);
		std::cout << "Recreated Swap Chain" << std::endl;
	}

	bool swap_buffers(Renderer* renderer)
	{    
		auto direct_registry = get_direct_registry();
		auto result = vkQueuePresentKHR(direct_registry->presentQueue, &renderer->presentInfo);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR)
		{
			recreate_swap_chain(renderer);
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
		auto direct_registry = get_direct_registry();
		auto& window = *renderer->window;
		auto& device = direct_registry->device;
		if (device == VK_NULL_HANDLE)
			return;
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
		auto direct_registry = get_direct_registry();
		auto& device = direct_registry->device;
		if (device == VK_NULL_HANDLE)
			return;
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
		auto direct_registry = get_direct_registry();
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

	std::string vk_result_string(VkResult result)
	{
		std::string resultString;
		switch (result)
		{
		case VK_SUCCESS:
			resultString = "VK_SUCCESS";
			break;
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
		return resultString;
	}

	bool vk_check(const char* fn, VkResult result)
	{
		if (result == VK_SUCCESS)
		{
			return true;
		}

		auto resultString = vk_result_string(result);

		// Print error message to standard error stream
		throw std::runtime_error("Vulkan Error: Function '" + std::string(fn) + "' failed with " + resultString + " (" + std::to_string(result) + ")");
	}

	void vk_log(const char* fn, VkResult result)
	{	
		auto resultString = vk_result_string(result);
		std::cerr << "Vulkan Error: Function '" << std::string(fn) << "' failed with " << resultString << " (" + std::to_string(result) << ")";
	}

	uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags properties)
	{
		auto direct_registry = get_direct_registry();
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
		auto direct_registry = get_direct_registry();
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
		auto direct_registry = get_direct_registry();
		vkEndCommandBuffer(command_buffer);

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.commandBufferCount = 1;
		submit_info.pCommandBuffers = &command_buffer;

		vkQueueSubmit(direct_registry->graphicsQueue, 1, &submit_info, VK_NULL_HANDLE);
		vkQueueWaitIdle(direct_registry->graphicsQueue);

		vkFreeCommandBuffers(direct_registry->device, direct_registry->commandPool, 1, &command_buffer);
	}
}