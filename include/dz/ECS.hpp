#pragma once
#include "GlobalUID.hpp"
#include <unordered_map>
#include <memory>
#include "BufferGroup.hpp"
#include "Framebuffer.hpp"
#include "DrawListManager.hpp"
#include "Window.hpp"
#include "Shader.hpp"
#include "Util.hpp"
#include "State.hpp"
#include "ECS/Provider.hpp"
#include "ECS/Light.hpp"
#include "ECS/Scene.hpp"
#include "ECS/Entity.hpp"
#include "ECS/Mesh.hpp"
#include "ECS/SubMesh.hpp"
#include "ECS/Camera.hpp"
#include "ECS/Material.hpp"
#include "ImagePack.hpp"
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <set>
#include <iostreams/Serial.hpp>
#include <fstream>
#include <mutex>

namespace dz {

    inline static std::string Cameras_Str = "Cameras";
    inline static std::string Atlas_Str = "Atlas";
    inline static std::string VertexPositions_Str = "VertexPositions";
    inline static std::string VertexUV2s_Str = "VertexUV2s";
    inline static std::string VertexNormals_Str = "VertexNormals";
    
    template<int TCID, typename... TProviders>
    struct ECS : Restorable {

        inline static int CID = TCID;
	    int getCID() override { return TCID; }

        inline static void RegisterStateCID() {
            register_restorable_constructor(CID, [](Serial& serial) -> Restorable* {
                return new ECS(serial);
            });
        }

        struct RegisteredComponentEntry {
            int type_id;
            std::string struct_name;
            std::string buffer_name;
            std::string sparse_name;
            int constructed_count = 0;
        };

        struct ProviderReflectableGroup : ReflectableGroup {
            float priority;
            std::string name;
            std::string struct_name;
            std::string buffer_name;
            std::string sparse_name;
            std::string glsl_struct;
            std::unordered_map<ShaderModuleType, std::string> glsl_methods;
            std::unordered_map<ShaderModuleType, std::vector<std::string>> glsl_layouts;
            std::string glsl_main;
            bool is_component = false;
            int component_id = 0;

            GroupType GetGroupType() override {
                return ReflectableGroup::Provider;
            }
            std::string& GetName() override {
                return name;
            }
        };

        std::recursive_mutex e_mutex; // !

        std::string buffer_name; // !

        WINDOW* window_ptr = 0; // !
        bool loaded_from_io = false; // !

        std::vector<std::shared_ptr<ReflectableGroup>> reflectable_group_root_vector; // Y
        std::vector<std::shared_ptr<ReflectableGroup>> material_group_vector; // Y
        std::vector<std::shared_ptr<ReflectableGroup>> mesh_group_vector; // Y
        std::map<size_t, std::vector<ReflectableGroup*>> pid_reflectable_vecs; // !
        std::unordered_map<size_t, std::unordered_map<size_t, size_t>> pid_id_index_maps; // !

        std::unordered_map<size_t, std::function<std::shared_ptr<ReflectableGroup>(BufferGroup*, Serial&)>> pid_create_from_serial; // !

        std::map<size_t, ProviderReflectableGroup> pid_provider_groups; // !
        std::map<float, std::vector<size_t>> prioritized_provider_ids; // !
        std::unordered_map<ShaderModuleType, std::map<float, std::vector<std::string>>> priority_glsl_mains; // !

        std::vector<std::string> restricted_keys{
            Atlas_Str,
            VertexPositions_Str,
            VertexUV2s_Str,
            VertexNormals_Str
        }; // !
        std::map<int, RegisteredComponentEntry> registered_component_map; // !
        bool components_registered = false; // !

        using DrawProviderT = typename FirstMatchingOrDefault<IsDrawProvider, TProviders...>::type;
        using SubMeshProviderT = typename FirstMatchingOrDefault<IsSubMeshProvider, TProviders...>::type;
        using EntityProviderT = typename FirstMatchingOrDefault<IsEntityProvider, TProviders...>::type;
        using CameraProviderT = typename FirstMatchingOrDefault<IsCameraProvider, TProviders...>::type;
        using SceneProviderT = typename FirstMatchingOrDefault<IsSceneProvider, TProviders...>::type;
        using MaterialProviderT = typename FirstMatchingOrDefault<IsMaterialProvider, TProviders...>::type;
        using MeshProviderT = typename FirstMatchingOrDefault<IsMeshProvider, TProviders...>::type;
        DrawListManager<DrawProviderT> draw_mg; // !

        BufferGroup* buffer_group = nullptr; // !
        bool buffer_initialized = false; // !
        int buffer_size = 0; // !
        
        Shader* main_shader = nullptr; // !
        Shader* model_compute_shader = nullptr; // !
        Shader* camera_mat_compute_shader = nullptr; // !

        bool material_browser_open = true;

        bool atlas_dirty = false;
        ImagePack atlas_pack;

        auto GenerateEntitysDrawFunction() {
            return [&](auto buffer_group, auto& draw_object) -> DrawTuples {
                return {
                    {main_shader, draw_object.GetVertexCount(buffer_group, draw_object)}
                };
            };
        }

        auto GenerateCamerasDrawFunction() {
            return [&](auto buffer_group, auto camera_index) -> CameraTuple {
                auto& camera_group = GetGroupByIndex<CameraProviderT, typename CameraProviderT::ReflectableGroup>(camera_index);
                return {camera_index, camera_group.framebuffer, [&, camera_index]() {
                    shader_update_push_constant(main_shader, 0, (void*)&camera_index, sizeof(uint32_t));
                }};
            };
        }

