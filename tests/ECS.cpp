#include <DirectZ.hpp>

#include <typeinfo>

Shader* default_entity_shader = 0;
uint32_t default_vertex_count = 6;

struct Entity;
struct Component;
struct System;
#define ExampleECS ECS<Entity, Component, System>

struct Entity {
    int id = 0;
    int componentsCount = 0;
    int components[ECS_MAX_COMPONENTS] = {0};
    
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

enum class ComponentTypeHint {
    FLOAT,
    VEC4,
    VEC4_RGBA,
    MAT4,
    STRUCT
};

struct Component {
    struct ComponentData {
        int index;
        int type;
        int type_index;
        int data_size;
    };
    using DataT = ComponentData;
    int id = 0;
    int index = -1;
    
    inline static std::string GetGLSLStruct() {
        return R"(
struct Component {
    int index;
    int type;
    int type_index;
    int data_size;
};
)";
    }

    inline static std::string GetGLSLMethods() {
        return R"(
Component GetComponentByType(in Entity entity, int type) {
    for (int i = 0; i < entity.componentsCount; i++) {
        int component_index = entity.components[i];
        if (Components.data[component_index].type == type) {
            return Components.data[component_index];
        }
    }
    Component DefaultComponent = Component(-1, -1, -1, -1);
    return DefaultComponent;
}
bool HasComponentWithType(in Entity entity, int type, out int t_component_index) {
    for (int i = 0; i < entity.componentsCount; i++) {
        int component_index = entity.components[i];
        if (Components.data[component_index].type == type) {
            t_component_index = Components.data[component_index].type_index;
            return true;
        }
    }
    t_component_index = -1;
    return false;
}
)";
    }

    DataT& GetRootComponentData(ExampleECS& ecs) {
        return ecs.GetRootComponentData(index);
    }
    
    template<typename AComponentT>
    AComponentT::DataT& GetComponentData(ExampleECS& ecs) {
        return ecs.GetComponentData<AComponentT>(index);
    }

    virtual const std::string& GetComponentName() = 0;
    virtual ComponentTypeHint GetComponentTypeHint() {
        return ComponentTypeHint::STRUCT;
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

// Component Helper defines (for reflection)

#define DEF_GET_COMPONENT_NAME(TYPE) const std::string& GetComponentName() override { \
    return ComponentStructName<TYPE>::string; \
}

#define DEF_GET_PROPERTY_INDEX_BY_NAME(INDEXES) int GetPropertyIndexByName(const std::string& prop_name) override { \
    auto it = INDEXES.find(prop_name); \
    if (it == INDEXES.end()) \
        return -1; \
    return it->second.first; \
}

#define DEF_GET_PROPERTY_NAMES(NAMES) const std::vector<std::string>& GetPropertyNames() override { \
    return NAMES; \
}

#define DEF_GET_VOID_PROPERTY_BY_INDEX(NAME_INDEXES, INDEX_NAMES) void* GetVoidPropertyByIndex(ExampleECS& ecs, int prop_index) override { \
    auto& data = GetComponentData<PositionComponent>(ecs); \
    auto index_it = INDEX_NAMES.find(prop_index); \
    if (index_it == INDEX_NAMES.end()) \
        return nullptr; \
    auto& prop_name = index_it->second; \
    auto it = NAME_INDEXES.find(prop_name); \
    if (it == NAME_INDEXES.end()) \
        return nullptr; \
    auto offset = it->second.second; \
    return ((char*)&data) + offset; \
}

#define DEF_GET_VOID_PROPERTY_BY_NAME void* GetVoidPropertyByName(ExampleECS& ecs, const std::string& prop_name) override { \
    auto prop_index = GetPropertyIndexByName(prop_name); \
    if (prop_index == -1) \
        return 0; \
    return GetVoidPropertyByIndex(ecs, prop_index); \
}

#define DEF_GET_PROPERTY_TYPEINFOS(TYPEINFOS) const std::vector<const std::type_info*>& GetPropertyTypeinfos() override { \
    return TYPEINFOS; \
}

/// Position Component

struct PositionComponent;
DEF_COMPONENT_ID(PositionComponent, 1);
DEF_COMPONENT_STRUCT_NAME(PositionComponent, "PositionComponent");
DEF_COMPONENT_STRUCT(PositionComponent, R"(
#define PositionComponent vec4
)");
DEF_COMPONENT_GLSL_METHODS(PositionComponent, "");
DEF_COMPONENT_GLSL_MAIN(PositionComponent, R"(
        final_position += vec4(positioncomponent.x, positioncomponent.y, positioncomponent.z, 0);  
)");

