#include <DirectZ.hpp>
#include <fstream>
using namespace dz::ecs;
using namespace dz::loaders;

#include <typeinfo>

uint32_t default_vertex_count = 6;

struct StateSystem : Provider<StateSystem> {
    int id;
    virtual ~StateSystem() = default;
};

// #define ENABLE_LIGHTS
using ExampleECS = ECS<
    CID_MIN,
    Scene,
    Entity,
    Mesh,
    SubMesh,
    Camera,
    Material
#ifdef ENABLE_LIGHTS
    , Light
#endif
>;


std::shared_ptr<ExampleECS> ecs_ptr;

float ORIGINAL_WINDOW_WIDTH = 1280.f;
float ORIGINAL_WINDOW_HEIGHT = 768.f;

ReflectableGroup* SelectedReflectableGroup = 0;
int SelectedReflectableID = 0;

// void DrawEntityGroup(ExampleECS&, int, ExampleECS::EntityReflectableGroup&);
// void DrawSceneReflectableGroup(ExampleECS&, int, ExampleECS::SceneReflectableGroup&);
void DrawGenericGroup(ReflectableGroup&);
void DrawWindowGroup(const std::string&, WindowReflectableGroup& window_reflectable_group);
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

auto set_pair_id_index(auto pair, auto& id, auto& index) {
    id = pair.first;
    index = pair.second;
}

using TL = TypeLoader<STB_Image_Loader>;

std::pair<size_t, int> AddPyramidMesh(ExampleECS& ecs, int material_index);
std::pair<size_t, int> AddPlaneMesh(ExampleECS& ecs, int material_index);
std::pair<size_t, int> AddCubeMesh(ExampleECS& ecs, int material_index);

