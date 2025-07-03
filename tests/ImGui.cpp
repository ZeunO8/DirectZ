#include <DirectZ.hpp>
int main() {
    auto window = window_create({
        .title = "ImGui Test",
        .x = 0,
        .y = 240,
        .width = 640,
        .height = 480,
        .borderless = true,
        .vsync = true
    });

    auto& imgui = window_get_ImGuiLayer(window);

    imgui.AddImmediateDrawFunction(0.0f, "DockspaceRoot", [](dz::ImGuiLayer& layer)
    {
        static bool opt_fullscreen = true;
        static bool opt_is_open = true;
        static bool opt_padding = false;
        static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

        ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
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

        static bool show_demo = true;
        static bool show_custom = true;

        if (show_demo)
        {
            ImGui::ShowDemoWindow(&show_demo);
        }

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

        ImGui::End();
    });

    while (window_poll_events(window)) {
        window_render(window);
    }
}