struct PositionComponent : Component {
    using DataT = vec<float, 4>;
    inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
        {"x", {0, 0}},
        {"y", {1, 4}},
        {"z", {2, 8}},
        {"t", {3, 12}}
    };
    inline static std::unordered_map<int, std::string> prop_index_names = {
        {0, "x"},
        {1, "y"},
        {2, "z"},
        {3, "t"}
    };
    inline static std::vector<std::string> prop_names = {
        "x",
        "y",
        "z",
        "t"
    };
    inline static const std::vector<const std::type_info*> typeinfos = {
        &typeid(float),
        &typeid(float),
        &typeid(float),
        &typeid(float)
    };
    DEF_GET_COMPONENT_NAME(PositionComponent);
    ComponentTypeHint GetComponentTypeHint() override {
        return ComponentTypeHint::VEC4;
    }
    DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
    DEF_GET_PROPERTY_NAMES(prop_names);
    DEF_GET_VOID_PROPERTY_BY_INDEX(prop_name_indexes, prop_index_names);
    DEF_GET_VOID_PROPERTY_BY_NAME;
    DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
};

/// Position Component

struct ColorComponent;
DEF_COMPONENT_ID(ColorComponent, 2);
DEF_COMPONENT_STRUCT_NAME(ColorComponent, "ColorComponent");
DEF_COMPONENT_STRUCT(ColorComponent, R"(
#define ColorComponent vec4
)");
DEF_COMPONENT_GLSL_METHODS(ColorComponent, "");
DEF_COMPONENT_GLSL_MAIN(ColorComponent, R"(
        final_color = colorcomponent;
)");

struct ColorComponent : Component {
    using DataT = vec<float, 4>;
    inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
        {"r", {0, 0}},
        {"g", {1, 4}},
        {"b", {2, 8}},
        {"a", {3, 12}}
    };
    inline static std::unordered_map<int, std::string> prop_index_names = {
        {0, "r"},
        {1, "g"},
        {2, "b"},
        {3, "a"}
    };
    inline static std::vector<std::string> prop_names = {
        "r",
        "g",
        "b",
        "a"
    };
    inline static const std::vector<const std::type_info*> typeinfos = {
        &typeid(float),
        &typeid(float),
        &typeid(float),
        &typeid(float)
    };
    DEF_GET_COMPONENT_NAME(ColorComponent);
    ComponentTypeHint GetComponentTypeHint() override {
        return ComponentTypeHint::VEC4_RGBA;
    }
    DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
    DEF_GET_PROPERTY_NAMES(prop_names);
    DEF_GET_VOID_PROPERTY_BY_INDEX(prop_name_indexes, prop_index_names);
    DEF_GET_VOID_PROPERTY_BY_NAME;
    DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
};

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

ExampleECS::EntityComponentEntry* SelectedEntityEntry = 0;
int SelectedEntityID = 0;
WINDOW* SelectedWindow = 0;
size_t SelectedWindowID = 0;

void DrawEntityEntry(ExampleECS&, int, ExampleECS::EntityComponentEntry&);
void DrawWindowEntry(const std::string&, WINDOW*);
bool EntityGraphFilterCheck(ExampleECS& ecs, ImGuiTextFilter& EntityGraphFilter, ExampleECS::EntityComponentEntry& entity_entry);
bool WindowGraphFilterCheck(ImGuiTextFilter& WindowGraphFilter, const std::string& window_name);

