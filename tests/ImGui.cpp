#include <DirectZ.hpp>
int main() {
    auto window = window_create({
        .title = "ImGui Test",
        .x = 0,
        .y = 240,
        .width = 1280,
        .height = 768,
        .borderless = false,
        .vsync = true
    });

    auto& imgui = get_ImGuiLayer();
    
    imgui.AddImmediateDrawFunction(0.5, "Menu", [](auto& layer)
    {
        if (ImGui::BeginMainMenuBar())
        {
            if (ImGui::BeginMenu("File"))
            {
                if (ImGui::MenuItem("New", "Ctrl+N"))
                {
                    // Handle new file creation logic here
                }

                if (ImGui::MenuItem("Open", "Ctrl+O"))
                {
                    // Handle file open logic here
                }

                if (ImGui::MenuItem("Save", "Ctrl+S"))
                {
                    // Handle file save logic here
                }

                if (ImGui::MenuItem("Save As..", "Ctrl+Shift+S"))
                {
                    // Handle save as logic here
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit", "Alt+F4"))
                {
                    // Handle exit logic here
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit"))
            {
                if (ImGui::MenuItem("Undo", "Ctrl+Z"))
                {
                    // Undo logic here
                }

                if (ImGui::MenuItem("Redo", "Ctrl+Y"))
                {
                    // Redo logic here
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Cut", "Ctrl+X"))
                {
                    // Cut logic here
                }

                if (ImGui::MenuItem("Copy", "Ctrl+C"))
                {
                    // Copy logic here
                }

                if (ImGui::MenuItem("Paste", "Ctrl+V"))
                {
                    // Paste logic here
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View"))
            {
                static bool showToolbar = true;
                static bool showSidebar = true;
                static bool showStatusbar = true;

                ImGui::MenuItem("Toolbar", nullptr, &showToolbar);
                ImGui::MenuItem("Sidebar", nullptr, &showSidebar);
                ImGui::MenuItem("Status Bar", nullptr, &showStatusbar);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Window"))
            {
                if (ImGui::MenuItem("Minimize", "Ctrl+M"))
                {
                    // Minimize window logic
                }

                if (ImGui::MenuItem("Maximize", "Ctrl+Shift+M"))
                {
                    // Maximize window logic
                }

                if (ImGui::MenuItem("Restore", "Ctrl+R"))
                {
                    // Restore window size
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help"))
            {
                if (ImGui::MenuItem("Documentation", "F1"))
                {
                    // Open documentation link or popup
                }

                if (ImGui::MenuItem("About"))
                {
                    // Show about dialog
                }

                ImGui::EndMenu();
            }

            ImGui::EndMainMenuBar();
        }
    });


    imgui.AddImmediateDrawFunction(1.0f, "DockspaceRoot", [](dz::ImGuiLayer& layer)
    {
        static bool opt_fullscreen = true;
        static bool opt_is_open = true;
        static bool opt_padding = false;

        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;
        
        ImGuiWindowFlags window_flags = ImGuiWindowFlags_NoDocking;

        if (opt_fullscreen) {
            const ImGuiViewport* viewport = ImGui::GetMainViewport();
            ImGui::SetNextWindowPos(viewport->WorkPos);
            ImGui::SetNextWindowSize(viewport->WorkSize);
            ImGui::SetNextWindowViewport(viewport->ID);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
            ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
            window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
            window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus; 
        }
        else {
            dockspace_flags &= ~ImGuiDockNodeFlags_PassthruCentralNode;
        }
        if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
            window_flags |= ImGuiWindowFlags_NoBackground;
        
        if (!opt_padding) {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
        }
        ImGui::Begin("Dockspace Begin", &opt_is_open, window_flags);
        if (!opt_padding) {
            ImGui::PopStyleVar();
        }
        if (opt_fullscreen) {
            ImGui::PopStyleVar(2);
        }
        ImGuiIO& io = ImGui::GetIO();
        if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable) {
            ImGuiID dockspace_id = ImGui::GetID("DockspaceID");
            ImGui::DockSpace(dockspace_id, ImVec2(0.f, 0.f), dockspace_flags);
        }
        else {
            // Docking is disabled
            // NOTE: Not sure how to recover or what to do here
        }
    });

    imgui.AddImmediateDrawFunction(2.0, "Demo", [](auto& layer) {
        static bool show_demo = true;

        if (show_demo)
        {
            ImGui::ShowDemoWindow(&show_demo);
        }
    });

    imgui.AddImmediateDrawFunction(3.0, "HiFromDirectZ", [](auto& layer) {
        static bool show_custom = true;
        if (show_custom)
        {
            ImGui::Begin("Custom Panel", &show_custom);
            ImGui::Text("Hello from DirectZ!");
            ImGui::SliderFloat("Float Value", reinterpret_cast<float*>(&layer), 0.0f, 10.0f);
            if (ImGui::Button("Close"))
            {
                show_custom = false;
            }
            ImGui::End();
        }
    });

    imgui.AddImmediateDrawFunction(10.0, "DockspaceRootEnd", [](auto& layer) {
        ImGui::End();
    });

    while (window_poll_events(window)) {
        window_render(window);
    }
}