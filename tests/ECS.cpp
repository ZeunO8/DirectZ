#include <DirectZ.hpp>
#include <fstream>
using namespace dz::ecs;

#include <typeinfo>

uint32_t default_vertex_count = 6;

struct Material : Provider<Material> {
    vec<float, 4> color;
    inline static float Priority = 2.5f;
    inline static std::string ProviderName = "Material";
    inline static std::string StructName = "Material";
    inline static std::string GLSLStruct = R"(
struct Material {
    vec4 color;
};
)";
    inline static std::vector<std::tuple<float, std::string, ShaderModuleType>> GLSLMain = {};
};

struct StateSystem : Provider<StateSystem> {
    int id;
    virtual ~StateSystem() = default;
};

// #define ENABLE_LIGHTS
using ExampleECS = ECS<CID_MIN, Entity, Component, Shape, Camera
#ifdef ENABLE_LIGHTS
, Light
#endif
>;


std::shared_ptr<ExampleECS> ecs_ptr;

// struct RotationComponent : Component {
//     float yaw;
//     float pitch;
//     float roll;
    
// };

float ORIGINAL_WINDOW_WIDTH = 1280.f;
float ORIGINAL_WINDOW_HEIGHT = 768.f;

ReflectableGroup* SelectedReflectableGroup = 0;
int SelectedReflectableID = 0;

void DrawEntityEntry(ExampleECS&, int, ExampleECS::EntityComponentReflectableGroup&);
void DrawSceneReflectableGroup(ExampleECS&, int, ExampleECS::SceneReflectableGroup&);
void DrawGenericEntry(int, ReflectableGroup& reflectable_group);
void DrawWindowEntry(const std::string&, WindowReflectableGroup& window_reflectable_group);
bool ReflectableGroupFilterCheck(ImGuiTextFilter&, ReflectableGroup&);
bool WindowGraphFilterCheck(ImGuiTextFilter& WindowGraphFilter, const std::string& window_name);

ImGuiTextFilter SceneGraphFilter;
ImGuiTextFilter WindowGraphFilter;

struct PropertyEditor {
    std::string name = "Property Editor";
    bool is_open = false;
};

PropertyEditor property_editor;

WINDOW* window = nullptr;

auto GetRegisterComponentsLambda() {
    return [](auto& ecs) {
        assert(ecs.template RegisterComponent<PositionComponent>());
        assert(ecs.template RegisterComponent<ColorComponent>());
        assert(ecs.template RegisterComponent<ScaleComponent>());
        return true;
    };
}