        auto GenerateCameraVisibilityFunction() {
            return [&](auto buffer_group, auto camera_index) -> std::vector<int> {
                std::vector<int> visible;
                auto& camera_group = GetGroupByIndex<CameraProviderT, typename CameraProviderT::ReflectableGroup>(camera_index);
                auto current_parent_group_ptr = camera_group.parent_ptr;
                while (current_parent_group_ptr) {
                    auto scene_group_ptr = dynamic_cast<SceneProviderT::ReflectableGroup*>(current_parent_group_ptr);
                    if (scene_group_ptr) {
                        break;
                    }
                    current_parent_group_ptr = current_parent_group_ptr->parent_ptr;
                }
                auto visible_from_node_ptr = (current_parent_group_ptr) ?
                    &current_parent_group_ptr->GetChildren() :
                    &reflectable_group_root_vector;

                std::function<void(std::vector<int>&, const std::vector<std::shared_ptr<ReflectableGroup>>*, int)> add_child_entries;

                add_child_entries = [&](auto& visible, auto node_ptr, auto scenes_hit) {
                    for (auto& ref_group_sh_ptr : *node_ptr) {
                        auto ref_group_ptr = ref_group_sh_ptr.get();
                        assert(ref_group_ptr);
                        auto e_group_ptr = dynamic_cast<EntityProviderT::ReflectableGroup*>(ref_group_ptr);
                        if (e_group_ptr) {
                            for (auto& submesh_group_sh_ptr : e_group_ptr->reflectable_children) {
                                auto& group = *submesh_group_sh_ptr;
                                if (group.cid != SubMeshProviderT::PID)
                                    continue;
                                visible.push_back(group.index);
                            }
                            add_child_entries(visible, &ref_group_ptr->GetChildren(), scenes_hit);
                            continue;
                        }
                        auto s_group_ptr = dynamic_cast<SceneProviderT::ReflectableGroup*>(ref_group_ptr);
                        if (s_group_ptr && !scenes_hit) {
                            add_child_entries(visible, &ref_group_ptr->GetChildren(), scenes_hit + 1);
                            continue;
                        }
                        auto c_group_ptr = dynamic_cast<CameraProviderT::ReflectableGroup*>(ref_group_ptr);
                        if (c_group_ptr) {
                            // do nothing for now (and ever?)
                            continue;
                        }
                    }
                };
                
                add_child_entries(visible, visible_from_node_ptr, 0);

                return visible;
            };
        }

        template <typename TProvider>
        void IsDrawProviderName(std::string& out_string) {
            if (!out_string.empty())
                return;
            if (TProvider::GetIsDrawProvider()) {
                out_string = (TProvider::GetStructName() + "s");
            }
        }

        std::string FindBufferNameFromProviders() {
            std::string out_string;
            (IsDrawProviderName<TProviders>(out_string), ...);
            return out_string;
        }

        ECS(Serial& serial):
            buffer_name(FindBufferNameFromProviders()),
            window_ptr(state_get_ptr<WINDOW>(CID_WINDOW)),
            draw_mg(
                buffer_name, GenerateEntitysDrawFunction(),
                Cameras_Str, GenerateCamerasDrawFunction(),
                GenerateCameraVisibilityFunction()
            )
        {
            RegisterProviders();
            buffer_group = CreateBufferGroup();
            assert(buffer_group);
            main_shader = GenerateMainShader();
            model_compute_shader = GenerateModelComputeShader();
            camera_mat_compute_shader = GenerateCameraMatComputeShader();
            EnableDrawInWindow(window_ptr);
            restore(serial);
            loaded_from_io = true;
        }

        ECS(WINDOW* initial_window_ptr):
            buffer_name(FindBufferNameFromProviders()),
            window_ptr(initial_window_ptr),
            draw_mg(
                buffer_name, GenerateEntitysDrawFunction(),
                Cameras_Str, GenerateCamerasDrawFunction(),
                GenerateCameraVisibilityFunction()
            )
        {
            RegisterProviders();
            buffer_group = CreateBufferGroup();
            assert(buffer_group);
            main_shader = GenerateMainShader();
            model_compute_shader = GenerateModelComputeShader();
            camera_mat_compute_shader = GenerateCameraMatComputeShader();
            EnableDrawInWindow(window_ptr);
        }

        void MarkReady() {
            UpdateAtlas();
            auto atlas = atlas_pack.getAtlas();
            shader_use_image(main_shader, Atlas_Str, atlas);
            shader_use_image(model_compute_shader, Atlas_Str, atlas);
            shader_use_image(camera_mat_compute_shader, Atlas_Str, atlas);
            if (!buffer_initialized) {
                buffer_group_initialize(buffer_group);
                buffer_initialized = true;
            }
            shader_update_descriptor_sets(main_shader);
        }

        void UpdateAtlas() {
            atlas_pack.check();
            for (auto& material_group_sh_ptr : material_group_vector) {
                auto generic_group_ptr = material_group_sh_ptr.get();
                auto material_group_ptr = dynamic_cast<typename MaterialProviderT::ReflectableGroup*>(generic_group_ptr);
                auto& material_group = *material_group_ptr;
                if (!material_group.image)
                    continue;
                auto& material = GetMaterial(material_group.id);
                auto& atlasImageSize = *(vec<float, 2>*)&material.atlas_pack[0];
                auto& atlasPackedRect = *(vec<float, 2>*)&material.atlas_pack[2];
                auto& packed_rect = atlas_pack.findPackedRect(material_group.image);
                atlasImageSize = {packed_rect.w, packed_rect.h};
                atlasPackedRect = {packed_rect.x, packed_rect.y};
                continue;
            }
        }

        bool backup(Serial& serial) override {
            std::lock_guard lock(e_mutex);
            if (!serial.canWrite())
                return false;
            if (!Backup_pid_reflectable_vecs_sizes(serial))
                return false;
            if (!BackupGroupVector(serial, reflectable_group_root_vector))
                return false;
            if (!BackupGroupVector(serial, material_group_vector))
                return false;
            if (!BackupGroupVector(serial, mesh_group_vector))
                return false;
            if (!BackupBuffers(serial))
                return false;
            return true;
        }

        bool restore(Serial& serial) override {
            std::lock_guard lock(e_mutex);
            if (!serial.canRead() || serial.getReadLength() == 0)
                return false;
            std::lock_guard cid_restore_lock(ReflectableGroup::cid_restore_mutex);
            ReflectableGroup::cid_restore_map_ptr = &pid_create_from_serial;
            ReflectableGroup::pid_reflectable_vecs_ptr = &pid_reflectable_vecs;
            ReflectableGroup::pid_id_index_maps_ptr = &pid_id_index_maps;
            if (!Restore_pid_reflectable_vecs_sizes(serial))
                return false;
            if (!RestoreGroupVector(serial, reflectable_group_root_vector, buffer_group))
                return false;
            if (!RestoreGroupVector(serial, material_group_vector, buffer_group))
                return false;
            if (!RestoreGroupVector(serial, mesh_group_vector, buffer_group))
                return false;
            if (!EnsureGroupVectorParentPtrs(reflectable_group_root_vector, nullptr))
                return false;
            if (!RestoreBuffers(serial))
                return false;
            UpdateGroupsChildren();
            return true;
        }