int main() {
    ExampleECS::RegisterStateCID();

    std::filesystem::path ioPath("ECS-Test.dat");

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
            .vsync = false
        });
        track_window_state(window);
        ecs_ptr = std::make_shared<ExampleECS>(window);
        track_state(ecs_ptr.get());
    }

    auto& ecs = *ecs_ptr;
        
    if (!state_loaded) {
        int mat1_index = -1;
        auto mat1_id = ecs.AddMaterial(Material{}, mat1_index);
        auto im_1 = TL::Load<Image*, STB_Image_Loader::info_type>({
            .path = "hi.bmp"
        });
        ecs.SetMaterialImage(mat1_id, im_1);

        int blue_material = -1;
        auto mat2_id = ecs.AddMaterial(Material{
            .albedo = {0.0f, 0.0f, 1.0f, 1.0f}
        }, blue_material);

        auto [pyramid_mesh_id, pyramid_mesh_index] = AddPyramidMesh(ecs, blue_material);
        auto [plane_mesh_id, plane_mesh_index] = AddPlaneMesh(ecs, blue_material);
        auto [cube_mesh_id, cube_mesh_index] = AddCubeMesh(ecs, blue_material);

        int mat3_index = -1;
        auto mat3_id = ecs.AddMaterial(Material{}, mat3_index);
        auto suzuho = TL::Load<Image*, STB_Image_Loader::info_type>({
            .path = "Suzuho-Ueda.bmp"
        });
        ecs.SetMaterialImage(mat3_id, suzuho);

        auto scene1_id = ecs.AddScene(Scene{});

        auto cam1_id = ecs.AddCamera(scene1_id, Camera{}, Camera::Perspective);

        auto e1_id = ecs.AddEntity(scene1_id, Entity{}, {plane_mesh_index});
        auto e2_id = ecs.AddEntity(scene1_id, Entity{
            .position = {1.f, 1.f, 0.f, 1.f}
        }, {plane_mesh_index});

        auto scene2_id = ecs.AddScene(Scene{});

        auto cam2_id = ecs.AddCamera(scene2_id, Camera{}, Camera::Perspective);

        auto e3_id = ecs.AddEntity(scene2_id, Entity{}, {cube_mesh_index});
        auto e4_id = ecs.AddEntity(scene2_id, Entity{
            .position = {-2.f, 1.f, 0.f, 1.f}
        }, {pyramid_mesh_index});

        auto cam3_id = ecs.AddCamera(Camera{}, Camera::Perspective);
    }

    ecs.MarkReady();
    
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
        auto cameras_begin = ecs.GetProviderBegin<Camera>();
        auto cameras_end = ecs.GetProviderEnd<Camera>();
        for (auto camera_it = cameras_begin; camera_it != cameras_end; camera_it++) {
            auto camera_group_ptr = dynamic_cast<Camera::ReflectableGroup*>(*camera_it);
            if (!camera_group_ptr)
                continue;
            auto& camera_group = *camera_group_ptr;
            if (camera_group.open_in_editor) {
                if (!ImGui::Begin(camera_group.imgui_name.c_str(), &camera_group.open_in_editor)) {
                    ImGui::End();
                    continue;
                }
                auto panel_pos = ImGui::GetCursorScreenPos();
                auto panel_size = ImGui::GetContentRegionAvail();

                ImGui::Image((ImTextureID)camera_group.frame_image_ds, panel_size, ImVec2(0, 1), ImVec2(1, 0));
                ecs.ResizeFramebuffer(camera_group.id, panel_size.x, panel_size.y);

                // auto reflectable_group_ptr = SelectedReflectableGroup;
                // auto entity_group_ptr = dynamic_cast<Entity::ReflectableGroup*>(reflectable_group_ptr);
                // if (entity_group_ptr) {
                //     auto& entity_group = *entity_group_ptr;
                //     auto entity_ptr = ecs.GetEntity(entity_group.id);
                //     assert(entity_ptr);
                //     auto& entity = *entity_ptr;
                //     try {
                //         static auto manipulate_type = ImGuizmo::TRANSLATE;
                //         if (ImGui::IsKeyPressed(ImGuiKey_T)) {
                //             manipulate_type = ImGuizmo::TRANSLATE;
                //         }
                //         else if (ImGui::IsKeyPressed(ImGuiKey_S)) {
                //             manipulate_type = ImGuizmo::SCALE;
                //         }
                //         else if (ImGui::IsKeyPressed(ImGuiKey_R)) {
                //             manipulate_type = ImGuizmo::ROTATE;
                //         }
                //         auto& entity_position = entity.position;
                //         auto& entity_rotation = entity.rotation;
                //         auto& entity_scale = entity.scale;
                //         auto& model = entity.model;

                //         ImGuizmo::SetDrawlist();
                //         ImGuizmo::SetRect(panel_pos.x, panel_pos.y, panel_size.x, panel_size.y);
                //         auto camera_ptr = ecs.GetCamera(camera_index);
                //         assert(camera_ptr);
                //         auto& camera = *camera_ptr;
                //         ImGuizmo::SetOrthographic(Camera::ProjectionType(camera.type) == Camera::Orthographic);
                //         auto& camera_view = camera.view;
                //         auto camera_projection = camera.projection;
                        
                //         ImGuizmo::Manipulate(
                //             &camera_view[0][0],
                //             &camera_projection[0][0],
                //             manipulate_type,
                //             ImGuizmo::LOCAL,
                //             &model[0][0],
                //             nullptr
                //         );
                //         if (ImGuizmo::IsUsing())
                //         {
                //             float pos[3] = {0};
                //             float rot[3] = {0};
                //             float scale[3] = {0};
                //             ImGuizmo::DecomposeMatrixToComponents(&model[0][0], pos, rot, scale);
                //             entity_position[0] = pos[0];
                //             entity_position[1] = pos[1];
                //             entity_position[2] = pos[2];
                //             entity_scale[0] = scale[0];
                //             entity_scale[1] = scale[1];
                //             entity_scale[2] = scale[2];
                //             entity_rotation[0] = -radians(rot[0]);
                //             entity_rotation[1] = -radians(rot[1]);
                //             entity_rotation[2] = -radians(rot[2]);
                //             // std::cout << "rot[0]: " << entity_rotation[0] << ", rot[1]: " << entity_rotation[1] << ", rot[2]: " << entity_rotation[2] << std::endl;
                //         }
                //     }
                //     catch (...) { }
                // }
            
                ImGui::End();
            }
        }
    });

    imgui.AddImmediateDrawFunction(2.1f, "Materials", [&](auto& layer) mutable
    {
        if (!ImGui::Begin("Material Browser", &ecs.material_browser_open))
        {
            ImGui::End();
            return;
        }

        float padding = 24.0f;
        float item_size = 84.0f;
        ImVec2 item_size_vec(item_size, item_size);
        float label_height = ImGui::GetFontSize();
        float full_item_height = item_size + label_height + 8.0f;
        float cell_size = item_size + padding;
        float panel_width = ImGui::GetContentRegionAvail().x;
        int columns = (int)(panel_width / cell_size);
        if (columns < 1)
            columns = 1;

        int item_index = 0;

        ImGui::BeginChild("MaterialGrid", ImVec2(0, 0), false, ImGuiWindowFlags_AlwaysUseWindowPadding);

        auto materials_begin = ecs.GetProviderBegin<Material>();
        auto materials_end = ecs.GetProviderEnd<Material>();

        for (auto material_it = materials_begin; material_it != materials_end; material_it++, item_index++)
        {
            auto material_group_ptr = dynamic_cast<Material::ReflectableGroup*>(*material_it);
            if (!material_group_ptr)
                continue;
            auto& material_group = *material_group_ptr;
            auto& material = ecs.GetProviderData<Material>(material_group.id);

            ImGui::PushID(item_index);

            ImGui::BeginGroup();
            {
                ImVec2 group_start = ImGui::GetCursorScreenPos();

                ImVec2 cursor = ImGui::GetCursorScreenPos();
                ImVec2 rect_min = cursor;
                ImVec2 rect_max = ImVec2(cursor.x + item_size, cursor.y + item_size);
                ImDrawList* draw_list = ImGui::GetWindowDrawList();
                static float rounding = 6.0f;

                bool has_texture = !material.atlas_pack.all(-1.f);

                if (has_texture)
                {
                    draw_list->AddImageRounded(
                        (ImTextureID)material_group.frame_image_ds,
                        rect_min,
                        rect_max,
                        ImVec2(0, 0),
                        ImVec2(1, 1),
                        IM_COL32_WHITE,
                        rounding
                    );
                    draw_list->AddRect(
                        rect_min,
                        rect_max,
                        IM_COL32_WHITE,
                        rounding,
                        0,
                        1.0f
                    );
                    ImGui::Dummy(item_size_vec);
                }
                else
                {
                    ImU32 color = ImGui::ColorConvertFloat4ToU32(ImVec4(
                        material.albedo[0],
                        material.albedo[1],
                        material.albedo[2],
                        material.albedo[3]
                    ));

                    draw_list->AddRectFilled(rect_min, rect_max, color, rounding);
                    draw_list->AddRect(rect_min, rect_max, IM_COL32_WHITE, rounding);
                    ImGui::Dummy(item_size_vec);
                }

                ImVec2 text_size = ImGui::CalcTextSize(material_group.name.c_str());
                float text_x = cursor.x + (item_size - text_size.x) * 0.5f;
                ImGui::SetCursorScreenPos(ImVec2(text_x, cursor.y + item_size + 2.0f));
                ImGui::TextUnformatted(material_group.name.c_str());

                ImGui::Dummy(ImVec2(item_size, 0));
            }
            ImGui::EndGroup();

            ImVec2 group_min = ImGui::GetItemRectMin();
            ImVec2 group_max = ImGui::GetItemRectMax();

            if (ImGui::IsMouseHoveringRect(group_min, group_max))
            {
                ImGui::SetTooltip("Material: %s", material_group.name.c_str());
                if (ImGui::IsMouseClicked(ImGuiMouseButton_Left))
                {
                    SelectedReflectableGroup = material_group_ptr;
                    SelectedReflectableID = material_group.id;
                    property_editor.is_open = true;
                }
            }

            ImGui::PopID();

            if ((item_index + 1) % columns != 0)
                ImGui::SameLine();
        }

        ImGui::EndChild();
        ImGui::End();
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
                        DrawWindowGroup(window_name, window_reflectable_group);
                }
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
                for (size_t i = 0; i < ecs.reflectable_group_root_vector.size(); ++i) {
                    auto& reflectable_group_ptr = ecs.reflectable_group_root_vector[i];
                    auto& reflectable_group = *reflectable_group_ptr;
                    DrawGenericGroup(reflectable_group);
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
                ImGui::TextDisabled("GID: 0x%08X", SelectedReflectableID);
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
                    ImGui::PushID(&reflectable);
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
                            if (ImGui::DragFloat2("##vec2", static_cast<float*>(data_ptr), 0.01f, -1000.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                                reflectable.NotifyChange(0);
                                update_iterators();
                            }
                            break;
                        }
                        case ReflectableTypeHint::VEC3:
                        {
                            auto data_ptr = reflectable.GetVoidPropertyByIndex(0);
                            if (ImGui::DragFloat3("##vec3", static_cast<float*>(data_ptr), 0.01f, -1000.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                                reflectable.NotifyChange(0);
                                update_iterators();
                            }
                            break;
                        }
                        case ReflectableTypeHint::VEC3_RGB:
                        {
                            auto data_ptr = reflectable.GetVoidPropertyByIndex(0);
                            if (ImGui::ColorEdit3("##rgb", static_cast<float*>(data_ptr), ImGuiColorEditFlags_Float)) {
                                reflectable.NotifyChange(0);
                                update_iterators();
                            }
                            break;
                        }
                        case ReflectableTypeHint::VEC4:
                        {
                            auto data_ptr = reflectable.GetVoidPropertyByIndex(0);
                            if (ImGui::DragFloat4("##vec4", static_cast<float*>(data_ptr), 0.01f, -1000.0f, 1000.0f, "%.3f", ImGuiSliderFlags_AlwaysClamp)) {
                                reflectable.NotifyChange(0);
                                update_iterators();
                            }
                            break;
                        }
                        case ReflectableTypeHint::VEC4_RGBA:
                        {
                            auto data_ptr = reflectable.GetVoidPropertyByIndex(0);
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
                
                    ImGui::PopID();
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

std::pair<size_t, int> AddPyramidMesh(ExampleECS& ecs, int material_index)
{
    std::vector<vec<float, 4>> positions = {
        // Base triangle 1
        { -1.0f, 0.0f, -1.0f, 1.0f },
        {  1.0f, 0.0f, -1.0f, 1.0f },
        {  1.0f, 0.0f,  1.0f, 1.0f },

        // Base triangle 2
        {  1.0f, 0.0f,  1.0f, 1.0f },
        { -1.0f, 0.0f,  1.0f, 1.0f },
        { -1.0f, 0.0f, -1.0f, 1.0f },

        // Side 1
        {  1.0f, 0.0f, -1.0f, 1.0f },
        { -1.0f, 0.0f, -1.0f, 1.0f },
        {  0.0f, 1.0f,  0.0f, 1.0f },

        // Side 2
        {  1.0f, 0.0f,  1.0f, 1.0f },
        {  1.0f, 0.0f, -1.0f, 1.0f },
        {  0.0f, 1.0f,  0.0f, 1.0f },

        // Side 3
        { -1.0f, 0.0f,  1.0f, 1.0f },
        {  1.0f, 0.0f,  1.0f, 1.0f },
        {  0.0f, 1.0f,  0.0f, 1.0f },

        // Side 4
        { -1.0f, 0.0f, -1.0f, 1.0f },
        { -1.0f, 0.0f,  1.0f, 1.0f },
        {  0.0f, 1.0f,  0.0f, 1.0f }
    };

    std::vector<vec<float, 2>> uv2s = {
        { 1.0f, 0.0f }, { 0.0f, 0.0f }, { 1.0f, 1.0f },
        { 0.0f, 1.0f }, { 1.0f, 1.0f }, { 0.0f, 0.0f },
        { 1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.5f, 1.0f },
        { 1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.5f, 1.0f },
        { 1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.5f, 1.0f },
        { 1.0f, 0.0f }, { 0.0f, 0.0f }, { 0.5f, 1.0f }
    };

    std::vector<vec<float, 4>> normals = {
        { 0.0f, -1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f, 0.0f },
        { 0.0f, -1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f, 0.0f }, { 0.0f, -1.0f, 0.0f, 0.0f },
        { 0.0f, 0.707f, -0.707f, 0.0f }, { 0.0f, 0.707f, -0.707f, 0.0f }, { 0.0f, 0.707f, -0.707f, 0.0f },
        { 0.707f, 0.707f, 0.0f, 0.0f }, { 0.707f, 0.707f, 0.0f, 0.0f }, { 0.707f, 0.707f, 0.0f, 0.0f },
        { 0.0f, 0.707f, 0.707f, 0.0f }, { 0.0f, 0.707f, 0.707f, 0.0f }, { 0.0f, 0.707f, 0.707f, 0.0f },
        { -0.707f, 0.707f, 0.0f, 0.0f }, { -0.707f, 0.707f, 0.0f, 0.0f }, { -0.707f, 0.707f, 0.0f, 0.0f }
    };

    int mesh_index = -1;
    auto mesh_id = ecs.AddMesh(positions, uv2s, normals, material_index, mesh_index);
    return { mesh_id, mesh_index };
}

std::pair<size_t, int> AddCubeMesh(ExampleECS& ecs, int material_index)
{
    std::vector<vec<float, 4>> positions = {
        { 0.5,  0.5,  0.5, 1.0}, {-0.5,  0.5,  0.5, 1.0}, {-0.5, -0.5,  0.5, 1.0},
        {-0.5, -0.5,  0.5, 1.0}, { 0.5, -0.5,  0.5, 1.0}, { 0.5,  0.5,  0.5, 1.0},

        {-0.5,  0.5, -0.5, 1.0}, { 0.5,  0.5, -0.5, 1.0}, { 0.5, -0.5, -0.5, 1.0},
        { 0.5, -0.5, -0.5, 1.0}, {-0.5, -0.5, -0.5, 1.0}, {-0.5,  0.5, -0.5, 1.0},

        {-0.5,  0.5,  0.5, 1.0}, {-0.5,  0.5, -0.5, 1.0}, {-0.5, -0.5, -0.5, 1.0},
        {-0.5, -0.5, -0.5, 1.0}, {-0.5, -0.5,  0.5, 1.0}, {-0.5,  0.5,  0.5, 1.0},

        { 0.5,  0.5, -0.5, 1.0}, { 0.5,  0.5,  0.5, 1.0}, { 0.5, -0.5,  0.5, 1.0},
        { 0.5, -0.5,  0.5, 1.0}, { 0.5, -0.5, -0.5, 1.0}, { 0.5,  0.5, -0.5, 1.0},

        { 0.5,  0.5, -0.5, 1.0}, {-0.5,  0.5, -0.5, 1.0}, {-0.5,  0.5,  0.5, 1.0},
        {-0.5,  0.5,  0.5, 1.0}, { 0.5,  0.5,  0.5, 1.0}, { 0.5,  0.5, -0.5, 1.0},

        { 0.5, -0.5,  0.5, 1.0}, {-0.5, -0.5,  0.5, 1.0}, {-0.5, -0.5, -0.5, 1.0},
        {-0.5, -0.5, -0.5, 1.0}, { 0.5, -0.5, -0.5, 1.0}, { 0.5, -0.5,  0.5, 1.0}
    };

    std::vector<vec<float, 2>> uv2s = {
        {1.0, 1.0}, {0.0, 1.0}, {0.0, 0.0},
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 1.0},

        {0.0, 1.0}, {1.0, 1.0}, {1.0, 0.0},
        {1.0, 0.0}, {0.0, 0.0}, {0.0, 1.0},

        {0.0, 1.0}, {0.0, 1.0}, {0.0, 0.0},
        {0.0, 0.0}, {0.0, 0.0}, {0.0, 1.0},

        {1.0, 1.0}, {1.0, 1.0}, {1.0, 0.0},
        {1.0, 0.0}, {1.0, 0.0}, {1.0, 1.0},

        {1.0, 1.0}, {0.0, 1.0}, {0.0, 1.0},
        {0.0, 1.0}, {1.0, 1.0}, {1.0, 1.0},

        {1.0, 0.0}, {0.0, 0.0}, {0.0, 0.0},
        {0.0, 0.0}, {1.0, 0.0}, {1.0, 0.0}
    };

    std::vector<vec<float, 4>> normals = {
        {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0}, {0, 0, 1, 0},
        {0, 0, -1, 0}, {0, 0, -1, 0}, {0, 0, -1, 0}, {0, 0, -1, 0}, {0, 0, -1, 0}, {0, 0, -1, 0},
        {-1, 0, 0, 0}, {-1, 0, 0, 0}, {-1, 0, 0, 0}, {-1, 0, 0, 0}, {-1, 0, 0, 0}, {-1, 0, 0, 0},
        {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0}, {1, 0, 0, 0},
        {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0}, {0, 1, 0, 0},
        {0, -1, 0, 0}, {0, -1, 0, 0}, {0, -1, 0, 0}, {0, -1, 0, 0}, {0, -1, 0, 0}, {0, -1, 0, 0}
    };

    int mesh_index = -1;
    auto mesh_id = ecs.AddMesh(positions, uv2s, normals, material_index, mesh_index);
    return { mesh_id, mesh_index };
}

std::pair<size_t, int> AddPlaneMesh(ExampleECS& ecs, int material_index)
{
    std::vector<vec<float, 4>> positions = {
        { 0.5,  0.5, 0, 1},
        {-0.5,  0.5, 0, 1},
        {-0.5, -0.5, 0, 1},
        {-0.5, -0.5, 0, 1},
        { 0.5, -0.5, 0, 1},
        { 0.5,  0.5, 0, 1}
    };

    std::vector<vec<float, 2>> uv2s = {
        {1.0, 0.0},
        {0.0, 0.0},
        {0.0, 1.0},
        {0.0, 1.0},
        {1.0, 1.0},
        {1.0, 0.0}
    };

    std::vector<vec<float, 4>> normals = {
        {0.0, 0.0, 1.0, 1},
        {0.0, 0.0, 1.0, 1},
        {0.0, 0.0, 1.0, 1},
        {0.0, 0.0, 1.0, 1},
        {0.0, 0.0, 1.0, 1},
        {0.0, 0.0, 1.0, 1}
    };

    int mesh_index = -1;
    auto mesh_id = ecs.AddMesh(positions, uv2s, normals, material_index, mesh_index);
    return { mesh_id, mesh_index };
}

// void DrawEntityGroup(ExampleECS& ecs, int id, ExampleECS::EntityReflectableGroup& entity_group)
// {
//     ImGui::TableNextRow();
//     ImGui::TableNextColumn();
//     ImGui::PushID(&entity_group);
//     ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_None;
//     tree_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;// Standard opening mode as we are likely to want to add selection afterwards
//     tree_flags |= ImGuiTreeNodeFlags_NavLeftJumpsToParent;  // Left arrow support
//     tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;         // Span full width for easier mouse reach
//     tree_flags |= ImGuiTreeNodeFlags_DrawLinesToNodes;      // Always draw hierarchy outlines
//     if (&entity_group == SelectedReflectableGroup)
//         tree_flags |= ImGuiTreeNodeFlags_Selected;
//     if (entity_group.children.empty())
//         tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
//     if (entity_group.disabled)
//         ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
//     bool node_open = ImGui::TreeNodeEx("", tree_flags, "%s", entity_group.name.c_str());
//     if (entity_group.disabled)
//         ImGui::PopStyleColor();
//     if (ImGui::IsItemFocused()) {
//         SelectedReflectableGroup = &entity_group;
//         SelectedReflectableID = id;
//         property_editor.is_open = true;
//     }

//     if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
//         ImGui::OpenPopup("EntityContextMenu");
//     }

//     if (ImGui::BeginPopup("EntityContextMenu")) {
//         if (ImGui::BeginMenu("Add Child")) {
//             bool add = false;
//             int shape_index = -1;
//             if (ImGui::MenuItem("Plane")) {
//                 add = true;
//                 shape_index = plane_shape_index;
//             }
//             if (ImGui::MenuItem("Cube")) {
//                 add = true;
//                 shape_index = cube_shape_index;
//             }
//             if (ImGui::MenuItem("Monkey")) {
//                 // ecs.AddEntityToScene(scene_id, "Monkey");
//             }
//             if (ImGui::MenuItem("Mesh")) {
//                 // ecs.AddEntityToScene(scene_id, "Mesh");
//             }
//             if (add) {
//                 // auto child_id = ecs.AddChildEntity(entity_group.id, Entity{});
//                 // auto child_ptr = ecs.GetEntity(child_id);
//                 // assert(child_ptr);
//                 // auto& child = *child_ptr;
//                 // child.shape_index = shape_index;
//             }
//             ImGui::EndMenu();
//         }

//         if (ImGui::MenuItem("Duplicate Entity")) {
//             // ecs.DuplicateEntity(id);
//         }
//         if (ImGui::MenuItem("Delete Entity")) {
//             // ecs.DeleteEntity(id);
//         }

//         ImGui::EndPopup();
//     }

//     if (node_open) {
//         // for (auto& child_group_sh_ptr : entity_group.reflectable_children) {
//         //     auto child_group_ptr = child_group_sh_ptr.get();
//         //     if (ReflectableGroupFilterCheck(SceneGraphFilter, *child_group_ptr)) {
//         //         auto entity_child_group_ptr = dynamic_cast<ExampleECS::EntityReflectableGroup*>(child_group_ptr);
//         //         if (entity_child_group_ptr) {
//         //             DrawEntityGroup(ecs, entity_child_group_ptr->id, *entity_child_group_ptr);
//         //         }
//         //     }
//         // }
//         ImGui::TreePop();
//     }
//     ImGui::PopID();
// }

// void DrawSceneReflectableGroup(ExampleECS& ecs, int scene_id, ExampleECS::SceneReflectableGroup& scene_group) {
//     ImGui::TableNextRow();
//     ImGui::TableNextColumn();
//     ImGui::PushID(&scene_group);
//     ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_None;
//     tree_flags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick;// Standard opening mode as we are likely to want to add selection afterwards
//     tree_flags |= ImGuiTreeNodeFlags_NavLeftJumpsToParent;  // Left arrow support
//     tree_flags |= ImGuiTreeNodeFlags_SpanFullWidth;         // Span full width for easier mouse reach
//     tree_flags |= ImGuiTreeNodeFlags_DrawLinesToNodes;      // Always draw hierarchy outlines
//     if (&scene_group == SelectedReflectableGroup)
//         tree_flags |= ImGuiTreeNodeFlags_Selected;
//     auto& scene_group_children = scene_group.GetChildren();
//     if (scene_group_children.empty())
//         tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
//     if (scene_group.disabled)
//         ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);
//     bool node_open = ImGui::TreeNodeEx("", tree_flags, "%s", scene_group.name.c_str());
//     if (scene_group.disabled)
//         ImGui::PopStyleColor();
//     if (ImGui::IsItemFocused()) {
//         SelectedReflectableGroup = &scene_group;
//         SelectedReflectableID = scene_id;
//         property_editor.is_open = true;
//     }

//     if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
//         ImGui::OpenPopup("SceneContextMenu");
//     }

//     if (ImGui::BeginPopup("SceneContextMenu")) {
//         if (ImGui::BeginMenu("Add Entity")) {
//             if (ImGui::MenuItem("Plane")) {
//                 // ecs.AddEntityToScene(scene_id, "Plane");
//             }
//             if (ImGui::MenuItem("Cube")) {
//                 // ecs.AddEntityToScene(scene_id, "Cube");
//             }
//             if (ImGui::MenuItem("Monkey")) {
//                 // ecs.AddEntityToScene(scene_id, "Monkey");
//             }
//             if (ImGui::MenuItem("Mesh")) {
//                 // ecs.AddEntityToScene(scene_id, "Mesh");
//             }
//             ImGui::EndMenu();
//         }

//         if (ImGui::BeginMenu("Add Camera")) {
//             if (ImGui::MenuItem("Perspective")) {
//                 // ecs.AddCamera(scene_id, Camera::Perspective);
//             }
//             if (ImGui::MenuItem("Orthographic")) {
//                 // ecs.AddCamera(scene_id, Camera::Orthographic);
//             }
//             ImGui::EndMenu();
//         }

//         if (ImGui::BeginMenu("Add Light")) {
//             if (ImGui::MenuItem("Directional")) {
//                 // ecs.AddLight(scene_id, Light::Directional);
//             }
//             if (ImGui::MenuItem("Spot")) {
//                 // ecs.AddLight(scene_id, Light::Spot);
//             }
//             if (ImGui::MenuItem("Point")) {
//                 // ecs.AddLight(scene_id, Light::Point);
//             }
//             ImGui::EndMenu();
//         }

//         ImGui::EndPopup();
//     }

//     if (node_open) {
//         // for (auto& child_group_ptr : scene_group_children) {
//         //     auto& child_group = *child_group_ptr;
//         //     if (ReflectableGroupFilterCheck(SceneGraphFilter, child_group)) {
//         //         switch (child_group.GetGroupType()) {
//         //         case ReflectableGroup::Window:
//         //             assert(false);
//         //             break;
//         //         case ReflectableGroup::Generic:
//         //         case ReflectableGroup::Camera:
//         //         case ReflectableGroup::Light:
//         //         case ReflectableGroup::Provider:
//         //             DrawGenericGroup(child_group.id, child_group);
//         //             break;
//         //         case ReflectableGroup::Scene:
//         //             DrawSceneReflectableGroup(ecs, child_group.id, dynamic_cast<ExampleECS::SceneReflectableGroup&>(child_group));
//         //             break;
//         //         case ReflectableGroup::Entity: {
//         //             auto& entity_group = dynamic_cast<ExampleECS::EntityReflectableGroup&>(child_group);
//         //             if (!entity_group.is_child)
//         //                 DrawEntityGroup(ecs, child_group.id, entity_group);
//         //             break;
//         //         }
//         //         }
//         //     }
//         // }
//         ImGui::TreePop();
//     }
//     ImGui::PopID();
// }

void DrawDropTarget(ReflectableGroup& target_group, ReflectableGroup* dragged_group, float y_distance);

void DrawGenericGroup(ReflectableGroup& reflectable_group)
{
    ImGui::TableNextRow();
    ImGui::TableNextColumn();

    ImGui::PushID(&reflectable_group);

    ImGuiTreeNodeFlags tree_flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_NavLeftJumpsToParent | ImGuiTreeNodeFlags_SpanFullWidth | ImGuiTreeNodeFlags_DrawLinesToNodes;

    if (&reflectable_group == SelectedReflectableGroup)
        tree_flags |= ImGuiTreeNodeFlags_Selected;

    auto& group_children = reflectable_group.GetChildren();
    bool is_leaf = group_children.empty();
    if (is_leaf) {
        tree_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;
    }

    if (reflectable_group.disabled)
        ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

    auto& is_open = reflectable_group.tree_node_open;
    if (is_open)
        ImGui::SetNextItemOpen(is_open);

    bool node_open = ImGui::TreeNodeEx("", tree_flags, "%s", reflectable_group.GetName().c_str());

    if (node_open != is_open)
        is_open = node_open;

    if (reflectable_group.disabled)
        ImGui::PopStyleColor();

    if (ImGui::IsItemFocused()) {
        SelectedReflectableGroup = &reflectable_group;
        SelectedReflectableID = reflectable_group.id;
        property_editor.is_open = true;
    }

    // Handle drag and drop source
    if (ImGui::BeginDragDropSource()) {
        ReflectableGroup* reflectable_group_ptr = &reflectable_group;
        ImGui::SetDragDropPayload("REFLECTABLE_GROUP", &reflectable_group_ptr, sizeof(ReflectableGroup*));
        ImGui::TextUnformatted(reflectable_group.GetName().c_str());
        ImGui::EndDragDropSource();
    }

    // Target drop handler on top of this node (for insert or reparent)
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("REFLECTABLE_GROUP")) {
            if (payload->DataSize == sizeof(ReflectableGroup*)) {
                ReflectableGroup* dragged = *static_cast<ReflectableGroup* const*>(payload->Data);
                if (dragged != &reflectable_group && !reflectable_group.IsDescendantOf(dragged)) {
                    DrawDropTarget(reflectable_group, dragged, 0.0f);
                }
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (node_open) {
        for (size_t i = 0; i < group_children.size(); ++i) {
            ImGui::PushID(static_cast<int>(i));
            DrawGenericGroup(*group_children[i]);
            ImGui::PopID();
        }
        ImGui::TreePop();
    }

    // Add dummy drop zone if this node is leaf and can hold children
    bool can_hold_children = (reflectable_group.cid != Camera::PID && reflectable_group.cid != SubMesh::PID);
    if (is_leaf && can_hold_children && ImGui::GetDragDropPayload() != nullptr) {
        ImVec2 cursor = ImGui::GetCursorScreenPos();
        ImVec2 mouse = ImGui::GetIO().MousePos;
        float distance = fabs(cursor.y - mouse.y);
        if (distance < 20.0f) {
            ImGui::Dummy(ImVec2(ImGui::GetContentRegionAvail().x, 4.0f));
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("REFLECTABLE_GROUP")) {
                    if (payload->DataSize == sizeof(ReflectableGroup*)) {
                        ReflectableGroup* dragged = *static_cast<ReflectableGroup* const*>(payload->Data);
                        DrawDropTarget(reflectable_group, dragged, -1.0f);
                    }
                }
                ImGui::EndDragDropTarget();
            }
        }
    }

    ImGui::PopID();
}

void DrawDropTarget(ReflectableGroup& target_group, ReflectableGroup* dragged_group, float y_distance)
{
    if (dragged_group == nullptr || dragged_group == &target_group || target_group.IsDescendantOf(dragged_group))
        return;

    ReflectableGroup* old_parent = dragged_group->parent_ptr;
    std::vector<std::shared_ptr<ReflectableGroup>>* old_siblings = old_parent ? &old_parent->GetChildren() : &ecs_ptr->reflectable_group_root_vector;
    auto it = std::find_if(old_siblings->begin(), old_siblings->end(), [&](auto& ptr) {
        return ptr.get() == dragged_group;
    });
    if (it == old_siblings->end())
        return;

    std::shared_ptr<ReflectableGroup> dragged_ptr = *it;
    old_siblings->erase(it);

    ReflectableGroup* new_parent_ptr = nullptr;
    if (y_distance >= 0.0f && y_distance < 8.0f) {
        // Insert before
        new_parent_ptr = target_group.parent_ptr;
        std::vector<std::shared_ptr<ReflectableGroup>>* siblings = new_parent_ptr ? &new_parent_ptr->GetChildren() : &ecs_ptr->reflectable_group_root_vector;
        auto pos = std::find_if(siblings->begin(), siblings->end(), [&](auto& ptr) {
            return ptr.get() == &target_group;
        });
        if (pos != siblings->end())
            siblings->insert(pos, dragged_ptr);
    }
    else if (y_distance >= 24.0f) {
        // Insert after
        new_parent_ptr = target_group.parent_ptr;
        std::vector<std::shared_ptr<ReflectableGroup>>* siblings = new_parent_ptr ? &new_parent_ptr->GetChildren() : &ecs_ptr->reflectable_group_root_vector;
        auto pos = std::find_if(siblings->begin(), siblings->end(), [&](auto& ptr) {
            return ptr.get() == &target_group;
        });
        if (pos != siblings->end())
            siblings->insert(pos + 1, dragged_ptr);
    } else {
        // Drop into
        target_group.GetChildren().push_back(dragged_ptr);
        new_parent_ptr = &target_group;
    }
    dragged_group->parent_ptr = new_parent_ptr;


    switch (dragged_group->cid) {
    case Entity::PID: {
        auto& entity = ecs_ptr->GetEntity(dragged_group->id);
        ExampleECS::SetWhoParent(entity, new_parent_ptr);
        break;
    }
    case Camera::PID: {
        auto& camera = ecs_ptr->GetCamera(dragged_group->id);
        ExampleECS::SetWhoParent(camera, new_parent_ptr);
        break;
    }
    case Scene::PID: {
        auto& scene = ecs_ptr->GetScene(dragged_group->id);
        ExampleECS::SetWhoParent(scene, new_parent_ptr);
        break;
    }
    case SubMesh::PID: {
        auto& submesh = ecs_ptr->GetSubMesh(dragged_group->id);
        ExampleECS::SetWhoParent(submesh, new_parent_ptr);
        break;
    }
    }

    ecs_ptr->MarkDirty();
}

void DrawWindowGroup(const std::string& window_name, WindowReflectableGroup& window_reflectable_group) {
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
        //         DrawEntityGroup(ecs, child_id, child_group);
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