ImGuiTextFilter EntityGraphFilter;
ImGuiTextFilter WindowGraphFilter;

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
        assert(ecs.template RegisterComponent<ColorComponent>());
        return true;
    });
    
    ecs.EnableDrawInWindow(window);

    auto eids = ecs.AddEntities(Entity{}, Entity{});

    auto e1_id = eids[0];

    ecs.AddChildEntities(e1_id, Entity{}, Entity{}, Entity{});

    auto e1_ptr = ecs.GetEntity(e1_id);
    assert(e1_ptr);
    auto& e1 = *e1_ptr;
    auto& e1_position_component = ecs.ConstructComponent<PositionComponent>(e1.id, {1.f, 1.f, 1.f, 1.f});
    auto& e1_color_component = ecs.ConstructComponent<ColorComponent>(e1.id, {0.f, 0.f, 1.f, 1.f});

    auto e2_id = eids[1];

    ecs.AddChildEntities(e2_id, Entity{}, Entity{});

    auto e2_ptr = ecs.GetEntity(e2_id);

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
        }
    });

    imgui.AddImmediateDrawFunction(2.0f, "Viewport", [&, frame_image_ds](auto& layer) {
        static bool show_viewport = true;
        if (show_viewport)
        {
            ImGui::Begin("Viewport", &show_viewport);
            ImVec2 viewportSize = ImGui::GetContentRegionAvail();
            ImGui::Image((ImTextureID)frame_image_ds, viewportSize);
            ImGui::End();
        }
    });

    imgui.AddImmediateDrawFunction(3.0f, "Window Graph", [&](auto& layer) mutable {
        static bool window_graph_open = true;
        if (window_graph_open) {
            ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
            if (!ImGui::Begin("Window Graph", &window_graph_open))
            {
                ImGui::End();
                return;
            }
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip);
            ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
            if (ImGui::InputTextWithHint("##Filter", "incl,-excl", WindowGraphFilter.InputBuf, IM_ARRAYSIZE(WindowGraphFilter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll))
                WindowGraphFilter.Build();
            ImGui::PopItemFlag();

            if (ImGui::BeginTable("##bg", 1, ImGuiTableFlags_RowBg))
            {
                auto windows_begin = dr_get_windows_begin();
                auto windows_end = dr_get_windows_end();
                
                for (auto it = windows_begin; it != windows_end; it++) {
                    auto& window_name = window_get_title_ref(*it);
                    if (WindowGraphFilterCheck(WindowGraphFilter, window_name))
                        DrawWindowEntry(window_name, *it);
                }
                // for (auto& [id, entity_entry] : ecs.id_entity_entries)
                ImGui::EndTable();
            }
            ImGui::End();
        }
    });

    imgui.AddImmediateDrawFunction(3.0f, "Entity Graph", [&](auto& layer) mutable {
        static bool entity_graph_open = true;
        if (entity_graph_open) {
            ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
            if (!ImGui::Begin("Entity Graph", &entity_graph_open))
            {
                ImGui::End();
                return;
            }
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip);
            ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
            if (ImGui::InputTextWithHint("##Filter", "incl,-excl", EntityGraphFilter.InputBuf, IM_ARRAYSIZE(EntityGraphFilter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll))
                EntityGraphFilter.Build();
            ImGui::PopItemFlag();

            if (ImGui::BeginTable("##bg", 1, ImGuiTableFlags_RowBg))
            {
                for (auto& [id, entity_entry] : ecs.id_entity_entries)
                    if (!entity_entry.is_child && EntityGraphFilterCheck(ecs, EntityGraphFilter, entity_entry))
                        DrawEntityEntry(ecs, id, entity_entry);
                ImGui::EndTable();
            }
            ImGui::End();
        }
    });

    imgui.AddImmediateDrawFunction(4.0f, "Entity Property Editor", [&](auto& layer) {
        static bool entity_property_editor_open = true;
        if (entity_property_editor_open)
        {
            if (!ImGui::Begin("Entity Property Editor", &entity_property_editor_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse))
            {
                ImGui::End();
                return;
            }

            if (ExampleECS::EntityComponentEntry* entity_entry_ptr = SelectedEntityEntry)
            {
                auto& entity_entry = *entity_entry_ptr;
                if (entity_entry.name.size() < 28)
                    entity_entry.name.resize(28);

                ImGui::PushID((int)SelectedEntityID);

                ImGui::Text("Entity:");
                ImGui::SameLine();
                ImGui::InputText("##EntityName", entity_entry.name.data(), 28);
                ImGui::TextDisabled("Entity ID: 0x%08X", SelectedEntityID);
                ImGui::Spacing();

                for (auto& [component_id, component_ptr] : entity_entry.components)
                {
                    auto& component = *component_ptr;
                    const auto& component_name = component.GetComponentName();

                    if (ImGui::CollapsingHeader(component_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));

                        auto component_type_hint = component.GetComponentTypeHint();

                        switch (component_type_hint) {
                        case ComponentTypeHint::VEC4:
                        {
                            auto data_ptr = ecs.GetComponentDataVoid(component.index);
                            ImGui::SetNextItemWidth(250);
                            ImGui::DragFloat4("##vec4", static_cast<float*>(data_ptr), 0.01f, -1000.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                            break;
                        }
                        case ComponentTypeHint::VEC4_RGBA:
                        {
                            auto data_ptr = ecs.GetComponentDataVoid(component.index);
                            ImGui::SetNextItemWidth(250);
                            ImGui::ColorEdit4("##rgba", static_cast<float*>(data_ptr), ImGuiColorEditFlags_Float);
                            break;
                        }
                        default:
                        case ComponentTypeHint::STRUCT: {
                            const auto& component_typeinfos = component.GetPropertyTypeinfos();
                            const auto& component_prop_names = component.GetPropertyNames();
                            for (auto& prop_name : component_prop_names)
                            {
                                ImGui::PushID(prop_name.c_str());
                                auto prop_index = component.GetPropertyIndexByName(prop_name);
                                auto type_info = component_typeinfos[prop_index];

                                ImGui::Text("%s", prop_name.c_str());
                                ImGui::SameLine(150);

                                if (*type_info == typeid(float))
                                {
                                    auto& value = component.GetPropertyByIndex<float>(ecs, prop_index);
                                    ImGui::SetNextItemWidth(180);
                                    ImGui::DragFloat("##f", &value, 0.01f, -1000.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp);
                                }
                                else if (*type_info == typeid(int))
                                {
                                    auto& value = component.GetPropertyByIndex<int>(ecs, prop_index);
                                    ImGui::SetNextItemWidth(180);
                                    ImGui::InputInt("##i", &value);
                                }
                                else if (*type_info == typeid(std::string))
                                {
                                    auto& value = component.GetPropertyByIndex<std::string>(ecs, prop_index);
                                    if (value.capacity() < 128) value.reserve(128);
                                    ImGui::SetNextItemWidth(220);
                                    ImGui::InputText("##s", value.data(), value.capacity() + 1);
                                }
                                else
                                {
                                    ImGui::TextDisabled("<Unsupported type>");
                                }

                                ImGui::PopID();
                            }
                            break;
                        }
                        }

                        ImGui::PopStyleVar(2);
                    }
                }

                ImGui::PopID();
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

void DrawEntityEntry(ExampleECS& ecs, int id, ExampleECS::EntityComponentEntry& entity_entry)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::PushID(id);
    ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_None;
    tree_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;// Standard opening mode as we are likely to want to add selection afterwards
    tree_flags |= ImGuiTreeNodeFlags_NavLeftJumpsToParent;  // Left arrow support
    tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;         // Span full width for easier mouse reach
    tree_flags |= ImGuiTreeNodeFlags_DrawLinesToNodes;      // Always draw hierarchy outlines
    if (&entity_entry == SelectedEntityEntry)
        tree_flags |= ImGuiTreeNodeFlags_Selected;
    if (entity_entry.children.empty())
        tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    // if (node->DataMyBool == false)
    //     ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    bool node_open = ImGui::TreeNodeEx("", tree_flags, "%s", entity_entry.name.c_str());
    // if (node->DataMyBool == false)
    //     ImGui::PopStyleColor();
    if (ImGui::IsItemFocused()) {
        SelectedEntityEntry = &entity_entry;
        SelectedEntityID = id;
    }
    if (node_open)
    {
        for (auto& child_id : entity_entry.children) {
            auto& child_entry = ecs.id_entity_entries[child_id];
            if (EntityGraphFilterCheck(ecs, EntityGraphFilter, child_entry))
                DrawEntityEntry(ecs, child_id, child_entry);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void DrawWindowEntry(const std::string& window_name, WINDOW* window_ptr) {
    auto id = window_get_id_ref(window_ptr);
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::PushID(id);
    ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_None;
    tree_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;// Standard opening mode as we are likely to want to add selection afterwards
    tree_flags |= ImGuiTreeNodeFlags_NavLeftJumpsToParent;  // Left arrow support
    tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;         // Span full width for easier mouse reach
    tree_flags |= ImGuiTreeNodeFlags_DrawLinesToNodes;      // Always draw hierarchy outlines
    if (window_ptr == SelectedWindow)
        tree_flags |= ImGuiTreeNodeFlags_Selected;
    // if (entity_entry.children.empty())
        tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    // if (node->DataMyBool == false)
    //     ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    bool node_open = ImGui::TreeNodeEx("", tree_flags, "%s", window_name.c_str());
    // if (node->DataMyBool == false)
    //     ImGui::PopStyleColor();
    if (ImGui::IsItemFocused()) {
        SelectedWindow = window_ptr;
        SelectedWindowID = id;
    }
    if (node_open)
    {
        // for (auto& child_id : entity_entry.children) {
        //     auto& child_entry = ecs.id_entity_entries[child_id];
        //     if (EntityGraphFilterCheck(ecs, EntityGraphFilter, child_entry))
        //         DrawEntityEntry(ecs, child_id, child_entry);
        // }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

bool EntityGraphFilterCheck(ExampleECS& ecs, ImGuiTextFilter& EntityGraphFilter, ExampleECS::EntityComponentEntry& entity_entry) {
    auto initial = EntityGraphFilter.PassFilter(entity_entry.name.c_str());
    if (initial)
        return true;
    for (auto& child_id : entity_entry.children) {
        auto& child_entry = ecs.id_entity_entries[child_id];
        auto child = EntityGraphFilterCheck(ecs, EntityGraphFilter, child_entry);
        if (child)
            return true;
    }
    return false;
}

bool WindowGraphFilterCheck(ImGuiTextFilter& WindowGraphFilter, const std::string& window_name) {
    auto initial = WindowGraphFilter.PassFilter(window_name.c_str());
    if (initial)
        return true;
    // for (auto& child_id : entity_entry.children) {
    //     auto& child_entry = ecs.id_entity_entries[child_id];
    //     auto child = EntityGraphFilterCheck(ecs, EntityGraphFilter, child_entry);
    //     if (child)
    //         return true;
    // }
    return false;
}