        bool Backup_pid_reflectable_vecs_sizes(Serial& serial) {
            auto pid_reflectable_vecs_size = pid_reflectable_vecs.size();
            serial << pid_reflectable_vecs_size;
            for (auto& [pid, vec] : pid_reflectable_vecs) {
                size_t vec_size = vec.size();
                serial << pid << vec_size;
            }
            return true;
        }

        bool Restore_pid_reflectable_vecs_sizes(Serial& serial) {
            auto pid_reflectable_vecs_size = pid_reflectable_vecs.size();
            serial >> pid_reflectable_vecs_size;
            for (size_t c = 1; c <= pid_reflectable_vecs_size; c++) {
                size_t pid = 0;
                size_t vec_size = 0;
                serial >> pid >> vec_size;
                pid_reflectable_vecs[pid].resize(vec_size);
            }
            return true;
        }

        bool EnsureGroupVectorParentPtrs(std::vector<std::shared_ptr<ReflectableGroup>>& group_vector, ReflectableGroup* parent_ptr) {
            try {
                for (auto& group_sh_ptr : group_vector) {
                    auto group_ptr = group_sh_ptr.get();
                    if (!group_ptr)
                        throw std::runtime_error("ReflectableGroup pointer is null");
                    auto& group = *group_ptr;
                    group.parent_ptr = parent_ptr;
                    assert(group.cid);
                    if (!EnsureGroupVectorParentPtrs(group.GetChildren(), group_ptr))
                        return false;
                }
                return true;
            }
            catch (...) {
                return false;
            }
        }

        void UpdateGroupsChildren() {
            auto& width = *window_get_width_ref(window_ptr);
            auto& height = *window_get_height_ref(window_ptr);
            constexpr auto cam_pid = CameraProviderT::GetPID();
            auto begin = pid_reflectable_vecs.rbegin();
            auto end = pid_reflectable_vecs.rend();
            for (auto it = begin; it != end; ++it) {
                auto& pid = it->first;
                auto& vec = it->second;
                for (auto ptr : vec) {
                    ptr->UpdateChildren();
                    if (pid == cam_pid) {
                        auto cam_ptr = dynamic_cast<typename CameraProviderT::ReflectableGroup*>(ptr);
                        assert(cam_ptr);
                        cam_ptr->InitFramebuffer(main_shader, width, height);
                    }
                }
            }
        }

        bool RestoreBuffers(Serial& serial) {
            if (!buffer_group)
                return false;
            auto keys_size = restricted_keys.size();
            serial >> keys_size;
            for (size_t key_count = 1; key_count <= keys_size; ++key_count) {
                std::string restricted_key;
                serial >> restricted_key;
                uint32_t element_count, element_size;
                serial >> element_count >> element_size;
                auto buffer_size = element_count * element_size;
                auto r_key_it = std::find(restricted_keys.begin(), restricted_keys.end(), restricted_key);
                if (r_key_it == restricted_keys.end()) {
                    auto bytes = (char*)malloc(buffer_size);
                    serial.readBytes((char*)bytes, buffer_size);
                    free(bytes);
                    continue;
                }
                buffer_group_set_buffer_element_count(buffer_group, restricted_key, element_count);
                auto actual_element_size = buffer_group_get_buffer_element_size(buffer_group, restricted_key);
                if (actual_element_size != element_size)
                    throw std::runtime_error("Incompatible buffer element sizes");
                auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, restricted_key);
                serial.readBytes((char*)buffer_ptr.get(), buffer_size);
            }
            return true;
        }

