namespace dz {
    VkDescriptorPool CreateImGuiDescriptorPool(VkDevice device)
    {
        std::vector<VkDescriptorPoolSize> pool_sizes =
        {
            { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
            { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
            { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
            { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
        };

        VkDescriptorPoolCreateInfo pool_info {};
        pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
        pool_info.maxSets = 1000 * static_cast<uint32_t>(pool_sizes.size());
        pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
        pool_info.pPoolSizes = pool_sizes.data();

        VkDescriptorPool descriptorPool = VK_NULL_HANDLE;

        VkResult result = vkCreateDescriptorPool(device, &pool_info, nullptr, &descriptorPool);

        if (result != VK_SUCCESS)
        {
            fprintf(stderr, "[ImGui] Failed to create descriptor pool: VkResult = %d\n", static_cast<int>(result));
            return VK_NULL_HANDLE;
        }

        return descriptorPool;
    }

    void DestroyImGuiDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
    {
        if (descriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
    }

    
    bool ImGuiLayer::Init() {
        if (ensured) {
            return false;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        // E.g. window.OnKeyPress = [](Key key) { ImGui::GetIO().AddKeyEvent(...); };

        ImGui_ImplVulkan_InitInfo init_info {};
        
        init_info.Instance = dr.instance;
        init_info.PhysicalDevice = dr.physicalDevice;
        init_info.Device = dr.device;
        DescriptorPool = init_info.DescriptorPool = CreateImGuiDescriptorPool(dr.device);
        init_info.QueueFamily = dr.graphicsAndComputeFamily;
        init_info.Queue = dr.graphicsQueue;
        init_info.PipelineCache = VK_NULL_HANDLE;
        init_info.MinImageCount = 2;
        init_info.ImageCount = MAX_FRAMES_IN_FLIGHT;
        init_info.Subpass = 0;
        init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
        init_info.Allocator = nullptr;
        init_info.RenderPass = dr.surfaceRenderPass;

        // 
        // 
        init_info.CheckVkResultFn = [](VkResult err)
        {
            if (err != VK_SUCCESS)
            {
                fprintf(stderr, "ImGui Vulkan error: VkResult = %d\n", static_cast<int>(err));
            }
        };

        if (!ImGui_ImplVulkan_Init(&init_info))
        {
            return false;
        }

        ensured = true;

        return true;
    }

    bool ImGuiLayer::Shutdown(DirectRegistry& direct_registry) {
        if (!ensured) {
            return false;
        }
        while (!layout_queue.empty()) {
            auto layout = layout_queue.front();
            layout_queue.pop();
            vkDestroyDescriptorSetLayout(direct_registry.device, layout, 0);
        }
		ImGui_ImplVulkan_Shutdown();
        DestroyImGuiDescriptorPool(direct_registry.device, DescriptorPool);
        ensured = false;
        return true;
    }

    void ImGuiLayer::Render(WINDOW& window) {
        auto& io = ImGui::GetIO();

        io.DisplaySize = ImVec2(*window.width, *window.height);

        if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f)
        {
            return;
        }

        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();

        for (auto& [priority, fn_map] : priority_immediate_draw_fn_map)
        {
            for (auto& [id, pair] : fn_map)
            {
                pair.second(*this);
            }
        }

        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), *(dr.commandBuffer));

        if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
        {
            ImGui::UpdatePlatformWindows();
            ImGui::RenderPlatformWindowsDefault();
        }
    }

    size_t ImGuiLayer::AddImmediateDrawFunction(float priority, const std::string& key, const ImmediateDrawFunction& fn)
    {
        auto id = GlobalUID::GetNew();
        auto& map = priority_immediate_draw_fn_map[priority];
        ImmediateDrawPair pair{key, fn};
        map[id] = pair;
        id_priority_map[id] = priority;
        return id;
    }

    bool ImGuiLayer::RemoveImmediateDrawFunction(size_t id)
    {
        auto priority_it = id_priority_map.find(id);
        if (priority_it == id_priority_map.end()) {
            return false;
        }
        auto& priority = priority_it->second;
        auto& map = priority_immediate_draw_fn_map[priority];
        auto id_it = map.find(id);
        if (id_it == map.end()) {
            return false;
        }
        map.erase(id_it);
        return true;
    }

    std::pair<VkDescriptorSetLayout, VkDescriptorSet> ImGuiLayer::CreateDescriptorSet(Image* image) {
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
        allocInfo.descriptorPool = DescriptorPool;
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

        layout_queue.push(layout);

        return {layout, descriptorSet};
    }
}