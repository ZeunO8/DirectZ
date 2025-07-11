#include <DirectZ.hpp>

#include <typeinfo>

Shader* default_entity_shader = 0;
uint32_t default_vertex_count = 6;

struct Entity;
struct Component;
struct System;
#define ExampleECS ECS<Entity, Component, System>

struct Entity {
    int id;
    int componentsCount;
    int components[ECS_MAX_COMPONENTS];
    
    inline static std::string GetGLSLStruct() {
        return R"(
struct Entity {
    int id;
    int componentsCount;
    int components[ECS_MAX_COMPONENTS];
};
)";
    }

    inline static std::string GetGLSLEntityVertexFunction() {
        return R"(
vec3 GetEntityVertex(in Entity entity) {
    switch (gl_VertexIndex)
    {
    case 0: return vec3(0.5, 0.5, 0);
    case 1: return vec3(-0.5, 0.5, 0);
    case 2: return vec3(-0.5, -0.5, 0);
    case 3: return vec3(-0.5, -0.5, 0);
    case 4: return vec3(0.5, -0.5, 0);
    case 5: return vec3(0.5, 0.5, 0);
    }
    return vec3(0);
}
)";
    }

    inline static std::string GetGLSLEntityVertexColorFunction() {
        return R"(
vec4 GetEntityVertexColor(in Entity entity) {
    return vec4(0, 0, 1, 0.8);
}
)";
    }

    Shader* GetShader() {
        return default_entity_shader;
    }

    uint32_t GetVertexCount() {
        return default_vertex_count;
    }
};

struct Component {
    struct ComponentData {
        int id;
        int type;
        int type_index;
        int placeholder;
    };
    using DataT = ComponentData;
    int index = -1;
    
    inline static std::string GetGLSLStruct() {
        return R"(
struct Component {
    int id;
    int type;
    int type_index;
    int placeholder;
};
)";
    }

    DataT& GetRootComponentData(ExampleECS& ecs) {
        return ecs.GetRootComponentData(index);
    }
    
    template<typename AComponentT>
    AComponentT::DataT& GetComponentData(ExampleECS& ecs) {
        return ecs.GetComponentData<AComponentT>(index);
    }

    virtual int GetPropertyIndexByName(const std::string& prop_name) = 0;
    virtual const std::vector<std::string>& GetPropertyNames() = 0;
    virtual void* GetVoidPropertyByIndex(ExampleECS& ecs, int prop_index) = 0;
    virtual void* GetVoidPropertyByName(ExampleECS& ecs, const std::string& prop_name) = 0;
    virtual const std::vector<const std::type_info*>& GetPropertyTypeinfos() = 0;
    template <typename T>
    T& GetPropertyByIndex(ExampleECS& ecs, int prop_index) {
        return *(T*)GetVoidPropertyByIndex(ecs, prop_index);
    }
    template <typename T>
    T& GetPropertyByName(ExampleECS& ecs, const std::string& prop_name) {
        return *(T*)GetVoidPropertyByName(ecs, prop_name);
    }
    virtual ~Component() = default;
};

struct PositionComponent : Component {
    using DataT = vec<float, 4>;
    int GetPropertyIndexByName(const std::string& prop_name) override {
        static std::unordered_map<std::string, int> prop_name_indexes = {
            {"x", 0},
            {"y", 1},
            {"z", 2},
            {"t", 3}
        };
        auto it = prop_name_indexes.find(prop_name);
        if (it == prop_name_indexes.end()) {
            return -1;
        }
        return it->second;
    }
    const std::vector<std::string>& GetPropertyNames() override {
        static std::vector<std::string> prop_names = {
            "x",
            "y",
            "z",
            "t"
        };
        return prop_names;
    }
    void* GetVoidPropertyByIndex(ExampleECS& ecs, int prop_index) override {
        auto& data = GetComponentData<PositionComponent>(ecs);
        switch (prop_index) {
        case 0:
            return &data[0];
        case 1:
            return &data[1];
        case 2:
            return &data[2];
        case 3:
            return &data[3];
        }
        return 0;
    }
    void* GetVoidPropertyByName(ExampleECS& ecs, const std::string& prop_name) override {
        auto prop_index = GetPropertyIndexByName(prop_name);
        if (prop_index == -1) {
            return 0;
        }
        return GetVoidPropertyByIndex(ecs, prop_index);
    }
    const std::vector<const std::type_info*>& GetPropertyTypeinfos() override {
        static const std::vector<const std::type_info*> typeinfos = {
            &typeid(float),
            &typeid(float),
            &typeid(float),
            &typeid(float)
        };
        return typeinfos;
    }
};