        bool BackupBuffers(Serial& serial) {
            if (!buffer_group)
                return false;
            auto keys_size = restricted_keys.size();
            serial << keys_size;
            for (auto& restricted_key : restricted_keys) {
                serial << restricted_key;
                auto element_count = buffer_group_get_buffer_element_count(buffer_group, restricted_key);
                auto element_size = buffer_group_get_buffer_element_size(buffer_group, restricted_key);
                serial << element_count << element_size;
                auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, restricted_key);
                serial.writeBytes((const char*)buffer_ptr.get(), element_count * element_size);
            }
            return true;
        }

        void RegisterProviders() {
            (RegisterProvider<TProviders>(), ...);
        }

        template <typename TProvider>
        void RegisterProvider() {
            std::lock_guard lock(e_mutex);
            auto priority = TProvider::GetPriority();
            auto& name = TProvider::GetProviderName();
            auto& struct_name = TProvider::GetStructName();
            auto& glsl_struct = TProvider::GetGLSLStruct();
            auto& glsl_methods = TProvider::GetGLSLMethods();
            auto& glsl_layouts = TProvider::GetGLSLLayouts();
            auto& priority_glsl_main = TProvider::GetGLSLMain();
            constexpr auto pid = TProvider::GetPID();
            prioritized_provider_ids[priority].push_back(pid);
            auto provider_index = pid_provider_groups.size();
            auto& provider_group = pid_provider_groups[pid];
            provider_group.index = provider_index;
            provider_group.id = pid;
            provider_group.name = name;
            provider_group.struct_name = struct_name;
            provider_group.buffer_name = (struct_name + "s");
            provider_group.glsl_struct = glsl_struct;
            provider_group.glsl_methods = glsl_methods;
            provider_group.glsl_layouts = glsl_layouts;
            provider_group.is_component = TProvider::GetIsComponent();
            for (auto& [main_priority, main_string, module_type] : priority_glsl_main) {
                priority_glsl_mains[module_type][main_priority].push_back(main_string);
            }
            if constexpr (TProvider::GetIsComponent()) {
                provider_group.sparse_name = ("Sparse" + struct_name + "s");
                provider_group.component_id = int(TProvider::GetComponentID());
                RegisterComponent<TProvider>();
            }
            pid_create_from_serial[pid] = [](auto buffer_group, auto& serial) {
                return TProvider::TryMakeGroupFromSerial(buffer_group, serial);
            };
        }

        template<typename TRComponent>
        bool RegisterComponent() {
            auto ID = int(TRComponent::GetComponentID());
            auto& StructName = TRComponent::GetStructName();
            registered_component_map[ID] = {
                .type_id = ID,
                .struct_name = StructName,
                .buffer_name = StructName + "s",
                .sparse_name = ("Sparse" + StructName + "s")
            };
            return true;
        }

        void Resize(int n) {
            std::lock_guard lock(e_mutex);
            buffer_group_set_buffer_element_count(buffer_group, buffer_name, n);
            for (auto& [component_id, component_entry] : registered_component_map) {
                auto sparse_buffer_name = component_entry.sparse_name;
                auto sparse_buffer_old_count = buffer_group_get_buffer_element_count(buffer_group, sparse_buffer_name);
                buffer_group_set_buffer_element_count(buffer_group, sparse_buffer_name, n);
                auto sparse_buffer = buffer_group_get_buffer_data_ptr(buffer_group, sparse_buffer_name);
                auto int_ptr = (int*)(sparse_buffer.get());
                for (size_t i = sparse_buffer_old_count; i < n; ++i)
                    int_ptr[i] = -1;
                continue;
            }
            buffer_size = n;
        }

        template <typename T>
        T& InsertReflectableGroup(std::vector<std::shared_ptr<ReflectableGroup>>& children) {
            auto index = children.size();
            children.push_back(std::make_shared<T>());
            return dynamic_cast<T&>(*children[index]);
        }

        template <typename TData, typename TProvider>
        void AddProviderSingle(int parent_id, size_t& id, const TData& data, std::vector<std::shared_ptr<ReflectableGroup>>& reflectable_group_vector, int& out_index) {
            if (id)
                return;

            if constexpr (!(std::is_same_v<TData, TProvider>))
                return;

            constexpr auto pid = TProvider::GetPID();

            auto ecs_id = GlobalUID::GetNew("ECS:GID");
            auto pro_id = GlobalUID::GetNew(("ECS:PID:" + std::to_string(pid)));

            std::lock_guard lock(e_mutex);

            auto root_index = reflectable_group_vector.size();

            auto group_ptr = TProvider::TryMakeGroup(buffer_group);

            reflectable_group_vector.push_back(group_ptr);

            auto& group = *group_ptr;

            group.cid = pid;
            group.id = ecs_id;
            group.GetName() += (" #" + std::to_string(pro_id));

            auto& provider_group = pid_provider_groups[pid];
            
            id = ecs_id;
            
            auto provider_index = buffer_group_get_buffer_element_count(buffer_group, provider_group.buffer_name);
            
            auto& prov_ptr_vec = pid_reflectable_vecs[pid];
            assert(provider_index == prov_ptr_vec.size());

            pid_id_index_maps[pid][id] = provider_index;
            prov_ptr_vec.push_back(group_ptr.get());

            out_index = group.index = provider_index;

            if (provider_group.buffer_name == buffer_name)
                Resize(provider_index + 1);
            else
                buffer_group_set_buffer_element_count(buffer_group, provider_group.buffer_name, provider_index + 1);

            auto buffer = buffer_group_get_buffer_data_ptr(buffer_group, provider_group.buffer_name);

            auto data_ptr = ((TData*)(buffer.get()));

            data_ptr[provider_index] = data;

            auto parent_group_ptr = FindParentGroupPtr(parent_id);

            if (parent_group_ptr) {
                group.parent_ptr = parent_group_ptr;
            }

            if constexpr (TProvider::GetIsComponent()) {
                auto& parent_entity_group = GetGroupByID<EntityProviderT, typename EntityProviderT::ReflectableGroup>(parent_id);

                auto sparse = buffer_group_get_buffer_data_ptr(buffer_group, provider_group.sparse_name);
                auto sparse_ptr = ((int*)(sparse.get()));

                sparse_ptr[parent_entity_group.index] = provider_index;
            }

            group.UpdateChildren();

            if (parent_id != -1) {
                auto& parent_group = GetGenericGroupByID(parent_id);
                if constexpr (std::is_same_v<TProvider, EntityProviderT>) {
                    auto& entity = GetEntity(id);
                    SetWhoParent(entity, &parent_group);
                }
                if constexpr (std::is_same_v<TProvider, CameraProviderT>) {
                    auto& camera = GetCamera(id);
                    SetWhoParent(camera, &parent_group);
                }
                if constexpr (std::is_same_v<TProvider, SceneProviderT>) {
                    auto& scene = GetScene(id);
                    SetWhoParent(scene, &parent_group);
                }
                if constexpr (std::is_same_v<TProvider, SubMeshProviderT>) {
                    auto& submesh = GetSubMesh(id);
                    SetWhoParent(submesh, &parent_group);
                }
            }

            if constexpr (std::is_same_v<TProvider, EntityProviderT> || std::is_same_v<TProvider, CameraProviderT>) {
                MarkDirty();
            }
        }

        static auto SetWhoParent(auto& who, auto new_parent_ptr) {
            who.parent_index = new_parent_ptr ? new_parent_ptr->index : -1;
            who.parent_cid = new_parent_ptr ? new_parent_ptr->cid : 0;
        }

        template <typename TData>
        size_t AddProvider(int parent_id, const TData& data, std::vector<std::shared_ptr<ReflectableGroup>>& reflectable_group_vector, int& out_index) {
            size_t id = 0;
            (AddProviderSingle<TData, TProviders>(parent_id, id, data, reflectable_group_vector, out_index), ...);
            if (!id)
                throw std::runtime_error("Unable to find Provider supporting TData");
            return id;
        }

        template <typename TEntity>
        size_t AddEntity(int parent_id, const TEntity& entity_data, const std::vector<int>& mesh_indexes) {
            auto parent_group_ptr = FindParentGroupPtr(parent_id);
            int out_index = -1;
            auto entity_id = AddProvider<TEntity>(parent_id, entity_data,
                parent_group_ptr ? parent_group_ptr->GetChildren() : reflectable_group_root_vector, out_index);
            auto& entity_group = GetGroupByID<EntityProviderT, typename EntityProviderT::ReflectableGroup>(entity_id);
            for (auto& mesh_index : mesh_indexes) {
                auto& mesh_group = GetGroupByIndex<MeshProviderT, typename MeshProviderT::ReflectableGroup>(mesh_index);
                int submesh_index = -1;
                SubMeshProviderT submesh_data{
                    .parent_index = out_index,
                    .mesh_index = mesh_index,
                    .material_index = mesh_group.material_index
                };
                auto submesh_id = AddProvider<SubMeshProviderT>(entity_id, submesh_data, entity_group.reflectable_children, submesh_index);
            }
            return entity_id;
        }

        template <typename TEntity>
        size_t AddEntity(const TEntity& entity_data, const std::vector<int>& mesh_indexes) {
            return AddEntity<TEntity>(-1, entity_data, mesh_indexes);
        }

        template <typename TScene>
        size_t AddScene(int parent_id, const TScene& scene_data) {
            auto parent_group_ptr = FindParentGroupPtr(parent_id);
            int out_index = -1;
            return AddProvider<TScene>(parent_id, scene_data,
                parent_group_ptr ? parent_group_ptr->GetChildren() : reflectable_group_root_vector, out_index);
        }

        template <typename TScene>
        size_t AddScene(const TScene& scene_data) {
            return AddScene<TScene>(-1, scene_data);
        }

        SceneProviderT& GetScene(size_t scene_id) {
            return GetProviderData<SceneProviderT>(scene_id);
        }

        template <typename TMaterial>
        size_t AddMaterial(const TMaterial& material_data, int& out_index) {
            return AddProvider<TMaterial>(-1, material_data, material_group_vector, out_index);
        }

        MaterialProviderT& GetMaterial(size_t material_id) {
            return GetProviderData<MaterialProviderT>(material_id);
        }

        void SetMaterialImage(size_t material_id, Image* image) {
            auto& material_group = GetGroupByID<MaterialProviderT, typename MaterialProviderT::ReflectableGroup>(material_id);
            material_group.image = image;
            
            auto frame_ds_pair = image_create_descriptor_set(image);
            material_group.frame_image_ds = frame_ds_pair.second;

            atlas_pack.addImage(material_group.image);
            if (buffer_initialized)
                UpdateAtlas();
        }

        size_t AddMesh(
            size_t parent_id,
            const std::vector<vec<float, 4>>& positions,
            const std::vector<vec<float, 2>>& uv2s,
            const std::vector<vec<float, 4>>& normals,
            int material_index,
            int& out_index
        ) {
            MeshProviderT mesh_data;
            auto position_index = buffer_group_get_buffer_element_count(buffer_group, VertexPositions_Str);
            auto uv2_index = buffer_group_get_buffer_element_count(buffer_group, VertexUV2s_Str);
            auto normal_index = buffer_group_get_buffer_element_count(buffer_group, VertexNormals_Str);

            if (!positions.empty()) {
                mesh_data.vertex_count = positions.size();
                mesh_data.position_offset = position_index;
            }
            if (!uv2s.empty())
                mesh_data.uv2_offset = uv2_index;
            if (!normals.empty())
                mesh_data.normal_offset = normal_index;

            auto parent_group_ptr = FindParentGroupPtr(parent_id);
            auto mesh_id = AddProvider<MeshProviderT>(parent_id, mesh_data,
                parent_group_ptr ? parent_group_ptr->GetChildren() : mesh_group_vector, out_index);
            
            if (mesh_data.position_offset != -1) {
                buffer_group_set_buffer_element_count(buffer_group, VertexPositions_Str, mesh_data.position_offset + positions.size());

                auto positions_sh_ptr = buffer_group_get_buffer_data_ptr(buffer_group, VertexPositions_Str);
                auto positions_ptr = (vec<float, 4>*)(positions_sh_ptr.get());

                memcpy((void*)&positions_ptr[mesh_data.position_offset], positions.data(), positions.size() * sizeof(vec<float, 4>));
            }

            if (mesh_data.uv2_offset != -1) {
                buffer_group_set_buffer_element_count(buffer_group, VertexUV2s_Str, mesh_data.uv2_offset + uv2s.size());

                auto uv2s_sh_ptr = buffer_group_get_buffer_data_ptr(buffer_group, VertexUV2s_Str);
                auto uv2s_ptr = (vec<float, 2>*)(uv2s_sh_ptr.get());

                memcpy((void*)&uv2s_ptr[mesh_data.uv2_offset], uv2s.data(), uv2s.size() * sizeof(vec<float, 2>));
            }

            if (mesh_data.normal_offset != -1) {
                buffer_group_set_buffer_element_count(buffer_group, VertexNormals_Str, mesh_data.normal_offset + normals.size());

                auto normals_sh_ptr = buffer_group_get_buffer_data_ptr(buffer_group, VertexNormals_Str);
                auto normals_ptr = (vec<float, 4>*)(normals_sh_ptr.get());

                memcpy((void*)&normals_ptr[mesh_data.normal_offset], normals.data(), normals.size() * sizeof(vec<float, 4>));
            }

            auto& mesh_group = GetGroupByID<MeshProviderT, typename MeshProviderT::ReflectableGroup>(mesh_id);
            mesh_group.material_index = material_index;

            return mesh_id;
        }

        size_t AddMesh(
            const std::vector<vec<float, 4>>& positions,
            const std::vector<vec<float, 2>>& uv2s,
            const std::vector<vec<float, 4>>& normals,
            int material_index,
            int& out_index
        ) {
            return AddMesh(-1, positions, uv2s, normals, material_index, out_index);
        }

        ReflectableGroup& GetGenericGroupByID(size_t id) {
            auto ptr = FindParentGroupPtr(id);
            if (ptr)
                return *ptr;
            throw std::runtime_error("Unable to find group with id");
        }

        template <typename TProvider, typename TReflectableGroup>
        TReflectableGroup& GetGroupByIndex(size_t index) {
            assert(index >= 0);
            constexpr auto pid = TProvider::GetPID();

            auto& reflectable_vec = pid_reflectable_vecs[pid];
            
            if (index >= reflectable_vec.size())
                throw std::runtime_error("Index out of range");

            auto group_ptr = reflectable_vec[index];
            auto t_group_ptr = dynamic_cast<TReflectableGroup*>(group_ptr);

            if (!t_group_ptr)
                throw std::runtime_error("group_ptr is not of type TReflectableGroup");

            return *t_group_ptr;
        }

        template <typename TProvider, typename TReflectableGroup>
        TReflectableGroup& GetGroupByID(size_t id) {
            constexpr auto pid = TProvider::GetPID();
            auto& ref_map = pid_id_index_maps[pid];
            auto it = ref_map.find(id);
            if (it == ref_map.end())
                throw std::runtime_error("Provider not found with id");
            auto group_index = it->second;
            return GetGroupByIndex<TProvider, TReflectableGroup>(group_index);
        }

        ReflectableGroup* FindParentGroupPtr(int parent_id) {
            if (parent_id == -1)
                return nullptr;
            for (auto& [pid, ref_map] : pid_id_index_maps) {
                auto it = ref_map.find(parent_id);
                if (it == ref_map.end())
                    continue;
                auto& ref_vec = pid_reflectable_vecs[pid];
                auto index = it->second;
                if (index >= 0 && index < ref_vec.size())
                    return ref_vec[index];
            }
            return nullptr;
        }

        template <typename TProvider>
        std::vector<ReflectableGroup*>::iterator GetProviderBegin() {
            constexpr auto pid = TProvider::GetPID();
            auto ref_vec_it = pid_reflectable_vecs.find(pid);
            if (ref_vec_it == pid_reflectable_vecs.end())
                throw std::runtime_error("Provider not found");
            return ref_vec_it->second.begin();
        }

        template <typename TProvider>
        std::vector<ReflectableGroup*>::iterator GetProviderEnd() {
            constexpr auto pid = TProvider::GetPID();
            auto ref_vec_it = pid_reflectable_vecs.find(pid);
            if (ref_vec_it == pid_reflectable_vecs.end())
                throw std::runtime_error("Provider not found");
            return ref_vec_it->second.end();
        }

        template <typename TProvider>
        TProvider& GetProviderData(size_t id) {
            constexpr auto pid = TProvider::GetPID();
            auto index = pid_id_index_maps[pid][id];
            auto provider_b_name = TProvider::GetStructName() + "s";
            auto p_buff = buffer_group_get_buffer_data_ptr(buffer_group, provider_b_name);
            auto p_ptr = ((TProvider*)p_buff.get());
            return p_ptr[index];
        }

        EntityProviderT& GetEntity(size_t entity_id) {
            return GetProviderData<EntityProviderT>(entity_id);
        }

        SubMeshProviderT& GetSubMesh(size_t submesh_id) {
            return GetProviderData<SubMeshProviderT>(submesh_id);
        }

        template <typename TCamera>
        size_t AddCamera(size_t parent_id, const TCamera& camera_data, TCamera::ProjectionType projectionType) {
            auto parent_group_ptr = FindParentGroupPtr(parent_id);
            int out_index = -1;
            auto camera_id = AddProvider<TCamera>(parent_id, camera_data,
                parent_group_ptr ? parent_group_ptr->GetChildren() : reflectable_group_root_vector, out_index);

            auto& camera = GetCamera(camera_id);
            auto& camera_group = GetGroupByID<TCamera, typename TCamera::ReflectableGroup>(camera_id);

            auto& width = *window_get_width_ref(window_ptr);
            auto& height = *window_get_height_ref(window_ptr);
            // Initialize camera matrices
            {
                switch(projectionType) {
                case TCamera::Perspective:
                    CameraInit(camera, {0, 0, 10}, {0, 0, 0}, {0, 1, 0}, 0.25f, 1000.f, width, height, radians(81.f));
                    break;
                case TCamera::Orthographic:
                    CameraInit(camera, {0, 0, 10}, {0, 0, 0}, {0, 1, 0}, 0.25f, 1000.f, vec<float, 4>(0, 0, width, height));
                    break;
                }
            }

            camera_group.NotifyNameChanged();
            camera_group.InitFramebuffer(main_shader, width, height);

            return camera_id;
        }

        template <typename TCamera>
        size_t AddCamera(const TCamera& camera_data, TCamera::ProjectionType projectionType) {
            return AddCamera<TCamera>(-1, camera_data, projectionType);
        }

        CameraProviderT& GetCamera(size_t camera_id) {
            return GetProviderData<CameraProviderT>(camera_id);
        }

        template <typename TComponent>
        size_t ConstructComponent(size_t entity_id, const TComponent& data) {
            auto& entity = GetEntity(entity_id);
            auto& entity_group = GetGroupByID<EntityProviderT, typename EntityProviderT::ReflectableGroup>(entity_id);
            constexpr auto component_type_id = TComponent::GetComponentID();
            int mask = 1 << (component_type_id - 1);
            if (entity.enabled_components & mask)
                throw std::runtime_error("Component already exists");
            int out_index = -1;
            auto component_id = AddProvider<TComponent>(entity_id, data, entity_group.component_groups, out_index);
            auto& component = GetProviderData<TComponent>(component_id);
            entity.enabled_components |= mask;
            entity_group.UpdateChildren();
            return component_id;
        }

        template <typename T, typename TProvider>
        void IsTThisProvider(bool& out_bool) {
            if (out_bool)
                return;
            out_bool = std::is_same_v<T, TProvider>;
        }

        template <typename T>
        bool IsTAProvider() {
            bool out_bool = false;
            (IsTThisProvider<T, TProviders>(out_bool), ...);
            return out_bool;
        }

        void SetProviderCount(const std::string& buffer_name, int count) {
            auto buffer_size = buffer_group_get_buffer_element_count(buffer_group, buffer_name);
            if (count > buffer_size) {
                buffer_group_set_buffer_element_count(buffer_group, buffer_name, count);
            }
        }

        template <typename T>
        T* GetProviderData(const std::string& buffer_name) {
            auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, buffer_name);
            return (T*)buffer_ptr.get();
        }

        BufferGroup* CreateBufferGroup() {
            auto buffer_group_ptr = buffer_group_create("ECS:Group");
            for (auto& [provider_id, provider_group] : pid_provider_groups) {
                restricted_keys.push_back(provider_group.buffer_name);
                if (provider_group.is_component)
                    restricted_keys.push_back(provider_group.sparse_name);
            }
            buffer_group_restrict_to_keys(buffer_group_ptr, restricted_keys);
            return buffer_group_ptr;
        }

        Shader* GenerateMainShader() {
            auto shader_ptr = shader_create();


            shader_add_buffer_group(shader_ptr, buffer_group);

            shader_add_module(shader_ptr, ShaderModuleType::Vertex, GenerateMainVertexShaderCode());
            shader_add_module(shader_ptr, ShaderModuleType::Fragment, GenerateMainFragmentShaderCode());

            return shader_ptr;
        }

        Shader* GenerateModelComputeShader() {
            auto shader_ptr = shader_create();

            for (auto& [provider_id, provider_group] : pid_provider_groups)
                shader_set_define(shader_ptr, "CID_" + provider_group.name, std::to_string(provider_id));

            shader_add_buffer_group(shader_ptr, buffer_group);

            shader_add_module(shader_ptr, ShaderModuleType::Compute, GenerateModelComputeShaderCode());

            window_register_compute_dispatch(window_ptr, 0.0f, shader_ptr, [&]() {
                return buffer_group_get_buffer_element_count(buffer_group, buffer_name);
            });

            return shader_ptr;
        }

        Shader* GenerateCameraMatComputeShader() {
            auto shader_ptr = shader_create();

            for (auto& [provider_id, provider_group] : pid_provider_groups)
                shader_set_define(shader_ptr, "CID_" + provider_group.name, std::to_string(provider_id));

            shader_add_buffer_group(shader_ptr, buffer_group);

            shader_add_module(shader_ptr, ShaderModuleType::Compute, GenerateCameraMatComputeShaderCode());

            window_register_compute_dispatch(window_ptr, -10.0f, shader_ptr, [&]() {
                return buffer_group_get_buffer_element_count(buffer_group, Cameras_Str);
            });

            return shader_ptr;
        }

        void EnableDrawInWindow(WINDOW* window_ptr) {
            window_add_drawn_buffer_group(window_ptr, &draw_mg, buffer_group);

            window_register_free_callback(window_ptr, 100.f, [&]() mutable {
                // for (auto& [scene_id, scene_group] : id_scene_groups)
                //     scene_group.indexed_reflectable_groups.clear();
                // id_scene_groups.clear();
            });
        }

        std::string GenerateShaderHeader(ShaderModuleType moduleType) {          
            std::string shader_header;

            auto binding_index = 0;

            shader_header += "\nlayout(binding = " + std::to_string(binding_index++) + ") uniform sampler2D Atlas;\n";

            // Setup Structs
            for (auto& [priority, provider_ids] : prioritized_provider_ids) {
                for (auto& provider_id : provider_ids) {
                    auto& provider_group = pid_provider_groups[provider_id];
                    shader_header += provider_group.glsl_struct;
                }
            }

            // Mesh Buffers
            shader_header += R"(
layout(std430, binding = )" + std::to_string(binding_index++) + R"() buffer VertexPositionsBuffer {
    vec4 data[];
} VertexPositions;
)";
            shader_header += R"(
