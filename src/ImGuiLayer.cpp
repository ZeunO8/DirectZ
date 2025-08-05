#include <dz/ImGuiLayer.hpp>
#include "fa-solid-900-ttf.cpp"
#include "WindowImpl.hpp"
#include "RendererImpl.hpp"

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

        if (result != VK_SUCCESS) {
            fprintf(stderr, "[ImGui] Failed to create descriptor pool: VkResult = %d\n", static_cast<int>(result));
            return VK_NULL_HANDLE;
        }

        return descriptorPool;
    }

    void DestroyImGuiDescriptorPool(VkDevice device, VkDescriptorPool descriptorPool)
    {
        if (descriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(device, descriptorPool, nullptr);
        }
    }

    int Platform_CreateVkSurface(
        ImGuiViewport* vp,
        ImU64 vk_inst,
        const void* vk_allocators,
        ImU64* out_vk_surface
    ) {
        auto& window = *(WINDOW*)vp->PlatformHandle;
        *out_vk_surface = (ImU64)window.renderer->surface;
        return 0;
    }

    void ImGui_ImplCustomPlatform_Init()
    {
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();

        platform_io.Monitors.clear();

        auto displays_count = displays_get_count();
        for (int i = 0; i < displays_count; ++i) {
            auto desc = displays_describe(i);

            ImGuiPlatformMonitor monitor;
            monitor.MainPos = ImVec2(desc.x, desc.y);
            monitor.MainSize = ImVec2(desc.width, desc.height);
            monitor.WorkPos = ImVec2(desc.work_x, desc.work_y);
            monitor.WorkSize = ImVec2(desc.work_width, desc.work_height);
            monitor.DpiScale = desc.dpi_scale;

            platform_io.Monitors.push_back(monitor);
        }
    
        platform_io.Platform_CreateWindow = [](ImGuiViewport* vp) {
            auto window_title = "Window #" + std::to_string(dr.window_ptrs.size() + 1);
            auto new_window = window_create({
                .title = window_title,
                .x = vp->Pos.x,
                .y = vp->Pos.y,
                .width = vp->Size.x,
                .height = vp->Size.y,
                .borderless = true,
                .vsync = true
            });
            vp->PlatformUserData = vp->PlatformHandleRaw = window_get_native_handle(new_window);
			vp->PlatformHandle = new_window;
            ImGui_ImplVulkan_ViewportData* vd = IM_NEW(ImGui_ImplVulkan_ViewportData)();
            vp->RendererUserData = vd;
            vd->WindowOwned = true;
            new_window->imguiViewport = vp;
            // vp->Hidden = false;
            // vp->Active = false;
            return;
        };

        platform_io.Platform_CreateVkSurface = Platform_CreateVkSurface;

        platform_io.Renderer_CreateWindow = nullptr;
        platform_io.Renderer_DestroyWindow = nullptr;
        platform_io.Renderer_SetWindowSize = nullptr;
        platform_io.Renderer_RenderWindow = nullptr;
        platform_io.Renderer_SwapBuffers = nullptr;

        platform_io.Platform_DestroyWindow = [](ImGuiViewport* vp) {
            window_free((WINDOW*)vp->PlatformHandle);
        };

        platform_io.Platform_ShowWindow = [](ImGuiViewport* vp) {
            return;
            // ShowNativeWindow(vp->PlatformHandle);
        };

        platform_io.Platform_SetWindowPos = [](ImGuiViewport* vp, ImVec2 pos) {
            auto window = (WINDOW*)vp->PlatformHandle;
            return window_set_position(window, pos.x, pos.y);
        };

        platform_io.Platform_GetWindowPos = [](ImGuiViewport* vp) -> ImVec2 {
            auto window = (WINDOW*)vp->PlatformHandle;
            return window_get_position(window);
        };

        platform_io.Platform_SetWindowSize = [](ImGuiViewport* vp, ImVec2 size) {
            auto window = (WINDOW*)vp->PlatformHandle;
            window_set_size(window, size.x, size.y);
        };

        platform_io.Platform_GetWindowSize = [](ImGuiViewport* vp) -> ImVec2 {
            auto& window = *(WINDOW*)vp->PlatformHandle;
            return ImVec2{*window.width, *window.height};
        };

        platform_io.Platform_SetWindowFocus = [](ImGuiViewport* vp) {
            auto window = (WINDOW*)vp->PlatformHandle;
            window_set_focused(window);
        };

        platform_io.Platform_GetWindowFocus = [](ImGuiViewport* vp) -> bool {
            auto& window = *(WINDOW*)vp->PlatformHandle;
            return *window.focused;
        };

        platform_io.Platform_GetWindowMinimized = [](ImGuiViewport* vp) -> bool {
            auto window = (WINDOW*)vp->PlatformHandle;
            bool minimized = window_get_minimized(window);
            return minimized;
        };

        platform_io.Platform_SetWindowTitle = [](ImGuiViewport* vp, const char* title) {
            auto window = (WINDOW*)vp->PlatformHandle;
            window_set_title(window, title);
        };

        platform_io.Platform_GetWindowDpiScale = [](ImGuiViewport* vp) -> float {
            // return GetWindowDpiScale(vp);
            return 1.f;
        };

        platform_io.Platform_OnChangedViewport = [](ImGuiViewport* vp) {
            return;
            // Optional
        };

        // platform_io.Platform_SetImeInputPos = [](ImGuiViewport* vp, ImVec2 pos) {
        //     // SetImePosition(vp->PlatformHandle, pos);
        // };

        ImGui::GetIO().BackendFlags |= ImGuiBackendFlags_PlatformHasViewports;
    }
    
    bool ImGuiLayer::Init() {
        if (initialized) {
            return false;
        }

        IMGUI_CHECKVERSION();
        ImGui::CreateContext();

        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

        ImGui::StyleColorsDark();

        if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
            ImGuiStyle& style = ImGui::GetStyle();
            style.WindowRounding = 0.0f;
            style.Colors[ImGuiCol_WindowBg].w = 1.0f;
        }

        io.Fonts->AddFontDefault();

        ImFontConfig icons_config;
        icons_config.MergeMode = true;
        icons_config.PixelSnapH = true;
        icons_config.FontDataOwnedByAtlas = false;
        static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
        io.Fonts->AddFontFromMemoryTTF(
            (void*)fa_solid_900_ttf,
            fa_solid_900_ttf_len,
            12.0f,
            &icons_config,
            icon_ranges
        );

        initialized = true;

        return true;
    }

    bool ImGuiLayer::VulkanInit() {
        if (vulkan_initialized) {
            return false;
        }
    
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
        init_info.CheckVkResultFn = [](VkResult err) {
            if (err != VK_SUCCESS)
            {
                fprintf(stderr, "ImGui Vulkan error: VkResult = %d\n", static_cast<int>(err));
            }
        };

        if (!ImGui_ImplVulkan_Init(&init_info)) {
            return false;
        }
        
        ImGui_ImplCustomPlatform_Init();

        vulkan_initialized = true;

        return true;
    }

    bool ImGuiLayer::Shutdown(DirectRegistry& direct_registry) {
        if (!initialized || !vulkan_initialized) {
            return false;
        }
		ImGui_ImplVulkan_Shutdown();
        DestroyImGuiDescriptorPool(direct_registry.device, DescriptorPool);
        ImGui::DestroyContext();
        initialized = false;
        vulkan_initialized = false;
        return true;
    }

    void ImGuiLayer::Render(WINDOW& window) {
        auto root_window = &window == dr.window_ptrs[0];
        auto& io = ImGui::GetIO();

        io.DisplaySize = ImVec2(*window.width, *window.height);

        if (io.DisplaySize.x <= 0.0f || io.DisplaySize.y <= 0.0f) {
            return;
        }

        if (root_window) {
            auto& float_frametime = *window.float_frametime;
            auto framerate = (1.f / float_frametime);
            io.Framerate = framerate;
            io.DeltaTime = float_frametime;

            ImGui_ImplVulkan_NewFrame();
            ImGui::NewFrame();
            ImGuizmo::BeginFrame();
            
            for (auto& [priority, fn_map] : priority_immediate_draw_fn_map)
                for (auto& [id, pair] : fn_map)
                    pair.second(*this);
    
            ImGui::Render();

            if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
                ImGui::UpdatePlatformWindows();
                ImGui::RenderPlatformWindowsDefault(nullptr, nullptr);
            }
        }

        auto DrawData = GetDrawData(window);

        if (DrawData)
            ImGui_ImplVulkan_RenderDrawData(DrawData, *(dr.commandBuffer));
    }

    ImDrawData* ImGuiLayer::GetDrawData(WINDOW& window) {
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        for (int i = 0; i < platform_io.Viewports.Size; ++i) {
            ImGuiViewport* viewport = platform_io.Viewports[i];
            if (viewport->PlatformHandle == &window) {
                if (i == 0) {
                    return ImGui::GetDrawData();
                }
                else {
                    return viewport->DrawData;
                }
            }
        }
        return nullptr;
    }

    ImGuiViewport* ImGuiLayer::GetViewport(WINDOW* window) {
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        for (int i = 0; i < platform_io.Viewports.Size; ++i) {
            ImGuiViewport* viewport = platform_io.Viewports[i];
            if (viewport->PlatformHandle == window)
                return viewport;
        }
        return nullptr;
    }

    void ImGuiLayer::FocusWindow(WINDOW* window, bool focused) {
        ImGuiPlatformIO& platform_io = ImGui::GetPlatformIO();
        for (int i = 0; i < platform_io.Viewports.Size; ++i) {
            ImGuiViewport* viewport = platform_io.Viewports[i];
            if (viewport->PlatformHandle && viewport->PlatformHandle == window)
            {
                if (focused) {
                    viewport->Flags |= ImGuiViewportFlags_IsFocused;
                    viewport->Flags &= ~ImGuiViewportFlags_NoInputs;
                }
                else {
                    viewport->Flags &= ~ImGuiViewportFlags_IsFocused;
                    viewport->Flags |= ImGuiViewportFlags_NoInputs;
                }
            }
            else if (focused) {
                viewport->Flags &= ~ImGuiViewportFlags_IsFocused;
                viewport->Flags |= ImGuiViewportFlags_NoInputs;
                if (viewport->PlatformHandle) {
                    auto& other_window = *(WINDOW*)viewport->PlatformHandle;
                    *other_window.focused = false;
                }
            }
        }
    }

    size_t ImGuiLayer::AddImmediateDrawFunction(float priority, const std::string& key, const ImmediateDrawFunction& fn)
    {
        auto id = GlobalUID::GetNew("ImGuiLayer:AddImmediateDrawFunction");
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
}