DEF_COMPONENT_ID(PositionComponent, 1);
DEF_COMPONENT_STRUCT_NAME(PositionComponent, "PositionComponent");
DEF_COMPONENT_STRUCT(PositionComponent, R"(
struct PositionComponent {
    float x;
    float y;
    float z;
    float t;
};
)");

// struct RotationComponent : Component {
//     float yaw;
//     float pitch;
//     float roll;
    
// };

struct System {
    int id;
    virtual ~System() = default;
};

float ORIGINAL_WINDOW_WIDTH = 1280.f;
float ORIGINAL_WINDOW_HEIGHT = 768.f;

int main() {
    auto window = window_create({
        .title = "ECS Test",
        .x = 0,
        .y = 240,
        .width = ORIGINAL_WINDOW_WIDTH,
        .height = ORIGINAL_WINDOW_HEIGHT,
        .borderless = false,
        .vsync = false
    });
    
    ExampleECS ecs([](auto& ecs){
        assert(ecs.template RegisterComponent<PositionComponent>());
        return true;
    });
    
    ecs.EnableDrawInWindow(window);

    auto eids = ecs.AddEntities(Entity{0}, Entity{0});

    auto e1_ptr = ecs.GetEntity(eids[0]);
    assert(e1_ptr);
    auto& e1 = *e1_ptr;
    auto& e1_position_component = ecs.ConstructComponent<PositionComponent>(e1.id, {1.f, 1.f, 1.f, 1.f});

    const auto& position_component_typeinfos = e1_position_component.GetPropertyTypeinfos();
    const auto& position_component_prop_names = e1_position_component.GetPropertyNames();
    for (auto& prop_name : position_component_prop_names) {
        auto prop_index = e1_position_component.GetPropertyIndexByName(prop_name);
        auto type_info = position_component_typeinfos[prop_index];
        if (*type_info == typeid(float)) {
            auto& value = e1_position_component.GetPropertyByIndex<float>(ecs, prop_index);
            std::cout << "Property " << prop_name << "<float>(" << value << ")" << std::endl;
        } else if (*type_info == typeid(int)) {
            auto& value = e1_position_component.GetPropertyByIndex<int>(ecs, prop_index);
            std::cout << "Property " << prop_name << "<int>(" << value << ")" << std::endl;
        } else if (*type_info == typeid(std::string)) {
            auto& value = e1_position_component.GetPropertyByIndex<std::string>(ecs, prop_index);
            std::cout << "Property " << prop_name << "<std::string>(" << value << ")" << std::endl;
        } else {
            // Handle unknown type
            std::cout << "Property " << prop_name << " has unknown type" << std::endl;
        }
    }

    auto e2_ptr = ecs.GetEntity(eids[1]);
    assert(e2_ptr);
    auto& e2 = *e2_ptr;
    auto& e2_position_component = ecs.ConstructComponent<PositionComponent>(e2.id, {2.f, 2.f, 2.f, 1.f});

    auto frame_image = ecs.GetFramebufferImage();

    auto& imgui = window_get_ImGuiLayer(window);

    auto [frame_image_ds_layout, frame_image_ds] = imgui.CreateDescriptorSet(frame_image);
    
    auto& window_width = *window_get_width_ref(window);
    auto& window_height = *window_get_height_ref(window);
    
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
            ImVec2 dockspaceSize = ImGui::GetContentRegionAvail();
            ImGui::DockSpace(dockspace_id, dockspaceSize, dockspace_flags);
        }
        else {
            // Docking is disabled
            // NOTE: Not sure how to recover or what to do here
        }
    });

    imgui.AddImmediateDrawFunction(2.0f, "Viewport", [&, frame_image_ds](auto& layer) {
        static bool show_viewport = true;
        if (show_viewport)
        {
            ImGui::Begin("Viewport", &show_viewport);
            ImVec2 viewportSize = ImGui::GetContentRegionAvail();
            // ImVec2 viewportSize{window_width / 2.f, window_height / 2.f};
            ImGui::Image((ImTextureID)frame_image_ds, viewportSize);
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