layout(std430, binding = )" + std::to_string(binding_index++) + R"() buffer VertexUV2sBuffer {
    vec2 data[];
} VertexUV2s;
)";
            shader_header += R"(
layout(std430, binding = )" + std::to_string(binding_index++) + R"() buffer VertexNormalsBuffer {
    vec4 data[];
} VertexNormals;
)";

            // Setup Buffers
            for (auto& [priority, provider_ids] : prioritized_provider_ids) {
                for (auto& provider_id : provider_ids) {
                    auto& provider_group = pid_provider_groups[provider_id];
                    shader_header += R"(
layout(std430, binding = )" + std::to_string(binding_index++) + ") buffer " + provider_group.struct_name + R"(Buffer {
    )" + provider_group.struct_name + R"( data[];
} )" + provider_group.struct_name + R"(s;

)";
                    if (provider_group.is_component) {
                        shader_header += R"(
layout(std430, binding = )" + std::to_string(binding_index++) + ") buffer " + provider_group.struct_name + R"(SparseBuffer {
    int data[];
} Sparse)" + provider_group.struct_name + R"(s;

)";
                    }
                    shader_header += provider_group.struct_name + " Get" + provider_group.struct_name + R"(Data(int t_provider_index) {
    return )" + provider_group.struct_name + R"(s.data[t_provider_index];
}
)";
                    shader_header += provider_group.glsl_methods[moduleType];
                }
            }

            // Component Methods
            shader_header += R"(
