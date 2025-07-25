#pragma once
#include "GlobalUID.hpp"
#include <unordered_map>
#include <memory>
#include "BufferGroup.hpp"
#include "Framebuffer.hpp"
#include "DrawListManager.hpp"
#include "Window.hpp"
#include "Shader.hpp"
#include "Camera.hpp"
#include "Util.hpp"
#include "State.hpp"
#include "ECS/Provider.hpp"
#include "ECS/Light.hpp"
#include "ECS/Component.hpp"
#include "ECS/Entity.hpp"
#include "ECS/Scene.hpp"
#include <string>
#include <vector>
#include <functional>
#include <map>
#include <set>
#include <iostreams/Serial.hpp>
#include <fstream>
#include <mutex>

#define ID_ENTITY_MIN 0
#define ID_CAMERA_MIN 357913941
#define ID_LIGHT_MIN 715827882

namespace dz {

    inline static const std::string cameras_buffer_name = "Cameras";
    inline static const std::string lights_buffer_name = "Lights";
    
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

        std::recursive_mutex e_mutex;

        std::string buffer_name;

        WINDOW* window_ptr = 0;
        std::filesystem::path save_path;
        bool loaded_from_io = false;

        std::vector<std::shared_ptr<ReflectableGroup>> reflectable_group_root_vector;
        std::unordered_map<size_t, std::map<size_t, ReflectableGroup*>> provider_id_reflectable_maps;
        std::unordered_map<size_t, std::vector<size_t>> provider_index_vectors;

        std::map<size_t, ProviderReflectableGroup> id_provider_groups;
        std::map<float, std::vector<size_t>> prioritized_provider_ids;

        std::unordered_map<ShaderModuleType, std::map<float, std::vector<std::string>>> priority_glsl_mains;

        std::vector<std::string> restricted_keys{"Components"};
        std::map<int, RegisteredComponentEntry> registered_component_map;
        bool components_registered = false;

        using DrawProviderT = typename FirstMatchingOrDefault<IsDrawProvider, TProviders...>::type;
        using CameraProviderT = typename FirstMatchingOrDefault<IsCameraProvider, TProviders...>::type;
        using SceneProviderT = typename FirstMatchingOrDefault<IsSceneProvider, TProviders...>::type;
        DrawListManager<DrawProviderT> draw_mg;

        BufferGroup* buffer_group = 0;
        bool buffer_initialized = false;
        int buffer_size = 0;
        
        Shader* main_shader = 0;
        Shader* model_compute_shader = 0;

        int constructed_component_count = 0;

