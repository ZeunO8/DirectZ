
#pragma once
#include <spirv_reflect.h>
#include <shaderc/shaderc.hpp>
#include <dz/Shader.hpp>
#include <dz/Framebuffer.hpp>
#include <unordered_map>
#include <map>
#include <dz/AssetPack.hpp>
#include <dz/BufferGroup.hpp>
#include <dz/Image.hpp>
#include <dz/ReflectedStructView.hpp>

namespace dz {
    struct ReflectedType {
        uint32_t id;
        std::string name;
        std::string type_kind; // e.g., "float", "vec3", "mat4", "struct", "array"
        uint32_t size_in_bytes = 0; // Size of the type in bytes (computed or derived)
        SpvReflectTypeDescription type_desc;
        SpvReflectDecorationFlags decorations; // Decorations applied to the type
        uint32_t array_stride;
    };

    bool hasCanonicalStruct(const ReflectedType& type);

    uint32_t GetMinimumTypeSizeInBytes(const SpvReflectTypeDescription& type_desc);
    uint32_t CalculateStructSize(const SpvReflectTypeDescription& type_desc);
    bool shader_buffers_ensure_and_bind(BufferGroup* buffer_group, Shader* shader);
    VkShaderStageFlags GetShaderStageFromModuleType(ShaderModuleType type);

    struct ReflectedVariable {
        std::string name;
        uint32_t location = static_cast<uint32_t>(-1);
        uint32_t binding = static_cast<uint32_t>(-1);
        uint32_t set = static_cast<uint32_t>(-1);
        uint32_t offset = static_cast<uint32_t>(-1); // Byte offset within its parent block/struct
        uint32_t array_stride = 0; // Stride for array types (if applicable)
        SpvReflectDescriptorType descriptor_type = (SpvReflectDescriptorType)-1;
        ReflectedType type;
        SpvReflectDecorationFlags decorations; // Decorations applied to the variable
    };

    struct ReflectedBlock {
        std::string name;
        uint32_t binding;
        uint32_t set;
        std::vector<ReflectedVariable> members;
        ReflectedType type; // Represents the type of the block itself (e.g., UniformBufferObject)
    };

    struct ReflectedStruct {
        std::string name;
        uint32_t id;
        SpvReflectTypeDescription type;

        std::map<std::string, SpvReflectTypeDescription> member_type_descs; // Maps member name (e.g., "model") to its SpvReflectTypeDescription
        std::map<std::string, uint32_t> member_offsets_map; // Maps member name to its offset within the struct
        std::map<std::string, uint32_t> member_sizes_map;   // Maps member name to its reflected size

        ReflectedStruct() = default;
        ReflectedStruct(std::string n, uint32_t i, SpvReflectTypeDescription td) : name(std::move(n)), id(i), type(td) {
            uint32_t offset = 0;
            for (uint32_t j = 0; j < type.member_count; ++j) {
                auto type_desc = type.members[j];
                auto name = type_desc.struct_member_name ? type_desc.struct_member_name : type_desc.type_name;
                if (name) {
                    auto size = GetMinimumTypeSizeInBytes(type_desc);
                    member_type_descs[name] = type_desc;
                    member_offsets_map[name] = offset;
                    member_sizes_map[name] = size;
                    offset += size;
                }
            }
        }
    };

    ReflectedStructView::ReflectedStructView(uint8_t* base_ptr, const ReflectedStruct& struct_def)
        : m_base_ptr(base_ptr), m_struct_def(struct_def) {
        if (!m_base_ptr) {
            throw std::runtime_error("ReflectedStructView: Base pointer for element is null.");
        }
        if (m_struct_def.type.op != SpvOpTypeStruct) {
            throw std::runtime_error("ReflectedStructView: Provided ReflectedType is not a struct.");
        }
    }

    void ReflectedStructView::set_member(const std::string& member_name, const void* data_ptr, size_t data_size_bytes) {
        auto offset_it = m_struct_def.member_offsets_map.find(member_name);
        auto size_it = m_struct_def.member_sizes_map.find(member_name);

        if (offset_it == m_struct_def.member_offsets_map.end() ||
            size_it == m_struct_def.member_sizes_map.end()) {
            throw std::runtime_error("ReflectedStructView Error: Member '" + member_name + "' not found in struct '"
                        + m_struct_def.name + "'.");
        }

        uint32_t member_offset = offset_it->second;
        uint32_t reflected_size = size_it->second;

        if (!(reflected_size == data_size_bytes)) {
            throw std::runtime_error("ReflectedSize does not match data_size (bytes)");
        }

        uint8_t* target_addr = m_base_ptr + member_offset;
        size_t bytes_to_copy = std::min(data_size_bytes, (size_t)reflected_size);

        memcpy(target_addr, data_ptr, bytes_to_copy);
    }

    uint8_t* ReflectedStructView::get_member(const std::string& member_name, size_t data_size_bytes) {
        auto offset_it = m_struct_def.member_offsets_map.find(member_name);
        auto size_it = m_struct_def.member_sizes_map.find(member_name);
        
        if (offset_it == m_struct_def.member_offsets_map.end() ||
            size_it == m_struct_def.member_sizes_map.end()) {
            throw std::runtime_error("ReflectedStructView Error: Member '" + member_name + "' not found in struct '"
                        + m_struct_def.name + "'.");
        }

        uint32_t member_offset = offset_it->second;
        uint32_t reflected_size = size_it->second;

        if (!(reflected_size == data_size_bytes)) {
            throw std::runtime_error("ReflectedSize does not match data_size (bytes)");
        }

        return m_base_ptr + member_offset;
    }