bool HasComponentWithType(in Entity entity, int entity_index, int type, out int t_component_index) {
    int mask = 1 << (type - 1);
    bool has_component = (entity.enabled_components & mask) != 0;
    switch (type) {
)";
            for (auto& [priority, provider_ids] : prioritized_provider_ids) {
                for (auto& provider_id : provider_ids) {
                    auto& provider_group = pid_provider_groups[provider_id];
                    if (provider_group.is_component) {
                        shader_header += R"(
    case )" + std::to_string(provider_group.component_id) + R"(:
        t_component_index = Sparse)" + provider_group.struct_name + R"(s.data[entity_index];
        break;
)";
                    }
                }
            }

            shader_header += R"(
    }
    return true;
}
)";
            return shader_header;
        }

        std::string GenerateShaderMain(ShaderModuleType moduleType) {
            std::string shader_main; 

            // Main Get Components
            for (auto& [id, entry] : registered_component_map) {
                auto struct_name_lower = to_lower(entry.struct_name);
                shader_main += "    " + entry.struct_name + " " + struct_name_lower + ";\n";
                shader_main += R"(    if (HasComponentWithType(entity, entity_index, )" + std::to_string(entry.type_id) + R"(, t_component_index))
        )" + struct_name_lower + " = Get" + entry.struct_name + "Data(t_component_index);\n";
            }

            // Priority Mains
            auto& module_glsl_mains = priority_glsl_mains[moduleType];
            for (auto& [priority, string_vec] : module_glsl_mains) {
                for (auto& string : string_vec) {
                    shader_main += string;
                }
            }

            return shader_main;
        }

        std::string GenerateShaderLayout(ShaderModuleType moduleType, int& in_location, int& out_location) {
            std::string shader_layouts;
            for (auto& [priority, provider_ids] : prioritized_provider_ids) {
                for (auto& provider_id : provider_ids) {
                    auto& provider_group = pid_provider_groups[provider_id];
                    auto& glsl_layouts = provider_group.glsl_layouts;
                    auto& layouts_vec = glsl_layouts[moduleType];
                    for (auto layout_str : layouts_vec) {
                        static std::string IN_STR = "@IN@";
                        static std::string OUT_STR = "@OUT@";
                        static auto replace_str = [](const auto& STR, auto& layout_str, auto& binding) {
                            auto pos = layout_str.find(STR);
                            if (pos != std::string::npos) {
                                auto layout_str_it = layout_str.begin();
                                layout_str.erase(layout_str_it + pos, layout_str_it + pos + STR.size());
                                auto binding_str = std::to_string(binding++);
                                layout_str_it = layout_str.begin();
                                layout_str.insert(layout_str_it + pos, binding_str.begin(), binding_str.end());
                            }
                        };
                        replace_str(IN_STR, layout_str, in_location);
                        replace_str(OUT_STR, layout_str, out_location);
                        shader_layouts += ("\n" + layout_str + "\n");
                    }
                }
            }
            return shader_layouts;
        }

        std::string GenerateMainVertexShaderCode() {
            std::string shader_string = R"(
#version 450
layout(location = 0) out int outID;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec4 outPosition;
layout(location = 3) out vec3 outNormal;
layout(location = 4) out vec3 outViewPosition;
layout(location = 5) out vec2 outUV2;
)";
            int out_location = 6;
            int in_location = 0;
            shader_string += GenerateShaderLayout(ShaderModuleType::Vertex, in_location, out_location);

            shader_string += R"(