        auto GenerateEntitysDrawFunction() {
            return [&](auto buffer_group, auto& entity) -> DrawTuples {
                return {
                    {main_shader, entity.GetVertexCount(buffer_group, entity)}
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
                        auto e_group_ptr = dynamic_cast<DrawProviderT::ReflectableGroup*>(ref_group_ptr);
                        if (e_group_ptr) {
                            visible.push_back(e_group_ptr->index);
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

        template <typename TqProvider>
        void IsDrawProviderName(std::string& out_string) {
            if (!out_string.empty())
                return;
            if (TqProvider::GetIsDrawProvider()) {
                out_string = (TqProvider::GetStructName() + "s");
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
                "Cameras", GenerateCamerasDrawFunction(),
                GenerateCameraVisibilityFunction()
            )
        {
            RegisterProviders();
            buffer_group = CreateBufferGroup();
            assert(buffer_group);
            main_shader = GenerateMainShader();
            model_compute_shader = GenerateModelComputeShader();
            EnableDrawInWindow(window_ptr);
            restore(serial);
            loaded_from_io = true;
        }

        ECS(WINDOW* initial_window_ptr):
            buffer_name(FindBufferNameFromProviders()),
            window_ptr(initial_window_ptr),
            draw_mg(
                buffer_name, GenerateEntitysDrawFunction(),
                "Cameras", GenerateCamerasDrawFunction(),
                GenerateCameraVisibilityFunction()
            )
        {
            RegisterProviders();
            buffer_group = CreateBufferGroup();
            assert(buffer_group);
            main_shader = GenerateMainShader();
            model_compute_shader = GenerateModelComputeShader();
            EnableDrawInWindow(window_ptr);
        }

        void MarkReady() {
            if (!buffer_initialized) {
                buffer_group_initialize(buffer_group);
                buffer_initialized = true;
            }
        }

        bool backup(Serial& serial) override {
            std::lock_guard lock(e_mutex);
            if (!serial.canWrite())
                return false;
            // if (!SerializeGroups(serial, id_entity_groups))
            //     return false;
            // if (!SerializeGroups(serial, indexed_camera_groups))
            //     return false;
            // if (!SerializeGroups(serial, indexed_light_groups))
            //     return false;
            // if (!SerializeGroups(serial, id_scene_groups))
            //     return false;
            // if (!SerializeBuffers(serial))
            //     return false;
            return true;
        }

        bool restore(Serial& serial) override {
            std::lock_guard lock(e_mutex);
            if (!serial.canRead() || serial.getReadLength() == 0)
                return false;
            // if (!DeserializeGroups(serial, id_entity_groups))
            //     return false;
            // if (!DeserializeGroups(serial, indexed_camera_groups))
            //     return false;
            // if (!DeserializeGroups(serial, indexed_light_groups))
            //     return false;
            // if (!DeserializeGroups(serial, id_scene_groups))
            //     return false;
            // if (!DeserializeBuffers(serial))
            //     return false;
            // CheckDisabled();
            // UpdateGroupsChildren(id_entity_groups);
            // UpdateGroupsChildren(indexed_camera_groups);
            // UpdateGroupsChildren(indexed_light_groups);
            // UpdateGroupsChildren(id_scene_groups);
            return true;
        }

        // void CheckDisabled() {
        //     CheckLightDisabled();
        // }

        // void CheckLightDisabled() {
        //     auto r_it = std::find(restricted_keys.begin(), restricted_keys.end(), "Lights");
        //     // if (r_it == restricted_keys.end())
        //     //     for (auto& [scene_id, scene_group] : id_scene_groups)
        //     //         if (scene_group.indexed_reflectable_groups.size() > 0)
        //     //             scene_group.indexed_reflectable_groups.clear();
        // }

        // template <typename KeyT, typename GroupT>
        // bool SerializeGroups(Serial& ioSerial, const std::map<KeyT, GroupT>& groups) {
        //     ioSerial << groups.size();
        //     for (auto& [id, group] : groups) {
        //         ioSerial << id;
        //         if (!group.serialize(*this, ioSerial))
        //             return false;
        //     }
        //     return true;
        // }

        // template <typename KeyT, typename GroupT>
        // bool DeserializeGroups(Serial& ioSerial, std::map<KeyT, GroupT>& groups) {
        //     auto size = groups.size();
        //     ioSerial >> size;
        //     for (size_t count = 1; count <= size; ++count) {
        //         KeyT id;
        //         ioSerial >> id;
        //         auto& group = groups[id];
        //         if (!group.deserialize(*this, ioSerial))
        //             return false;
        //         if constexpr (std::is_same_v<GroupT, CameraReflectableGroup>) {
        //             auto& width = *window_get_width_ref(window_ptr);
        //             auto& height = *window_get_height_ref(window_ptr);
        //             group.InitFramebuffer(*this, width, height);
        //         }
        //     }
        //     return true;
        // }

        // template <typename KeyT, typename GroupT>
        // void UpdateGroupsChildren(std::map<KeyT, GroupT>& groups) {
        //     for (auto& [id, group] : groups) {
        //         group.UpdateChildren(*this);
        //         if constexpr (std::is_same_v<GroupT, EntityReflectableGroup>)
        //             group.UpdateReflectables();
        //     }
        // }

        // bool DeserializeBuffers(Serial& ioSerial) {
        //     if (!buffer_group)
        //         return false;
        //     auto keys_size = restricted_keys.size();
        //     ioSerial >> keys_size;
        //     for (size_t key_count = 1; key_count <= keys_size; ++key_count) {
        //         std::string restricted_key;
        //         ioSerial >> restricted_key;
        //         uint32_t element_count, element_size;
        //         ioSerial >> element_count >> element_size;
        //         auto buffer_size = element_count * element_size;
        //         auto r_key_it = std::find(restricted_keys.begin(), restricted_keys.end(), restricted_key);
        //         if (r_key_it == restricted_keys.end()) {
        //             auto bytes = (char*)malloc(buffer_size);
        //             ioSerial.readBytes((char*)bytes, buffer_size);
        //             free(bytes);
        //             continue;
        //         }
        //         buffer_group_set_buffer_element_count(buffer_group, restricted_key, element_count);
        //         auto actual_element_size = buffer_group_get_buffer_element_size(buffer_group, restricted_key);
        //         if (actual_element_size != element_size)
        //             throw std::runtime_error("Incompatible buffer element sizes");
        //         auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, restricted_key);
        //         ioSerial.readBytes((char*)buffer_ptr.get(), buffer_size);
        //     }
        //     return true;
        // }

        // bool SerializeBuffers(Serial& ioSerial) {
        //     if (!buffer_group)
        //         return false;
        //     auto keys_size = restricted_keys.size();
        //     ioSerial << keys_size;
        //     for (auto& restricted_key : restricted_keys) {
        //         ioSerial << restricted_key;
        //         auto element_count = buffer_group_get_buffer_element_count(buffer_group, restricted_key);
        //         auto element_size = buffer_group_get_buffer_element_size(buffer_group, restricted_key);
        //         ioSerial << element_count << element_size;
        //         auto buffer_ptr = buffer_group_get_buffer_data_ptr(buffer_group, restricted_key);
        //         ioSerial.writeBytes((const char*)buffer_ptr.get(), element_count * element_size);
        //     }
        //     return true;
        // }

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
            auto& priority_glsl_main = TProvider::GetGLSLMain();
            constexpr auto pid = TProvider::GetPID();
            prioritized_provider_ids[priority].push_back(pid);
            auto provider_index = id_provider_groups.size();
            auto& provider_group = id_provider_groups[pid];
            provider_group.index = provider_index;
            provider_group.id = pid;
            provider_group.name = name;
            provider_group.struct_name = struct_name;
            provider_group.buffer_name = (struct_name + "s");
            provider_group.glsl_struct = glsl_struct;
            provider_group.glsl_methods = glsl_methods;
            provider_group.is_component = TProvider::GetIsComponent();
            for (auto& [main_priority, main_string, module_type] : priority_glsl_main) {
                priority_glsl_mains[module_type][main_priority].push_back(main_string);
            }
            if constexpr (TProvider::GetIsComponent()) {
                provider_group.sparse_name = ("Sparse" + struct_name + "s");
                provider_group.component_id = int(TProvider::GetComponentID());
                RegisterComponent<TProvider>();
            }
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

        bool RegisterComponents(const std::function<bool(ECS&)>& register_all_components_fn) {
            std::lock_guard lock(e_mutex);
            if (register_all_components_fn) {
                return register_all_components_fn(*this);
            }
            return false;
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
        void AddProviderSingle(int parent_id, size_t& id, const TData& data, std::vector<std::shared_ptr<ReflectableGroup>>& reflectable_group_vector) {
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
            group.id = ecs_id;
            group.GetName() += (" #" + std::to_string(pro_id));
            auto& provider_group = id_provider_groups[pid];
            provider_id_reflectable_maps[pid][ecs_id] = group_ptr.get();
            id = ecs_id;
            provider_index_vectors[pid].push_back(id);
            auto provider_index = buffer_group_get_buffer_element_count(buffer_group, provider_group.buffer_name);
            group.index = provider_index;
            if (provider_group.buffer_name == buffer_name)
                Resize(provider_index + 1);
            else
                buffer_group_set_buffer_element_count(buffer_group, provider_group.buffer_name, provider_index + 1);
            auto buffer = buffer_group_get_buffer_data_ptr(buffer_group, provider_group.buffer_name);
            auto data_ptr = ((TData*)(buffer.get()));
            data_ptr[provider_index] = data;
            if constexpr (std::is_same_v<TData, DrawProviderT>) {
                data_ptr[provider_index].id = ecs_id;
            }
            auto parent_group_ptr = FindParentGroupPtr(parent_id);
            if (parent_group_ptr) {
                group.parent_ptr = parent_group_ptr;
            }
            if constexpr (TProvider::GetIsComponent()) {
                auto& parent_entity_group = GetGroup<DrawProviderT, typename DrawProviderT::ReflectableGroup>(parent_id);

                auto sparse = buffer_group_get_buffer_data_ptr(buffer_group, provider_group.sparse_name);
                auto sparse_ptr = ((int*)(sparse.get()));

                sparse_ptr[parent_entity_group.index] = provider_index;
            }
            group.UpdateChildren();
            if constexpr (std::is_same_v<TProvider, DrawProviderT> || std::is_same_v<TProvider, CameraProviderT>) {
                draw_mg.SetDirty();
            }
        }

        template <typename TData>
        size_t AddProvider(int parent_id, const TData& data, std::vector<std::shared_ptr<ReflectableGroup>>& reflectable_group_vector) {
            size_t id = 0;
            (AddProviderSingle<TData, TProviders>(parent_id, id, data, reflectable_group_vector), ...);
            if (!id)
                throw std::runtime_error("Unable to find Provider supporting TData");
            return id;
        }

        template <typename TEntity>
        size_t AddEntity(int parent_id, const TEntity& entity_data) {
            auto parent_group_ptr = FindParentGroupPtr(parent_id);
            return AddProvider<TEntity>(parent_id, entity_data,
                parent_group_ptr ? parent_group_ptr->GetChildren() : reflectable_group_root_vector);
        }

        template <typename TEntity>
        size_t AddEntity(const TEntity& entity_data) {
            return AddEntity<TEntity>(-1, entity_data);
        }

        template <typename TScene>
        size_t AddScene(int parent_id, const TScene& entity_data) {
            auto parent_group_ptr = FindParentGroupPtr(parent_id);
            return AddProvider<TScene>(parent_id, entity_data,
                parent_group_ptr ? parent_group_ptr->GetChildren() : reflectable_group_root_vector);
        }

        template <typename TScene>
        size_t AddScene(const TScene& entity_data) {
            return AddScene<TScene>(-1, entity_data);
        }

        template <typename TProvider, typename TReflectableGroup>
        TReflectableGroup& GetGroupByIndex(size_t index) {
            assert(index >= 0);
            constexpr auto pid = TProvider::GetPID();
            auto& id_vec = provider_index_vectors[pid];
            if (index >= id_vec.size())
                throw std::runtime_error("Index out of range");
            auto id = id_vec[index];
            assert(id);
            return GetGroup<TProvider, TReflectableGroup>(id);
        }

        template <typename TProvider, typename TReflectableGroup>
        TReflectableGroup& GetGroup(size_t id) {
            constexpr auto pid = TProvider::GetPID();
            auto& ref_map = provider_id_reflectable_maps[pid];
            auto it = ref_map.find(id);
            if (it == ref_map.end())
                throw std::runtime_error("Provider not found with id");
            auto group_ptr = it->second;
            auto cast_ptr = dynamic_cast<TReflectableGroup*>(group_ptr);
            if (!cast_ptr)
                throw std::runtime_error("Group is not of type TReflectableGroup");
            return *cast_ptr;
        }

        ReflectableGroup* FindParentGroupPtr(int parent_id) {
            if (parent_id == -1)
                return nullptr;
            for (auto& [pid, ref_map] : provider_id_reflectable_maps) {
                auto it = ref_map.find(parent_id);
                if (it == ref_map.end())
                    continue;
                return it->second;
            }
            return nullptr;
        }

        template <typename TProvider>
        std::map<size_t, ReflectableGroup*>::iterator GetProviderBegin() {
            constexpr auto pid = TProvider::GetPID();
            auto ref_map_it = provider_id_reflectable_maps.find(pid);
            if (ref_map_it == provider_id_reflectable_maps.end())
                throw std::runtime_error("Provider not found");
            return ref_map_it->second.begin();
        }

        template <typename TProvider>
        std::map<size_t, ReflectableGroup*>::iterator GetProviderEnd() {
            constexpr auto pid = TProvider::GetPID();
            auto ref_map_it = provider_id_reflectable_maps.find(pid);
            if (ref_map_it == provider_id_reflectable_maps.end())
                throw std::runtime_error("Provider not found");
            return ref_map_it->second.end();
        }

        template <typename TProvider>
        TProvider& GetProviderData(size_t id) {
            constexpr auto pid = TProvider::GetPID();
            auto& ref_map = provider_id_reflectable_maps[pid];
            auto it = ref_map.find(id);
            if (it == ref_map.end())
                throw std::runtime_error("Provider not found with id");
            auto index = it->second->index;
            auto provider_b_name = TProvider::GetStructName() + "s";
            auto p_buff = buffer_group_get_buffer_data_ptr(buffer_group, provider_b_name);
            auto p_ptr = ((TProvider*)p_buff.get());
            return p_ptr[index];
        }

        DrawProviderT& GetEntity(size_t entity_id) {
            return GetProviderData<DrawProviderT>(entity_id);
        }

        template <typename TCamera>
        size_t AddCamera(size_t parent_id, const TCamera& camera_data, TCamera::ProjectionType projectionType) {
            auto parent_group_ptr = FindParentGroupPtr(parent_id);
            auto camera_id = AddProvider<TCamera>(parent_id, camera_data,
                parent_group_ptr ? parent_group_ptr->GetChildren() : reflectable_group_root_vector);

            auto& camera = GetCamera(camera_id);
            auto& camera_group = GetGroup<TCamera, typename TCamera::ReflectableGroup>(camera_id);

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
            auto& entity_group = GetGroup<DrawProviderT, typename DrawProviderT::ReflectableGroup>(entity_id);
            constexpr auto component_type_id = TComponent::GetComponentID();
            int mask = 1 << (component_type_id - 1);
            if (entity.enabled_components & mask)
                throw std::runtime_error("Component already exists");
            auto component_id = AddProvider<TComponent>(entity_id, data, entity_group.component_groups);
            auto& component = GetProviderData<TComponent>(component_id);
            entity.enabled_components |= mask;
            entity_group.UpdateChildren();
            return component_id;
        }

        template <typename T, typename TqProvider>
        void IsTThisProvider(bool& out_bool) {
            if (out_bool)
                return;
            out_bool = std::is_same_v<T, TqProvider>;
        }

        template <typename T>
        bool IsTAProvider() {
            bool out_bool = false;
            (IsTThisProvider<T, TProviders>(out_bool), ...);
            return out_bool;
        }

        template <typename T>
        T* GetProviderValByID(int provider_val_id) {
            if (!IsTAProvider<T>())
                throw std::runtime_error("T is not a Provider");
            
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
            for (auto& [provider_id, provider_group] : id_provider_groups) {
                restricted_keys.push_back(provider_group.buffer_name);
                if (provider_group.is_component)
                    restricted_keys.push_back(provider_group.sparse_name);
            }
            buffer_group_restrict_to_keys(buffer_group_ptr, restricted_keys);
            return buffer_group_ptr;
        }

        Shader* GenerateMainShader() {
            auto shader_ptr = shader_create();

            shader_set_define(shader_ptr, "ECS_MAX_COMPONENTS", std::to_string(ECS_MAX_COMPONENTS));

            shader_add_buffer_group(shader_ptr, buffer_group);

            shader_add_module(shader_ptr, ShaderModuleType::Vertex, GenerateMainVertexShaderCode());
            shader_add_module(shader_ptr, ShaderModuleType::Fragment, GenerateMainFragmentShaderCode());

            return shader_ptr;
        }

        Shader* GenerateModelComputeShader() {
            auto shader_ptr = shader_create();

            shader_set_define(shader_ptr, "ECS_MAX_COMPONENTS", std::to_string(ECS_MAX_COMPONENTS));

            shader_add_buffer_group(shader_ptr, buffer_group);

            shader_add_module(shader_ptr, ShaderModuleType::Compute, GenerateModelComputeShaderCode());

            window_register_compute_dispatch(window_ptr, 0.0f, shader_ptr, [&]() {
                return buffer_group_get_buffer_element_count(buffer_group, buffer_name);
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

            // Setup Structs
            for (auto& [priority, provider_ids] : prioritized_provider_ids) {
                for (auto& provider_id : provider_ids) {
                    auto& provider_group = id_provider_groups[provider_id];
                    shader_header += provider_group.glsl_struct;
                }
            }

            // Setup Buffers
            for (auto& [priority, provider_ids] : prioritized_provider_ids) {
                for (auto& provider_id : provider_ids) {
                    auto& provider_group = id_provider_groups[provider_id];
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
                    auto& provider_group = id_provider_groups[provider_id];
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

            // TComponent
//             shader_header += R"(
// layout(std430, binding = )" + std::to_string(binding_index++) + R"() buffer ComponentBuffer {
//     Component data[];
// } Components;
// )";
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

        std::string GenerateMainVertexShaderCode() {
            std::string shader_string = R"(
#version 450
layout(location = 0) out int outID;
layout(location = 1) out vec4 outColor;
layout(location = 2) out vec4 outPosition;
layout(location = 3) out vec3 outNormal;
layout(location = 4) out vec3 outViewPosition;

layout(push_constant) uniform PushConstants {
    int camera_index;
} pc;
)";
            shader_string += GenerateShaderHeader(ShaderModuleType::Vertex);

            // Main
            shader_string += R"(
void main() {
    int entity_index = outID = gl_InstanceIndex;
    Entity entity = GetEntityData(entity_index);
    vec4 final_color;
    int t_component_index = -1;
)";

            shader_string += GenerateShaderMain(ShaderModuleType::Vertex);

            // Main Output
            shader_string += R"(
    vec3 normal_vs = normalize(mat3(transpose(inverse(entity.model))) * shape_normal);
    gl_Position = clip_position;
    outColor = final_color;
    outPosition = entity.model * vertex_position;
    outViewPosition = view_position.xyz;
    outNormal = normal_vs;
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

layout(location = 0) out vec4 FragColor;

layout(push_constant) uniform PushConstants {
    int camera_index;
} pc;
)";
            shader_string += GenerateShaderHeader(ShaderModuleType::Fragment);

            shader_string += R"(
void main() {
    int entity_index = inID;
    Entity entity = GetEntityData(entity_index);
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
void GetEntityModel(int entity_index, out mat4 out_model, out int parent_index) {
    Entity entity = GetEntityData(entity_index);

    mat4 model = mat4(1.0);
    model[3] = vec4(entity.position.xyz, 1.0);

    mat4 scale = mat4(1.0);
    scale[0].xyz *= entity.scale.x;
    scale[1].xyz *= entity.scale.y;
    scale[2].xyz *= entity.scale.z;

    mat4 rotX = mat4(1.0);
    rotX[1][1] =  cos(entity.rotation.x);
    rotX[1][2] = -sin(entity.rotation.x);
    rotX[2][1] =  sin(entity.rotation.x);
    rotX[2][2] =  cos(entity.rotation.x);

    mat4 rotY = mat4(1.0);
    rotY[0][0] =  cos(entity.rotation.y);
    rotY[0][2] =  sin(entity.rotation.y);
    rotY[2][0] = -sin(entity.rotation.y);
    rotY[2][2] =  cos(entity.rotation.y);

    mat4 rotZ = mat4(1.0);
    rotZ[0][0] =  cos(entity.rotation.z);
    rotZ[0][1] = -sin(entity.rotation.z);
    rotZ[1][0] =  sin(entity.rotation.z);
    rotZ[1][1] =  cos(entity.rotation.z);

    mat4 rot = rotZ * rotY * rotX;

    out_model = model * rot * scale;

    parent_index = entity.parent_index;
}

void main() {
    int entity_index = int(gl_GlobalInvocationID.x);
    if (entity_index >= Entitys.data.length()) return;
    
    int current_index = entity_index;

    mat4 final_model = mat4(1.0);

    while (current_index != -1) {
        mat4 current_model = mat4(1.0);
        GetEntityModel(current_index, current_model, current_index);
        final_model = current_model * final_model;
    }

    Entitys.data[entity_index].model = final_model;
}
)";
            return shader_string;
        }

        // std::vector<size_t> GetSceneIDs() {
        //     std::vector<size_t> ids(id_scene_groups.size(), 0);
        //     size_t i = 0;
        //     for (auto& [id, entry] : id_scene_groups)
        //         ids[i++] = id;
        //     return ids;
        // }

        // std::map<size_t, SceneReflectableGroup>::iterator GetScenesBegin() {
        //     return id_scene_groups.begin();
        // }

        // std::map<size_t, SceneReflectableGroup>::iterator GetScenesEnd() {
        //     return id_scene_groups.end();
        // }

        // std::map<int, CameraReflectableGroup>::iterator GetCamerasBegin(size_t scene_id) {
        //     auto scene_group_it = id_scene_groups.find(scene_id);
        //     if (scene_group_it == id_scene_groups.end())
        //         throw std::runtime_error("scene group not found with scene_id");
        //     return scene_group_it->second.indexed_camera_groups.begin();
        // }

        // std::map<int, CameraReflectableGroup>::iterator GetCamerasEnd(size_t scene_id) {
        //     auto scene_group_it = id_scene_groups.find(scene_id);
        //     if (scene_group_it == id_scene_groups.end())
        //         throw std::runtime_error("scene group not found with scene_id");
        //     return scene_group_it->second.indexed_camera_groups.end();
        // }

        // Image* GetFramebufferImage(size_t camera_index) {
        //     for (auto& [scene_id, scene_group] : id_scene_groups) {
        //         auto camera_it = scene_group.indexed_camera_groups.find(camera_index);
        //         if (camera_it == scene_group.indexed_camera_groups.end())
        //             continue;
        //         return camera_it->second.fb_color_image;
        //     }
        //     return nullptr;
        // }

        bool ResizeFramebuffer(size_t camera_id, uint32_t width, uint32_t height) {
            auto& camera_group = GetGroup<CameraProviderT, typename CameraProviderT::ReflectableGroup>(camera_id);
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
            auto& camera_group = GetGroup<CameraProviderT, typename CameraProviderT::ReflectableGroup>(camera_id);
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

    };
}