    struct SPIRVReflection {
        std::vector<ReflectedVariable> inputs;
        std::vector<ReflectedVariable> outputs;
        std::vector<ReflectedBlock> ssbos;
        std::vector<ReflectedBlock> ubos;
        std::vector<ReflectedBlock> push_constants; // Added for clarity
        std::unordered_map<std::string, ReflectedStruct> structs_by_name;
        std::unordered_map<uint32_t, ReflectedStruct> structs_by_id;
        std::unordered_map<std::string, ReflectedType> named_types;
        std::shared_ptr<SpvReflectShaderModule> module_ptr = std::shared_ptr<SpvReflectShaderModule>(new SpvReflectShaderModule, [](SpvReflectShaderModule* module) {
            spvReflectDestroyShaderModule(module);
            delete module;
        });
    };

    const ReflectedStruct& getCanonicalStruct(const SPIRVReflection& reflection, const ReflectedType& type);

    struct ShaderModule {
        std::vector<uint32_t> spirv_vec;
        VkShaderModule vk_module;
        ShaderModuleType type;
        SPIRVReflection reflection;
    };

    struct GpuBuffer {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceMemory memory = VK_NULL_HANDLE;
        void* mapped_memory = nullptr; // Persistently mapped pointer
        VkDeviceSize size = 0;
    };

    struct ShaderBuffer {
        std::string name; // Name from GLSL (e.g., "ubo_scene", "particles")
        uint32_t set = 0;
        uint32_t binding = 0;
        VkDescriptorType descriptor_type = VK_DESCRIPTOR_TYPE_MAX_ENUM;

        // Data layout from reflection
        VkDeviceSize static_size = 0; // Full size for UBOs, or size of ONE element for SSBOs
        uint32_t element_stride = 0; // Stride of one element in an array (for SSBOs)
        bool is_dynamic_sized = false; // Is this a runtime-sized SSBO?
        ReflectedType element_type;

        // Application-controlled data
        uint32_t element_count = 1; // Application sets this for dynamic SSBOs

        // The smart pointer that will point to CPU data initially, then GPU mapped memory.
        // The custom deleter will be empty for GPU memory, preventing crashes.
        std::shared_ptr<uint8_t> data_ptr = nullptr;

        // The final Vulkan resource
        GpuBuffer gpu_buffer;
    };

    struct ShaderImage {
        std::string name;
        uint32_t set = 0;
        uint32_t binding = 0;
        std::unordered_map<Shader*, VkDescriptorType> descriptor_types;
        VkImageViewType viewType;
        VkFormat format;
        std::unordered_map<Shader*, VkImageLayout> expected_layouts;
    };

    struct PushConstant {
        std::shared_ptr<void> ptr;
        uint32_t size = 0;
        uint32_t offset = 0;
        VkShaderStageFlags stageFlags = (VkShaderStageFlags)0;
    };

    struct Shader {
        bool initialized = false;
        std::map<ShaderModuleType, ShaderModule> module_map;
        std::map<uint32_t, VkDescriptorSetLayout> descriptor_set_layouts;
        VkDescriptorPool descriptor_pool = VK_NULL_HANDLE;
        std::map<uint32_t, VkDescriptorSet> descriptor_sets;
        std::map<BufferGroup*, bool> buffer_groups;
        std::vector<BufferGroup*> bound_buffer_groups;
        VkPipelineLayout pipeline_layout = VK_NULL_HANDLE;
        VkPipeline graphics_pipeline = VK_NULL_HANDLE;
        VkRenderPass render_pass = VK_NULL_HANDLE;
        std::map<std::string, std::string> define_map;
        std::unordered_map<std::string, size_t> push_constants_name_index;
        std::map<size_t, PushConstant> push_constants;
        AssetPack* include_asset_pack = 0;
        ShaderTopology topology;
        VkRenderPass renderPass = VK_NULL_HANDLE;
        std::unordered_map<std::string, Image*> sampler_key_image_override_map;
        std::unordered_map<std::string, uint32_t> keyed_set_binding_index_map;
        float line_width = 1.0f;
        bool depth_test_enabled = true;
        bool depth_write_enabled = true;
        bool depth_clamp_enabled = false;
        VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
        VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS;
        VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace front_face = VK_FRONT_FACE_CLOCKWISE;
        BlendState blend_state = BlendState::Disabled;
        bool depth_bounds_test_enabled = false;
        bool stencil_test_enabled = false;
    };

    struct BufferGroup {
        std::string group_name;
        std::unordered_map<std::string, ShaderBuffer> buffers;
        std::unordered_map<std::string, ShaderImage> images;
        std::unordered_map<std::string, std::shared_ptr<Image>> runtime_images;
        std::unordered_map<Shader*, bool> shaders;
        std::unordered_map<std::string, bool> restricted_to_keys;
    };
}