layout(push_constant) uniform PushConstants {
    int camera_index;
} pc;
)";
            shader_string += GenerateShaderHeader(ShaderModuleType::Vertex);

            // Main
            shader_string += R"(
void main() {
    int submesh_index = outID = gl_InstanceIndex;
    SubMesh submesh = GetSubMeshData(submesh_index);
    Mesh mesh = GetMeshData(submesh.mesh_index);
    Entity entity = GetEntityData(submesh.parent_index);
    vec4 final_color;
    int t_component_index = -1;
)";

            shader_string += GenerateShaderMain(ShaderModuleType::Vertex);

            // Main Output
            shader_string += R"(
    vec3 normal_vs = normalize(mat3(transpose(inverse(entity.model))) * mesh_normal);
    gl_Position = clip_position;
    outColor = final_color;
    outPosition = entity.model * vertex_position;
    outViewPosition = view_position.xyz;
    outNormal = normal_vs;
    outUV2 = mesh_uv2;
}
)";

            return shader_string;
        }

        std::string GenerateMainFragmentShaderCode() {
            std::string shader_string = R"(
#version 450
layout(location = 0) flat in int inID;
layout(location = 1) in vec4 inColor;
layout(location = 2) in vec4 inPosition;
layout(location = 3) in vec3 inNormal;
layout(location = 4) in vec3 inViewPosition;
layout(location = 5) in vec2 inUV2;

layout(location = 0) out vec4 FragColor;
)";
            int in_location = 6;
            int out_location = 1;
            shader_string += GenerateShaderLayout(ShaderModuleType::Fragment, in_location, out_location);

            shader_string += R"(