int main() {
    ExampleECS::RegisterStateCID();

    ExampleECS::SetComponentsRegisterFunction(GetRegisterComponentsLambda());

    const auto plane_shape_id = RegisterPlaneShape();
    const auto cube_shape_id = RegisterCubeShape();

    std::filesystem::path ioPath("example.ecs");

    set_state_file_path(ioPath);

    if (has_state()) {
        if (load_state()) {
            window = state_get_ptr<WINDOW>(CID_WINDOW);
            auto _ecs_ptr = dynamic_cast<ExampleECS*>(state_get_ptr<ExampleECS>(ExampleECS::CID));
            ecs_ptr = std::shared_ptr<ExampleECS>(_ecs_ptr, [](auto v) {});
        }
    }

    bool state_loaded = is_state_loaded();

    if (!state_loaded) {
        window = window_create({
            .title = "ECS Test",
            .x = 0,
            .y = 240,
            .width = ORIGINAL_WINDOW_WIDTH,
            .height = ORIGINAL_WINDOW_HEIGHT,
            .borderless = true,
            .vsync = true
        });
        track_window_state(window);
        ecs_ptr = std::make_shared<ExampleECS>(window);
        track_state(ecs_ptr.get());
    }

    auto& ecs = *ecs_ptr;
        
    if (!state_loaded) {
        ecs.SetProviderCount("Shapes", 2);
        auto shapes_ptr = ecs.GetProviderData<Shape>("Shapes");

        auto& plane_shape = shapes_ptr[0];
        plane_shape.type = plane_shape_id;
        plane_shape.vertex_count = 6;

        auto& cube_shape = shapes_ptr[1];
        cube_shape.type = cube_shape_id;
        cube_shape.vertex_count = 36;

        auto eids = ecs.AddEntitys(Entity{}, Entity{});

        auto e1_id = eids[0];

        // ecs.AddChildEntitys(e1_id, Entity{}, Entity{}, Entity{});

        auto e1_ptr = ecs.GetEntity(e1_id);
        assert(e1_ptr);
        auto& e1 = *e1_ptr;
        e1.shape_index = 1;
        auto& e1_position_component = ecs.ConstructComponent<PositionComponent>(e1.id, {0.5f, -0.5f, 1.f, 1.f});
        auto& e1_color_component = ecs.ConstructComponent<ColorComponent>(e1.id, {0.f, 0.f, 1.f, 1.f});
        auto& e1_scale_component = ecs.ConstructComponent<ScaleComponent>(e1.id, {1.0f, 1.0f, 1.0f, 1.0f});

        auto e2_id = eids[1];

        // ecs.AddChildEntitys(e2_id, Entity{}, Entity{});

        auto e2_ptr = ecs.GetEntity(e2_id);

        assert(e2_ptr);
        auto& e2 = *e2_ptr;
        auto& e2_position_component = ecs.ConstructComponent<PositionComponent>(e2.id, {-0.5f, 0.5f, 1.f, 1.f});
        auto& e2_color_component = ecs.ConstructComponent<ColorComponent>(e2.id, {1.f, 0.f, 0.f, 1.f});
        auto& e2_scale_component = ecs.ConstructComponent<ScaleComponent>(e2.id, {1.0f, 1.0f, 1.0f, 1.0f});
    }

#ifdef ENABLE_LIGHTS
    if (buffer_group_get_buffer_element_count(ecs.buffer_group, "Lights") == 0) {
        auto ecs_scene_ids = ecs.GetSceneIDs();
        ecs.AddLightToScene(ecs_scene_ids[0], Light::Directional);
        ecs.AddLightToScene(ecs_scene_ids[0], Light::Spot);
    }
#endif

    ecs.MarkReady();

    auto frame_image = ecs.GetFramebufferImage(0);
    
    auto& window_width = *window_get_width_ref(window);
    auto& window_height = *window_get_height_ref(window);

    window_register_free_callback(window, 0.0f, [&]() mutable {
        if (is_tracking_state()) {
            if (!save_state())
                std::cerr << "Failed to Save State" << std::endl;
        }
    });

    window_register_free_callback(window, 1000.0f, [&]() mutable {
        if (is_tracking_state()) {
            if (!free_state())
                std::cerr << "Failed to Free State" << std::endl;
        }
    });

    auto& imgui = get_ImGuiLayer();
    
    imgui.AddImmediateDrawFunction(0.5, "Menu", [&](auto& layer) {
        if (ImGui::BeginMainMenuBar()) {
            ImVec2 menu_pos = ImGui::GetWindowPos();
            ImVec2 menu_size = ImGui::GetWindowSize();
            ImVec2 mouse_pos = ImGui::GetMousePos();
            ImGuiViewport* main_viewport = ImGui::GetMainViewport();

            bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
            bool held = ImGui::IsMouseDown(ImGuiMouseButton_Left) && ImGui::IsMouseDragging(ImGuiMouseButton_Left);

            static bool dragging = false;

            if (main_viewport && main_viewport->PlatformHandle && !dragging && (hovered && held)) {
                dragging = true;
                window_request_drag((WINDOW*)main_viewport->PlatformHandle);
            }
            else if (dragging && !(hovered && held)) {
                dragging = false;
            }

            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New", "Ctrl+N")) {
                }

                if (ImGui::MenuItem("Open", "Ctrl+O")) {
                }

                if (ImGui::MenuItem("Save", "Ctrl+S")) {
                }

                if (ImGui::MenuItem("Save As..", "Ctrl+Shift+S")) {
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Exit", "Alt+F4")) {
                    window_request_close(window);
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Edit")) {
                if (ImGui::MenuItem("Undo", "Ctrl+Z")) {
                }

                if (ImGui::MenuItem("Redo", "Ctrl+Y")) {
                }

                ImGui::Separator();

                if (ImGui::MenuItem("Cut", "Ctrl+X")) {
                }

                if (ImGui::MenuItem("Copy", "Ctrl+C")) {
                }

                if (ImGui::MenuItem("Paste", "Ctrl+V")) {
                }

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("View")) {
                static bool showToolbar = true;
                static bool showSidebar = true;
                static bool showStatusbar = true;

                ImGui::MenuItem("Toolbar", nullptr, &showToolbar);
                ImGui::MenuItem("Sidebar", nullptr, &showSidebar);
                ImGui::MenuItem("Status Bar", nullptr, &showStatusbar);

                ImGui::EndMenu();
            }

            if (ImGui::BeginMenu("Help")) {
                if (ImGui::MenuItem("Documentation", "F1")) {
                }

                if (ImGui::MenuItem("About")) {
                }

                ImGui::EndMenu();
            }

            float button_w = ImGui::GetFontSize() * 1.5f;
            float button_h = ImGui::GetFontSize() * 1.2f;
            float spacing = ImGui::GetStyle().ItemSpacing.x / 2.f;
            float total_width = (button_w + spacing) * 3;

            ImGui::SetCursorPosX(ImGui::GetWindowSize().x - total_width);

            ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(4, 3));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 2.0f);

            if (ImGui::Button(ICON_MAX, ImVec2(button_w, button_h))) {
                // window_request_maximize(window);
            }

            ImGui::SameLine(0.0f, spacing);
            if (ImGui::Button(ICON_MIN, ImVec2(button_w, button_h))) {
                // window_request_minimize(window);
            }

            ImGui::SameLine(0.0f, spacing);
            if (ImGui::Button(ICON_CLOSE, ImVec2(button_w, button_h))) {
                window_request_close(window);
            }

            ImGui::PopStyleVar(2);

            ImGui::EndMainMenuBar();
        }
    });

    imgui.AddImmediateDrawFunction(1.0f, "DockspaceRoot", [](dz::ImGuiLayer& layer) {
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

    imgui.AddImmediateDrawFunction(2.0f, "Cameras", [&](auto& layer) mutable {
        auto cameras_begin = ecs.GetCamerasBegin();
        auto cameras_end = ecs.GetCamerasEnd();
        for (auto camera_it = cameras_begin; camera_it != cameras_end; camera_it++) {
            auto camera_index = camera_it->first;
            auto& camera_group = camera_it->second;
            if (camera_group.open_in_editor) {
                if (!ImGui::Begin(camera_group.imgui_name.c_str(), &camera_group.open_in_editor)) {
                    ImGui::End();
                    continue;
                }
                auto panel_pos = ImGui::GetCursorScreenPos();
                auto panel_size = ImGui::GetContentRegionAvail();

                ImGui::Image((ImTextureID)camera_group.frame_image_ds, panel_size, ImVec2(0, 1), ImVec2(1, 0));
                ecs.ResizeFramebuffer(camera_index, panel_size.x, panel_size.y);

                auto reflectable_group_ptr = SelectedReflectableGroup;
                auto entity_group_ptr = dynamic_cast<ExampleECS::EntityComponentReflectableGroup*>(reflectable_group_ptr);
                if (entity_group_ptr) {
                    auto& entity_group = *entity_group_ptr;
                    auto entity_ptr = ecs.GetEntity(entity_group.id);
                    assert(entity_ptr);
                    auto& entity = *entity_ptr;
                    try {
                        static auto manipulate_type = ImGuizmo::TRANSLATE;
                        if (ImGui::IsKeyPressed(ImGuiKey_T)) {
                            manipulate_type = ImGuizmo::TRANSLATE;
                        }
                        else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                            manipulate_type = ImGuizmo::SCALE;
                        }
                        else if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                            manipulate_type = ImGuizmo::ROTATE;
                        }
                        auto& entity_position = ecs.GetTypeComponentData<PositionComponent>(entity_group.id);
                        auto& entity_scale = ecs.GetTypeComponentData<ScaleComponent>(entity_group.id);
                        mat<float, 4, 4> transform(1.0f);
                        transform[3] = entity_position;
                        transform[0][0] *= entity_scale[0];
                        transform[0][1] *= entity_scale[0];
                        transform[0][2] *= entity_scale[0];
                        transform[1][0] *= entity_scale[1];
                        transform[1][1] *= entity_scale[1];
                        transform[1][2] *= entity_scale[1];
                        transform[2][0] *= entity_scale[2];
                        transform[2][1] *= entity_scale[2];
                        transform[2][2] *= entity_scale[2];
                        // transform = transform.transpose();
                        ImGuizmo::SetDrawlist();
                        ImGuizmo::SetRect(panel_pos.x, panel_pos.y, panel_size.x, panel_size.y);
                        auto camera_ptr = ecs.GetCamera(camera_index);
                        assert(camera_ptr);
                        auto& camera = *camera_ptr;
                        ImGuizmo::SetOrthographic(Camera::ProjectionType(camera.type) == Camera::Orthographic);
                        auto& camera_view = camera.view;
                        auto camera_projection = camera.projection;
                        // camera_projection[1][1] *= -1.0f;
                        ImGuizmo::Manipulate(
                            &camera_view[0][0],
                            &camera_projection[0][0],
                            manipulate_type, // or ROTATE / SCALE
                            ImGuizmo::LOCAL,     // or WORLD
                            &transform[0][0],
                            nullptr              // optional delta matrix out
                        );
                        if (ImGuizmo::IsUsing())
                        {
                            float pos[3] = {0};
                            float rot[3] = {0};
                            float scale[3] = {0};
                            ImGuizmo::DecomposeMatrixToComponents(&transform[0][0], pos, rot, scale);
                            entity_position[0] = pos[0];
                            entity_position[1] = pos[1];
                            entity_position[2] = pos[2];
                            entity_scale[0] = scale[0];
                            entity_scale[1] = scale[1];
                            entity_scale[2] = scale[2];
                        }
                    }
                    catch (...) { }
                }
                
                ImGui::End();
            }
        }
    });

    imgui.AddImmediateDrawFunction(3.0f, "Window Graph", [&](auto& layer) mutable {
        static bool window_graph_open = true;
        if (window_graph_open) {
            ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
            if (!ImGui::Begin("Window Graph", &window_graph_open)) {
                ImGui::End();
                return;
            }
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip);
            ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
            if (ImGui::InputTextWithHint("##Filter", "incl,-excl", WindowGraphFilter.InputBuf, IM_ARRAYSIZE(WindowGraphFilter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll))
                WindowGraphFilter.Build();
            ImGui::PopItemFlag();

            if (ImGui::BeginTable("##bg", 1, ImGuiTableFlags_RowBg)) {
                auto window_reflectable_entries_begin = dr_get_window_reflectable_entries_begin();
                auto window_reflectable_entries_end = dr_get_window_reflectable_entries_end();
                
                for (
                    auto it = window_reflectable_entries_begin;
                    it != window_reflectable_entries_end;
                    it++
                ) {
                    auto& window_reflectable_group = **it;
                    auto& window_name = window_reflectable_group.GetName();
                    if (WindowGraphFilterCheck(WindowGraphFilter, window_name))
                        DrawWindowEntry(window_name, window_reflectable_group);
                }
                // for (auto& [id, entity_group] : ecs.id_entity_groups)
                ImGui::EndTable();
            }
            ImGui::End();
        }
    });

    imgui.AddImmediateDrawFunction(3.0f, "Scene Graph", [&](auto& layer) mutable {
        static bool scene_graph_open = true;
        if (scene_graph_open) {
            ImGui::SetNextWindowSize(ImVec2(430, 450), ImGuiCond_FirstUseEver);
            if (!ImGui::Begin("Scene Graph", &scene_graph_open)) {
                ImGui::End();
                return;
            }
            ImGui::SetNextItemWidth(-FLT_MIN);
            ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip);
            ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
            if (ImGui::InputTextWithHint("##Filter", "incl,-excl", SceneGraphFilter.InputBuf, IM_ARRAYSIZE(SceneGraphFilter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll))
                SceneGraphFilter.Build();
            ImGui::PopItemFlag();

            if (ImGui::BeginTable("##bg", 1, ImGuiTableFlags_RowBg)) {
                auto scenes_begin = ecs.GetScenesBegin();
                auto scenes_end = ecs.GetScenesEnd();
                for (auto scene_it = scenes_begin; scene_it != scenes_end; ++scene_it) {
                    auto& [scene_id, scene_group] = *scene_it;
                    DrawSceneReflectableGroup(ecs, scene_id, scene_group);
                }
                ImGui::EndTable();
            }
            ImGui::End();
        }
    });

    imgui.AddImmediateDrawFunction(4.0f, "Property Editor", [&](auto& layer) {
        if (property_editor.is_open) {
            if (!ImGui::Begin("Property Editor", &property_editor.is_open, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoCollapse)) {
                property_editor.is_open = false;
                SelectedReflectableGroup = nullptr;
                SelectedReflectableID = 0;
                ImGui::End();
                return;
            }

            if (ReflectableGroup* reflectable_group_ptr = SelectedReflectableGroup) {
                auto& reflectable_group = *reflectable_group_ptr;
                auto& reflectable_group_name = reflectable_group.GetName();
                if (reflectable_group_name.size() < 28)
                    reflectable_group_name.resize(28);

                ImGui::PushID(SelectedReflectableGroup);

                if (ImGui::InputText("Name", reflectable_group_name.data(), reflectable_group_name.size())) {
                    reflectable_group.NotifyNameChanged();
                }
                ImGui::TextDisabled("ID: 0x%08X", SelectedReflectableID);
                ImGui::Spacing();

                auto& reflectables = reflectable_group.GetReflectables();

                auto reflect_begin = reflectables.begin();
                auto reflect_it = reflect_begin;
                auto reflect_end = reflectables.end();
                size_t reflect_dist = 0;

                auto update_iterators = [&]() {
                    reflect_begin = reflectables.begin();
                    reflect_it = reflect_begin + reflect_dist;
                    reflect_end = reflectables.end();
                };

                for (; reflect_it != reflect_end; reflect_it++) {
                    reflect_dist = std::distance(reflect_begin, reflect_it);
                    auto& reflectable = **reflect_it;
                    const auto& reflectable_name = reflectable.GetName();

                    if (ImGui::CollapsingHeader(reflectable_name.c_str(), ImGuiTreeNodeFlags_DefaultOpen))
                    {
                        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(6, 4));
                        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(12, 8));

                        auto reflectable_type_hint = reflectable.GetTypeHint();

                        switch (reflectable_type_hint) {
                        case ReflectableTypeHint::VEC2:
                        {
                            auto data_ptr = reflectable.GetVoidPropertyByIndex(0);
                            // ImGui::SetNextItemWidth(250);
                            if (ImGui::DragFloat2("##vec2", static_cast<float*>(data_ptr), 0.01f, -1000.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                                reflectable.NotifyChange(0);
                                update_iterators();
                            }
                            break;
                        }
                        case ReflectableTypeHint::VEC3:
                        {
                            auto data_ptr = reflectable.GetVoidPropertyByIndex(0);
                            // ImGui::SetNextItemWidth(250);
                            if (ImGui::DragFloat3("##vec3", static_cast<float*>(data_ptr), 0.01f, -1000.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                                reflectable.NotifyChange(0);
                                update_iterators();
                            }
                            break;
                        }
                        case ReflectableTypeHint::VEC3_RGB:
                        {
                            auto data_ptr = reflectable.GetVoidPropertyByIndex(0);
                            // ImGui::SetNextItemWidth(250);
                            if (ImGui::ColorEdit3("##rgba", static_cast<float*>(data_ptr), ImGuiColorEditFlags_Float)) {
                                reflectable.NotifyChange(0);
                                update_iterators();
                            }
                            break;
                        }
                        case ReflectableTypeHint::VEC4:
                        {
                            auto data_ptr = reflectable.GetVoidPropertyByIndex(0);
                            // ImGui::SetNextItemWidth(250);
                            if (ImGui::DragFloat4("##vec4", static_cast<float*>(data_ptr), 0.01f, -1000.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                                reflectable.NotifyChange(0);
                                update_iterators();
                            }
                            break;
                        }
                        case ReflectableTypeHint::VEC4_RGBA:
                        {
                            auto data_ptr = reflectable.GetVoidPropertyByIndex(0);
                            // ImGui::SetNextItemWidth(250);
                            if (ImGui::ColorEdit4("##rgba", static_cast<float*>(data_ptr), ImGuiColorEditFlags_Float)) {
                                reflectable.NotifyChange(0);
                                update_iterators();
                            }
                            break;
                        }
                        default:
                        case ReflectableTypeHint::STRUCT: {
                            const auto& reflectable_typeinfos = reflectable.GetPropertyTypeinfos();
                            const auto& reflectable_prop_names = reflectable.GetPropertyNames();
                            const auto& disabled_properties = reflectable.GetDisabledProperties();
                            size_t index = 0;
                            for (auto& prop_name : reflectable_prop_names)
                            {
                                ImGui::PushID(prop_name.c_str());
                                auto prop_index = reflectable.GetPropertyIndexByName(prop_name);
                                auto type_info = reflectable_typeinfos[prop_index];
                                bool is_disabled = (prop_index < disabled_properties.size()) ? disabled_properties[prop_index] : false;

                                ImGui::BeginDisabled(is_disabled);
                                ImGui::Text("%s", prop_name.c_str());
                                ImGui::SameLine();

                                if (*type_info == typeid(float))
                                {
                                    auto& value = reflectable.GetPropertyByIndex<float>(prop_index);
                                    ImGui::PushID(prop_index);
                                    if (ImGui::InputFloat("##input", &value, 0.1f, 1.0f, "%.3f")) {
                                        reflectable.NotifyChange(prop_index);
                                        update_iterators();
                                    }
                                    ImGui::PopID();
                                }
                                else if (*type_info == typeid(int)) {
                                    auto& value = reflectable.GetPropertyByIndex<int>(prop_index);
                                    ImGui::PushID(prop_index);
                                    if (ImGui::InputInt("##input", &value)) {
                                        reflectable.NotifyChange(prop_index);
                                        update_iterators();
                                    }
                                    ImGui::PopID();
                                }
                                else if (*type_info == typeid(Light::LightType)) {
                                    auto& value = reflectable.GetPropertyByIndex<Light::LightType>(prop_index);
                                    static const char* projection_types[] = { "Directional", "Spot", "Point" };
                                    int current_index = static_cast<int>(value);
                                    if (ImGui::Combo("##lightType", &current_index, projection_types, IM_ARRAYSIZE(projection_types))) {
                                        value = static_cast<Light::LightType>(current_index);
                                        reflectable.NotifyChange(prop_index);
                                        update_iterators();
                                    }
                                }
                                else if (*type_info == typeid(Camera::ProjectionType)) {
                                    auto& value = reflectable.GetPropertyByIndex<Camera::ProjectionType>(prop_index);
                                    static const char* projection_types[] = { "Perspective", "Orthographic" };
                                    int current_index = static_cast<int>(value);
                                    if (ImGui::Combo("##proj", &current_index, projection_types, IM_ARRAYSIZE(projection_types))) {
                                        value = static_cast<Camera::ProjectionType>(current_index);
                                        reflectable.NotifyChange(prop_index);
                                        update_iterators();
                                    }
                                }
                                else if (*type_info == typeid(std::string)) {
                                    auto& value = reflectable.GetPropertyByIndex<std::string>(prop_index);
                                    if (value.capacity() < 128) value.reserve(128);
                                    ImGui::PushID(prop_index);
                                    if (ImGui::InputText("##s", value.data(), value.capacity() + 1)) {
                                        reflectable.NotifyChange(prop_index);
                                        update_iterators();
                                    }
                                    ImGui::PopID();
                                }
                                else if (*type_info == typeid(vec<float, 2>)) {
                                    auto& value = reflectable.GetPropertyByIndex<vec<float, 2>>(prop_index);
                                    if (ImGui::DragFloat2("##vec2", static_cast<float*>(&value[0]), 0.01f, -1000.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                                        reflectable.NotifyChange(prop_index);
                                        update_iterators();
                                    }
                                }
                                else if (*type_info == typeid(vec<float, 3>)) {
                                    auto& value = reflectable.GetPropertyByIndex<vec<float, 3>>(prop_index);
                                    if (ImGui::DragFloat3("##vec3", static_cast<float*>(&value[0]), 0.01f, -1000.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                                        reflectable.NotifyChange(prop_index);
                                        update_iterators();
                                    }
                                }
                                else if (*type_info == typeid(vec<float, 4>)) {
                                    auto& value = reflectable.GetPropertyByIndex<vec<float, 4>>(prop_index);
                                    if (ImGui::DragFloat4("##vec4", static_cast<float*>(&value[0]), 0.01f, -1000.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                                        reflectable.NotifyChange(prop_index);
                                        update_iterators();
                                    }
                                }
                                else if (*type_info == typeid(color_vec<float, 3>)) {
                                    auto& value = reflectable.GetPropertyByIndex<color_vec<float, 3>>(prop_index);
                                    if (ImGui::ColorEdit3("##rgb", static_cast<float*>(&value[0]), ImGuiColorEditFlags_Float)) {
                                        reflectable.NotifyChange(0);
                                        update_iterators();
                                    }
                                }
                                else if (*type_info == typeid(color_vec<float, 4>)) {
                                    auto& value = reflectable.GetPropertyByIndex<color_vec<float, 4>>(prop_index);
                                    if (ImGui::ColorEdit4("##rgba", static_cast<float*>(&value[0]), ImGuiColorEditFlags_Float)) {
                                        reflectable.NotifyChange(0);
                                        update_iterators();
                                    }
                                }
                                else {
                                    ImGui::TextDisabled("<Unsupported type>");
                                }

                                ImGui::EndDisabled();
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

    while (windows_poll_events()) {
        windows_render();
    }
}

void DrawEntityEntry(ExampleECS& ecs, int id, ExampleECS::EntityComponentReflectableGroup& entity_group)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::PushID(&entity_group);
    ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_None;
    tree_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;// Standard opening mode as we are likely to want to add selection afterwards
    tree_flags |= ImGuiTreeNodeFlags_NavLeftJumpsToParent;  // Left arrow support
    tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;         // Span full width for easier mouse reach
    tree_flags |= ImGuiTreeNodeFlags_DrawLinesToNodes;      // Always draw hierarchy outlines
    if (&entity_group == SelectedReflectableGroup)
        tree_flags |= ImGuiTreeNodeFlags_Selected;
    if (entity_group.children.empty())
        tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    if (entity_group.disabled)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    bool node_open = ImGui::TreeNodeEx("", tree_flags, "%s", entity_group.name.c_str());
    if (entity_group.disabled)
        ImGui::PopStyleColor();
    if (ImGui::IsItemFocused()) {
        SelectedReflectableGroup = &entity_group;
        SelectedReflectableID = id;
        property_editor.is_open = true;
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("EntityContextMenu");
    }

    if (ImGui::BeginPopup("EntityContextMenu")) {
        if (ImGui::BeginMenu("Add Child")) {
            if (ImGui::MenuItem("Plane")) {
                // ecs.AddEntityToScene(scene_id, "Plane");
            }
            if (ImGui::MenuItem("Cube")) {
                // ecs.AddEntityToScene(scene_id, "Cube");
            }
            if (ImGui::MenuItem("Monkey")) {
                // ecs.AddEntityToScene(scene_id, "Monkey");
            }
            if (ImGui::MenuItem("Mesh")) {
                // ecs.AddEntityToScene(scene_id, "Mesh");
            }
            ImGui::EndMenu();
        }

        if (ImGui::MenuItem("Duplicate Entity")) {
            // ecs.DuplicateEntity(id);
        }
        if (ImGui::MenuItem("Delete Entity")) {
            // ecs.DeleteEntity(id);
        }

        ImGui::EndPopup();
    }

    if (node_open) {
        for (auto& child_id : entity_group.children) {
            auto& child_group = ecs.id_entity_groups[child_id];
            if (ReflectableGroupFilterCheck(SceneGraphFilter, child_group))
                DrawEntityEntry(ecs, child_id, child_group);
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void DrawSceneReflectableGroup(ExampleECS& ecs, int scene_id, ExampleECS::SceneReflectableGroup& scene_group) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::PushID(&scene_group);
    ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_None;
    tree_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;// Standard opening mode as we are likely to want to add selection afterwards
    tree_flags |= ImGuiTreeNodeFlags_NavLeftJumpsToParent;  // Left arrow support
    tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;         // Span full width for easier mouse reach
    tree_flags |= ImGuiTreeNodeFlags_DrawLinesToNodes;      // Always draw hierarchy outlines
    if (&scene_group == SelectedReflectableGroup)
        tree_flags |= ImGuiTreeNodeFlags_Selected;
    auto& scene_group_children = scene_group.GetChildren();
    if (scene_group_children.empty())
        tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    if (scene_group.disabled)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    bool node_open = ImGui::TreeNodeEx("", tree_flags, "%s", scene_group.name.c_str());
    if (scene_group.disabled)
        ImGui::PopStyleColor();
    if (ImGui::IsItemFocused()) {
        SelectedReflectableGroup = &scene_group;
        SelectedReflectableID = scene_id;
        property_editor.is_open = true;
    }

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        ImGui::OpenPopup("SceneContextMenu");
    }

    if (ImGui::BeginPopup("SceneContextMenu")) {
        if (ImGui::BeginMenu("Add Entity")) {
            if (ImGui::MenuItem("Plane")) {
                // ecs.AddEntityToScene(scene_id, "Plane");
            }
            if (ImGui::MenuItem("Cube")) {
                // ecs.AddEntityToScene(scene_id, "Cube");
            }
            if (ImGui::MenuItem("Monkey")) {
                // ecs.AddEntityToScene(scene_id, "Monkey");
            }
            if (ImGui::MenuItem("Mesh")) {
                // ecs.AddEntityToScene(scene_id, "Mesh");
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Add Camera")) {
            if (ImGui::MenuItem("Perspective")) {
                ecs.AddCameraToScene(scene_id, Camera::Perspective);
            }
            if (ImGui::MenuItem("Orthographic")) {
                ecs.AddCameraToScene(scene_id, Camera::Orthographic);
            }
            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Add Light")) {
            if (ImGui::MenuItem("Directional")) {
                ecs.AddLightToScene(scene_id, Light::Directional);
            }
            if (ImGui::MenuItem("Spot")) {
                ecs.AddLightToScene(scene_id, Light::Spot);
            }
            if (ImGui::MenuItem("Point")) {
                ecs.AddLightToScene(scene_id, Light::Point);
            }
            ImGui::EndMenu();
        }

        ImGui::EndPopup();
    }

    if (node_open) {
        for (auto scene_group_child_group : scene_group_children) {
            auto& child_group = *scene_group_child_group;
            if (ReflectableGroupFilterCheck(SceneGraphFilter, child_group)) {
                switch (child_group.GetGroupType()) {
                case ReflectableGroup::Window:
                    assert(false);
                    break;
                case ReflectableGroup::Generic:
                case ReflectableGroup::Camera:
                case ReflectableGroup::Light:
                case ReflectableGroup::Provider:
                    DrawGenericEntry(child_group.id, child_group);
                    break;
                case ReflectableGroup::Scene:
                    DrawSceneReflectableGroup(ecs, child_group.id, dynamic_cast<ExampleECS::SceneReflectableGroup&>(child_group));
                    break;
                case ReflectableGroup::Entity:
                    DrawEntityEntry(ecs, child_group.id, dynamic_cast<ExampleECS::EntityComponentReflectableGroup&>(child_group));
                    break;
                }
            }
        }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void DrawGenericEntry(int generic_id, ReflectableGroup& reflectable_group) {
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::PushID(&reflectable_group);
    ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_None;
    tree_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;// Standard opening mode as we are likely to want to add selection afterwards
    tree_flags |= ImGuiTreeNodeFlags_NavLeftJumpsToParent;  // Left arrow support
    tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;         // Span full width for easier mouse reach
    tree_flags |= ImGuiTreeNodeFlags_DrawLinesToNodes;      // Always draw hierarchy outlines
    if (&reflectable_group == SelectedReflectableGroup)
        tree_flags |= ImGuiTreeNodeFlags_Selected;
    tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    if (reflectable_group.disabled)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    bool node_open = ImGui::TreeNodeEx("", tree_flags, "%s", reflectable_group.GetName().c_str());
    if (reflectable_group.disabled)
        ImGui::PopStyleColor();
    if (ImGui::IsItemFocused()) {
        SelectedReflectableGroup = &reflectable_group;
        SelectedReflectableID = reflectable_group.id;
        property_editor.is_open = true;
    }

    if (node_open) {
        ImGui::TreePop();
    }
    ImGui::PopID();
}

void DrawWindowEntry(const std::string& window_name, WindowReflectableGroup& window_reflectable_group) {
    auto id = window_reflectable_group.id;
    ImGui::TableNextRow();
    ImGui::TableNextColumn();
    ImGui::PushID(&window_reflectable_group);
    ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_None;
    tree_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;// Standard opening mode as we are likely to want to add selection afterwards
    tree_flags |= ImGuiTreeNodeFlags_NavLeftJumpsToParent;  // Left arrow support
    tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;         // Span full width for easier mouse reach
    tree_flags |= ImGuiTreeNodeFlags_DrawLinesToNodes;      // Always draw hierarchy outlines
    if (&window_reflectable_group == SelectedReflectableGroup)
        tree_flags |= ImGuiTreeNodeFlags_Selected;
    // if (entity_group.children.empty())
        tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    if (window_reflectable_group.disabled)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
    bool node_open = ImGui::TreeNodeEx("", tree_flags, "%s", window_name.c_str());
    if (window_reflectable_group.disabled)
        ImGui::PopStyleColor();
    if (ImGui::IsItemFocused()) {
        SelectedReflectableGroup = &window_reflectable_group;
        SelectedReflectableID = window_reflectable_group.id;
        property_editor.is_open = true;
    }

    if (node_open) {
        // for (auto& child_id : entity_group.children) {
        //     auto& child_group = ecs.id_entity_groups[child_id];
        //     if (ReflectableGroupFilterCheck(SceneGraphFilter, child_group))
        //         DrawEntityEntry(ecs, child_id, child_group);
        // }
        ImGui::TreePop();
    }
    ImGui::PopID();
}

bool ReflectableGroupFilterCheck(ImGuiTextFilter& SceneGraphFilter, ReflectableGroup& reflectable_group) {
    auto initial = SceneGraphFilter.PassFilter(reflectable_group.GetName().c_str());
    if (initial)
        return true;
    for (auto child_group_ptr : reflectable_group.GetChildren()) {
        auto child = ReflectableGroupFilterCheck(SceneGraphFilter, *child_group_ptr);
        if (child)
            return true;
    }
    return false;
}

bool WindowGraphFilterCheck(ImGuiTextFilter& WindowGraphFilter, const std::string& window_name) {
    auto initial = WindowGraphFilter.PassFilter(window_name.c_str());
    if (initial)
        return true;
    // for (auto& child_id : entity_group.children) {
    //     auto& child_group = ecs.id_entity_groups[child_id];
    //     auto child = ReflectableGroupFilterCheck(SceneGraphFilter, child_group);
    //     if (child)
    //         return true;
    // }
    return false;
}