layout(push_constant) uniform PushConstants {
    int camera_index;
} pc;
)";
            shader_string += GenerateShaderHeader(ShaderModuleType::Fragment);

            shader_string += R"(
void main() {
    int submesh_index = inID;
    SubMesh submesh = GetSubMeshData(submesh_index);
    Entity entity = GetEntityData(submesh.parent_index);
    int t_component_index = -1;
    vec4 current_color = inColor;
)";
    
            shader_string += GenerateShaderMain(ShaderModuleType::Fragment);

            shader_string += R"(
    FragColor = current_color;
}
)";
            return shader_string;
        }

        std::string GenerateModelComputeShaderCode() {
            std::string shader_string = R"(
#version 450
layout(local_size_x = 1) in;
)";
            shader_string += GenerateShaderHeader(ShaderModuleType::Compute);

            shader_string += R"(
void main() {
    int entity_index = int(gl_GlobalInvocationID.x);
    if (entity_index >= Entitys.data.length()) return;
    
    int current_index = entity_index;
    int current_cid = CID_Entity;

    mat4 final_model = mat4(1.0);

    while (current_index != -1) {
        mat4 current_model = mat4(1.0);
        switch (current_cid) {
        case CID_Entity:
            GetEntityModel(current_index, current_model, current_index, current_cid);
            break;
        case CID_Camera:
            GetCameraModel(current_index, current_model, current_index, current_cid);
            break;
        case CID_Scene:
            GetSceneModel(current_index, current_model, current_index, current_cid);
            break;
        }
        final_model = current_model * final_model;
    }

    Entitys.data[entity_index].model = final_model;
}
)";
            return shader_string;
        }

        std::string GenerateCameraMatComputeShaderCode() {
            std::string shader_string = R"(
#version 450
layout(local_size_x = 1) in;
)";
            shader_string += GenerateShaderHeader(ShaderModuleType::Compute);

            shader_string += R"(

void main() {
    int camera_index = int(gl_GlobalInvocationID.x);
    if (camera_index >= Cameras.data.length()) return;
    
    int current_index = camera_index;
    int current_cid = CID_Camera;

    int scene_count = 0;

    mat4 final_model = mat4(1.0);

    while (current_index != -1) {
        mat4 current_model = mat4(1.0);
        switch (current_cid) {
        case CID_Entity:
            GetEntityModel(current_index, current_model, current_index, current_cid);
            break;
        case CID_Camera:
            GetCameraModel(current_index, current_model, current_index, current_cid);
            break;
        case CID_Scene:
            GetSceneModel(current_index, current_model, current_index, current_cid);
            break;
        }
        final_model = current_model * final_model;
    }

    Cameras.data[camera_index].view = inverse(final_model);
}
)";

            return shader_string;
        }

        bool ResizeFramebuffer(size_t camera_id, uint32_t width, uint32_t height) {
            auto& camera_group = GetGroupByID<CameraProviderT, typename CameraProviderT::ReflectableGroup>(camera_id);
            auto fb_resized = framebuffer_resize(camera_group.framebuffer, width, height);
            if (!fb_resized)
                return false;
            if (framebuffer_changed(camera_group.framebuffer)) {
                camera_group.fb_color_image = framebuffer_get_image(camera_group.framebuffer, AttachmentType::Color, true);
                camera_group.fb_depth_image = framebuffer_get_image(camera_group.framebuffer, AttachmentType::Depth, true);
                SetCameraAspect(camera_id, width, height);
                auto frame_ds_pair = image_create_descriptor_set(camera_group.fb_color_image);
                camera_group.frame_image_ds = frame_ds_pair.second;
            }
            return true;;
        }

        bool FramebufferChanged(size_t camera_id) {
            auto& camera_group = GetGroupByID<CameraProviderT, typename CameraProviderT::ReflectableGroup>(camera_id);
            return framebuffer_changed(camera_group.framebuffer);
        }

        bool SetCameraAspect(size_t camera_id, float width, float height) {
            if (!width || !height)
                return false;
            auto& camera = GetCamera(camera_id);
            camera.orthoWidth = width;
            camera.orthoHeight = height;
            switch (typename CameraProviderT::ProjectionType(camera.type)) {
            case CameraProviderT::Perspective:
                camera.aspect = camera.orthoWidth / camera.orthoHeight;
                break;
            default: break;
            }
            CameraInit(camera);
            return true;
        }

        void MarkDirty() {
            draw_mg.MarkDirty();
        }
    };
}