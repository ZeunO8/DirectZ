

struct ReflectedType
{
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
bool CreateAndBindShaderBuffers(BufferGroup* buffer_group, Shader* shader);

struct ReflectedVariable
{
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

struct ReflectedBlock
{
    std::string name;
    uint32_t binding;
    uint32_t set;
    std::vector<ReflectedVariable> members;
    ReflectedType type; // Represents the type of the block itself (e.g., UniformBufferObject)
};

struct ReflectedStruct
{
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

    if (!(reflected_size == data_size_bytes))
    {
        throw std::runtime_error("ReflectedSize does not match data_size (bytes)");
    }

    uint8_t* target_addr = m_base_ptr + member_offset;
    size_t bytes_to_copy = std::min(data_size_bytes, (size_t)reflected_size);

    memcpy(target_addr, data_ptr, bytes_to_copy);
}

uint8_t* ReflectedStructView::get_member(const std::string& member_name, size_t data_size_bytes)
{
    auto offset_it = m_struct_def.member_offsets_map.find(member_name);
    auto size_it = m_struct_def.member_sizes_map.find(member_name);
    
    if (offset_it == m_struct_def.member_offsets_map.end() ||
        size_it == m_struct_def.member_sizes_map.end())
    {
        throw std::runtime_error("ReflectedStructView Error: Member '" + member_name + "' not found in struct '"
                    + m_struct_def.name + "'.");
    }

    uint32_t member_offset = offset_it->second;
    uint32_t reflected_size = size_it->second;

    if (!(reflected_size == data_size_bytes))
    {
        throw std::runtime_error("ReflectedSize does not match data_size (bytes)");
    }

    return m_base_ptr + member_offset;
}

struct SPIRVReflection
{
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

struct ShaderModule
{
    std::vector<uint32_t> spirv_vec;
    VkShaderModule vk_module;
    ShaderModuleType type;
    SPIRVReflection reflection;
};

struct GpuBuffer
{
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceMemory memory = VK_NULL_HANDLE;
    void* mapped_memory = nullptr; // Persistently mapped pointer
    VkDeviceSize size = 0;
};

struct ShaderBuffer
{
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

struct ShaderImage
{
    std::string name;
    uint32_t set = 0;
    uint32_t binding = 0;
    std::unordered_map<Shader*, VkDescriptorType> descriptor_types;
    VkImageViewType viewType;
    VkFormat format;
    std::unordered_map<Shader*, VkImageLayout> expected_layouts;
};

struct Shader
{
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
    AssetPack* include_asset_pack = 0;
};

struct BufferGroup
{
    std::string group_name;
    std::unordered_map<std::string, ShaderBuffer> buffers;
    std::unordered_map<std::string, ShaderImage> images;
    std::unordered_map<std::string, std::shared_ptr<Image>> runtime_images;
    std::unordered_map<Shader*, bool> shaders;
    std::unordered_map<std::string, bool> restricted_to_keys;
};

class DynamicIncluder : public shaderc::CompileOptions::IncluderInterface
{
    Shader* shader = nullptr;

public:

    DynamicIncluder(Shader* shader)
        : shader(shader)
    {}

    shaderc_include_result* GetInclude(const char* requested_source,
                                       shaderc_include_type type,
                                       const char* requesting_source,
                                       size_t include_depth) override
    {
        if (!shader || !shader->include_asset_pack || !requested_source)
            return MakeErrorInclude("Missing shader or include asset pack");

        auto asset_pack = shader->include_asset_pack;
        Asset glsl;
        auto asset_available = get_asset(asset_pack, requested_source, glsl);

        if (!asset_available || !glsl.ptr || glsl.size == 0)
        {
            return MakeErrorInclude(std::string("Failed to resolve include: ") + requested_source);
        }

        std::string source_string((const char*)(glsl.ptr));
        return MakeSuccessInclude(source_string, requested_source);
    }

    void ReleaseInclude(shaderc_include_result* result) override
    {
        if (result)
        {
            delete[] result->source_name;
            delete[] result->content;
            delete result;
        }
    }

private:

    shaderc_include_result* MakeSuccessInclude(const std::string& content, const std::string& fullPath)
    {
        shaderc_include_result* result = new shaderc_include_result();

        result->source_name = CopyString(fullPath);
        result->source_name_length = fullPath.size();

        result->content = CopyString(content);
        result->content_length = content.size();

        result->user_data = nullptr;

        return result;
    }

    shaderc_include_result* MakeErrorInclude(const std::string& errorMessage)
    {
        shaderc_include_result* result = new shaderc_include_result();

        std::string name = "<error>";

        result->source_name = CopyString(name);
        result->source_name_length = name.size();

        result->content = CopyString(errorMessage);
        result->content_length = errorMessage.size();

        result->user_data = nullptr;

        return result;
    }

    char* CopyString(const std::string& str)
    {
        char* mem = new char[str.size() + 1];
        memcpy(mem, str.c_str(), str.size());
        mem[str.size()] = '\0';
        return mem;
    }
};

void shader_destroy(Shader* shader);

Shader* shader_create()
{
    auto shader = new Shader;
    auto& dr = *get_direct_registry();
    dr.uid_shader_map[GlobalUID::GetNew()] = std::shared_ptr<Shader>(shader, [](Shader* shader) {
        shader_destroy(shader);
        delete shader;
    });
    return shader;
}

void shader_include_asset_pack(Shader* shader, AssetPack* asset_pack)
{
    shader->include_asset_pack = asset_pack;
}

/* REFLECT */

// Helper to convert SpvReflectDecorationFlags to string
std::string GetDecorationString(SpvReflectDecorationFlags decorations) {
    std::string dec_str;
    if (decorations & SPV_REFLECT_DECORATION_BLOCK) dec_str += "Block ";
    if (decorations & SPV_REFLECT_DECORATION_BUFFER_BLOCK) dec_str += "BufferBlock ";
    if (decorations & SPV_REFLECT_DECORATION_ROW_MAJOR) dec_str += "RowMajor ";
    if (decorations & SPV_REFLECT_DECORATION_COLUMN_MAJOR) dec_str += "ColMajor ";
    if (decorations & SPV_REFLECT_DECORATION_BUILT_IN) dec_str += "BuiltIn ";
    if (decorations & SPV_REFLECT_DECORATION_NOPERSPECTIVE) dec_str += "NoPerspective ";
    if (decorations & SPV_REFLECT_DECORATION_FLAT) dec_str += "Flat ";
    if (decorations & SPV_REFLECT_DECORATION_NON_READABLE) dec_str += "NonReadable ";
    if (decorations & SPV_REFLECT_DECORATION_NON_WRITABLE) dec_str += "NonWritable ";
    if (decorations & SPV_REFLECT_DECORATION_RELAXED_PRECISION) dec_str += "RelaxedPrecision ";

    if (!dec_str.empty()) {
        dec_str.pop_back(); // Remove trailing space
    }
    return dec_str;
}

std::string GetTypeKindString(const SpvReflectTypeDescription& type_desc) {
    std::string kind_str;
    if ((type_desc.type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) != 0) {
        kind_str = "struct";
    } else if ((type_desc.type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) != 0) {
        kind_str = "array";
        // Use struct_type_description for array element type
        if (type_desc.struct_type_description) {
            kind_str += " of " + GetTypeKindString(*type_desc.struct_type_description);
        }
        if (type_desc.traits.array.dims_count > 0) {
            kind_str += " [";
            for (uint32_t i = 0; i < type_desc.traits.array.dims_count; ++i) {
                if (i > 0) kind_str += "x";
                if (type_desc.traits.array.dims[i] == 0) { // Runtime array
                    kind_str += "runtime";
                } else {
                    kind_str += std::to_string(type_desc.traits.array.dims[i]);
                }
            }
            kind_str += "]";
        }
    } else if ((type_desc.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) != 0) {
        kind_str = "mat" + std::to_string(type_desc.traits.numeric.matrix.column_count) + "x" + std::to_string(type_desc.traits.numeric.matrix.row_count);
        // Use struct_type_description for base scalar type of matrix components
        if (type_desc.struct_type_description) {
            kind_str += " of " + GetTypeKindString(*type_desc.struct_type_description);
        }
    } else if ((type_desc.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) != 0) {
        kind_str = "vec" + std::to_string(type_desc.traits.numeric.vector.component_count);
        // Use struct_type_description for base scalar type of vector components
        if (type_desc.struct_type_description) {
            kind_str += " of " + GetTypeKindString(*type_desc.struct_type_description);
        }
    } else if ((type_desc.type_flags & SPV_REFLECT_TYPE_FLAG_FLOAT) != 0) {
        kind_str = "float" + std::to_string(type_desc.traits.numeric.scalar.width);
    } else if ((type_desc.type_flags & SPV_REFLECT_TYPE_FLAG_INT) != 0) {
        // Use traits.numeric.scalar.signedness directly
        kind_str = (type_desc.traits.numeric.scalar.signedness != 0) ? "int" : "uint";
        kind_str += std::to_string(type_desc.traits.numeric.scalar.width);
    } else if ((type_desc.type_flags & SPV_REFLECT_TYPE_FLAG_BOOL) != 0) {
        kind_str = "bool";
    } else if (type_desc.op == SpvOpTypeImage) {
        kind_str = "image";
    } else if (type_desc.op == SpvOpTypeSampler) {
        kind_str = "sampler";
    } else if (type_desc.op == SpvOpTypeSampledImage) {
        kind_str = "sampled_image";
    } else if (type_desc.op == SpvOpTypeVoid) {
        kind_str = "void";
    } else {
        kind_str = "unknown_op_" + std::to_string(type_desc.op);
    }
    return kind_str;
}

uint32_t CalculateStructSize(const SpvReflectTypeDescription& type_desc)
{
    uint32_t size = 0;
    for (uint32_t i = 0; i < type_desc.member_count; ++i)
    {
        const auto& m = type_desc.members[i];
        size += GetMinimumTypeSizeInBytes(m);
        continue;
    }
    return size;
}

uint32_t GetMinimumTypeSizeInBytes(const SpvReflectTypeDescription& type_desc) {
    // SpvReflectTypeDescription itself doesn't directly expose 'size'.
    // We derive it from traits for basic types.
    // For structs, the 'size' field typically exists on SpvReflectBlockVariable,
    // which describes an instance of the struct, not the definition itself.
    // We'll return 0 for structs here as their size is context-dependent (padding/alignment).

    if (type_desc.op == SpvOpTypeRuntimeArray) {
        if (type_desc.struct_member_name)
        {
            return CalculateStructSize(type_desc);
        }
        return 0; // Runtime arrays have size determined at runtime
    }

    if ((type_desc.type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) != 0) {
        // Use struct_type_description for array element type
        if (type_desc.traits.array.dims_count > 0) {
            uint32_t total_elements = 1;
            for (uint32_t i = 0; i < type_desc.traits.array.dims_count; ++i) {
                if (type_desc.traits.array.dims[i] == 0) return 0;
                total_elements *= type_desc.traits.array.dims[i];
            }
            return total_elements * (type_desc.traits.array.stride ? type_desc.traits.array.stride : (type_desc.traits.numeric.scalar.width / 8));
        }
    } else if ((type_desc.type_flags & SPV_REFLECT_TYPE_FLAG_MATRIX) != 0) {
            return type_desc.traits.numeric.matrix.column_count * type_desc.traits.numeric.matrix.row_count * (type_desc.traits.numeric.scalar.width / 8);
    } else if ((type_desc.type_flags & SPV_REFLECT_TYPE_FLAG_VECTOR) != 0) {
        return type_desc.traits.numeric.vector.component_count * (type_desc.traits.numeric.scalar.width / 8);
    } else if ((type_desc.type_flags & (SPV_REFLECT_TYPE_FLAG_FLOAT | SPV_REFLECT_TYPE_FLAG_INT | SPV_REFLECT_TYPE_FLAG_BOOL)) != 0) {
        return type_desc.traits.numeric.scalar.width / 8; // Convert bits to bytes
    } else if ((type_desc.type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) != 0) {
        return CalculateStructSize(type_desc);
    }
    return 0; // Default if size cannot be determined for other types
}

ReflectedType CreateReflectedType(const SpvReflectTypeDescription& type_desc) {
    ReflectedType rtype;
    rtype.id = type_desc.id;
    auto name = type_desc.struct_member_name ? type_desc.struct_member_name : type_desc.type_name;
    rtype.name = name ? name : "";
    rtype.type_kind = GetTypeKindString(type_desc);
    rtype.size_in_bytes = GetMinimumTypeSizeInBytes(type_desc);
    rtype.type_desc = type_desc; // Copy the original descriptor for full details
    rtype.decorations = type_desc.decoration_flags;
    rtype.array_stride = type_desc.traits.array.stride;
    return rtype;
}

size_t HashStructSignature(const SpvReflectTypeDescription& type)
{
    size_t hash = 0;
    for (uint32_t i = 0; i < type.member_count; ++i)
    {
        const auto& m = type.members[i];
        hash ^= std::hash<std::string>()(m.struct_member_name ? m.struct_member_name : "") << 1;
        hash ^= m.type_flags + 0x9e3779b9;
        hash ^= m.traits.numeric.scalar.width; // Use width as a basic signature component
        // Note: m.offset is part of SpvReflectBlockVariable, not SpvReflectTypeDescription member.
        // If it's a direct member of a struct, you might not have offset here.
        // For `SpvReflectTypeDescription::members`, the 'members' field is deprecated
        // and doesn't contain a 'member_variable' field like SpvReflectBlockVariable.
        // Let's remove m.offset from hash signature as it's not directly available on SpvReflectTypeDescription's members.
    }
    return hash;
}

void ReflectAllTypes(const SpvReflectShaderModule& module, SPIRVReflection& out)
{
    auto& internal = *module._internal;
    std::unordered_map<size_t, uint32_t> seen_structs;

    for (uint32_t i = 0; i < internal.type_description_count; ++i)
    {
        const SpvReflectTypeDescription& type = internal.type_descriptions[i];
        // Only process named types or structs that might be referenced
        if (!(type.type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT))
            continue;

        ReflectedType rtype = CreateReflectedType(type);
        if (!rtype.name.empty()) { // Only add named types to the map
            out.named_types[rtype.name] = rtype;
        }

        if ((type.type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) != 0)
        {
            size_t sig = HashStructSignature(type);
            if (seen_structs.find(sig) == seen_structs.end())
            {
                seen_structs[sig] = type.id;
                ReflectedStruct s(rtype.name, rtype.id, type);
                out.structs_by_name[rtype.name] = s;
                out.structs_by_id[rtype.id] = s;
            }
        }
    }
}

void ReflectStructMembersRecursive(const SpvReflectTypeDescription* type_desc, std::vector<ReflectedVariable>& container, const std::string& prefix = "")
{
    if (!type_desc || !(type_desc->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT))
        return;

    // Use the type_desc->members (which are SpvReflectTypeDescription) for recursive calls
    // but the actual block variable members (if available) for offsets/strides
    for (uint32_t mi = 0; mi < type_desc->member_count; ++mi)
    {
        const SpvReflectTypeDescription* member_type_desc = &type_desc->members[mi];
        // Note: SpvReflectTypeDescription::members are just type descriptions,
        // they don't have the full SpvReflectBlockVariable info (like offset, array.stride, decorations)
        // unless they are explicitly block variables themselves.
        // For structs, member_variable isn't directly exposed on SpvReflectTypeDescription members.
        // These fields are typically available when iterating through SpvReflectBlockVariable::members.

        ReflectedVariable rv;
        rv.name = prefix + (member_type_desc->struct_member_name ? member_type_desc->struct_member_name : "");
        // Offsets, strides, and decorations for struct members are typically found in the SpvReflectBlockVariable
        // that *contains* this struct, not on the struct member's type description itself.
        // For now, these will be default values here unless passed explicitly or looked up.
        // If this function is only called for actual block members (UBO/SSBO), then the `member_block_var` is valid.
        // However, the current signature passes type_desc directly.

        // To get accurate offsets/strides/decorations for nested members,
        // this function would ideally be called with the actual SpvReflectBlockVariable members
        // if reflecting a uniform/storage buffer.
        // For now, we'll populate basic type info.
        rv.type = CreateReflectedType(*member_type_desc);
        // rv.offset and rv.array_stride cannot be reliably obtained from SpvReflectTypeDescription::members alone.
        // rv.decorations will be based on the type's decorations, not the variable's.


        if (member_type_desc->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT)
        {
            // Recursively add members of nested structs
            ReflectStructMembersRecursive(member_type_desc, container, rv.name + ".");
        }
        else if ((member_type_desc->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) && member_type_desc->struct_type_description)
        {
            // If it's an array of structs, recurse into the element type.
            if (member_type_desc->struct_type_description->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) {
                ReflectStructMembersRecursive(member_type_desc->struct_type_description, container, rv.name + "[].");
            } else {
                container.push_back(rv); // Add array of basic types as a leaf variable
            }
        }
        else
        {
            container.push_back(rv); // Add basic type as a leaf variable
        }
    }
}

void ReflectIO(const SpvReflectShaderModule& module, SPIRVReflection& out)
{
    for (uint32_t i = 0; i < module.input_variable_count; ++i)
    {
        const SpvReflectInterfaceVariable* var = module.input_variables[i];
        if (!var || !var->type_description)
            continue;

        ReflectedVariable rvar;
        rvar.name = var->name ? var->name : "";
        rvar.location = var->location;
        rvar.type = CreateReflectedType(*var->type_description);
        rvar.array_stride = var->array.stride; // Interface variables can also be arrays
        out.inputs.push_back(rvar);
    }

    for (uint32_t i = 0; i < module.output_variable_count; ++i)
    {
        const SpvReflectInterfaceVariable* var = module.output_variables[i];
        if (!var || !var->type_description)
            continue;

        ReflectedVariable rvar;
        rvar.name = var->name ? var->name : "";
        rvar.location = var->location;
        rvar.type = CreateReflectedType(*var->type_description);
        rvar.array_stride = var->array.stride;
        out.outputs.push_back(rvar);
    }
}

void ReflectStructMembers(
    const SpvReflectTypeDescription* type_desc,
    SPIRVReflection& out,
    std::vector<ReflectedVariable>& container)
{
    if (!type_desc || !(type_desc->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT))
        return;

    const SpvReflectTypeDescription* struct_desc = type_desc;
    if ((type_desc->type_flags & SPV_REFLECT_TYPE_FLAG_ARRAY) && type_desc->struct_type_description)
        struct_desc = type_desc->struct_type_description;

    for (uint32_t mi = 0; mi < struct_desc->member_count; ++mi)
    {
        const SpvReflectTypeDescription* member_desc = &struct_desc->members[mi];
        if (!member_desc)
            continue;

        ReflectedVariable rv;
        rv.name = member_desc->struct_member_name ? member_desc->struct_member_name : "";
        rv.type.id = member_desc->id;
        rv.type.name = member_desc->type_name ? member_desc->type_name : "";
        rv.type.type_desc = *member_desc;
        container.push_back(rv);
    }
}

VkImageViewType infer_view_type(const SpvReflectImageTraits& image)
{
    switch (image.dim)
    {
        case SpvDim1D: return image.arrayed ? VK_IMAGE_VIEW_TYPE_1D_ARRAY : VK_IMAGE_VIEW_TYPE_1D;
        case SpvDim2D: return image.arrayed ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        case SpvDim3D: return VK_IMAGE_VIEW_TYPE_3D;
        case SpvDimCube: return image.arrayed ? VK_IMAGE_VIEW_TYPE_CUBE_ARRAY : VK_IMAGE_VIEW_TYPE_CUBE;
        default: return VK_IMAGE_VIEW_TYPE_2D;
    }
}

VkFormat infer_format(SpvImageFormat fmt)
{
    switch (fmt)
    {
        case SpvImageFormatRgba32f: return VK_FORMAT_R32G32B32A32_SFLOAT;
        case SpvImageFormatRgba16f: return VK_FORMAT_R16G16B16A16_SFLOAT;
        case SpvImageFormatR32f: return VK_FORMAT_R32_SFLOAT;
        case SpvImageFormatRgba8: return VK_FORMAT_R8G8B8A8_UNORM;
        case SpvImageFormatRgba8Snorm: return VK_FORMAT_R8G8B8A8_SNORM;
        case SpvImageFormatRg32f: return VK_FORMAT_R32G32_SFLOAT;
        case SpvImageFormatRg16f: return VK_FORMAT_R16G16_SFLOAT;
        case SpvImageFormatR11fG11fB10f: return VK_FORMAT_B10G11R11_UFLOAT_PACK32;
        case SpvImageFormatR16f: return VK_FORMAT_R16_SFLOAT;
        case SpvImageFormatRgba16: return VK_FORMAT_R16G16B16A16_UNORM;
        case SpvImageFormatRgb10A2: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
        case SpvImageFormatRg16: return VK_FORMAT_R16G16_UNORM;
        case SpvImageFormatRg8: return VK_FORMAT_R8G8_UNORM;
        case SpvImageFormatR16: return VK_FORMAT_R16_UNORM;
        case SpvImageFormatR8: return VK_FORMAT_R8_UNORM;
        case SpvImageFormatRgba16Snorm: return VK_FORMAT_R16G16B16A16_SNORM;
        case SpvImageFormatRg16Snorm: return VK_FORMAT_R16G16_SNORM;
        case SpvImageFormatRg8Snorm: return VK_FORMAT_R8G8_SNORM;
        case SpvImageFormatR16Snorm: return VK_FORMAT_R16_SNORM;
        case SpvImageFormatR8Snorm: return VK_FORMAT_R8_SNORM;

        case SpvImageFormatRgba32i: return VK_FORMAT_R32G32B32A32_SINT;
        case SpvImageFormatRgba16i: return VK_FORMAT_R16G16B16A16_SINT;
        case SpvImageFormatRgba8i: return VK_FORMAT_R8G8B8A8_SINT;
        case SpvImageFormatR32i: return VK_FORMAT_R32_SINT;
        case SpvImageFormatRg32i: return VK_FORMAT_R32G32_SINT;
        case SpvImageFormatRg16i: return VK_FORMAT_R16G16_SINT;
        case SpvImageFormatRg8i: return VK_FORMAT_R8G8_SINT;
        case SpvImageFormatR16i: return VK_FORMAT_R16_SINT;
        case SpvImageFormatR8i: return VK_FORMAT_R8_SINT;

        case SpvImageFormatRgba32ui: return VK_FORMAT_R32G32B32A32_UINT;
        case SpvImageFormatRgba16ui: return VK_FORMAT_R16G16B16A16_UINT;
        case SpvImageFormatRgba8ui: return VK_FORMAT_R8G8B8A8_UINT;
        case SpvImageFormatR32ui: return VK_FORMAT_R32_UINT;
        case SpvImageFormatRgb10a2ui: return VK_FORMAT_A2R10G10B10_UINT_PACK32;
        case SpvImageFormatRg32ui: return VK_FORMAT_R32G32_UINT;
        case SpvImageFormatRg16ui: return VK_FORMAT_R16G16_UINT;
        case SpvImageFormatRg8ui: return VK_FORMAT_R8G8_UINT;
        case SpvImageFormatR16ui: return VK_FORMAT_R16_UINT;
        case SpvImageFormatR8ui: return VK_FORMAT_R8_UINT;

        case SpvImageFormatR64ui: return VK_FORMAT_R64_UINT;
        case SpvImageFormatR64i: return VK_FORMAT_R64_SINT;

        case SpvImageFormatUnknown: // Fallthrough
        default: return VK_FORMAT_UNDEFINED;
    }
}

VkImageLayout infer_required_image_layout(VkDescriptorType descriptor_type)
{
    switch (descriptor_type)
    {
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return VK_IMAGE_LAYOUT_GENERAL;

        case VK_DESCRIPTOR_TYPE_SAMPLER:
            return VK_IMAGE_LAYOUT_UNDEFINED; // no image associated

        default:
            return VK_IMAGE_LAYOUT_UNDEFINED;
    }
}

void ReflectAndPrepareBuffers(BufferGroup* buffer_group, const SpvReflectShaderModule& module, Shader* shader, SPIRVReflection& reflection)
{
    for (uint32_t i = 0; i < module.descriptor_binding_count; ++i)
    {
        const auto& binding_info = module.descriptor_bindings[i];
        std::string name = binding_info.name ? binding_info.name : "";

        if (name.empty()) {
            std::cerr << "Warning: Skipping unnamed descriptor_binding at set=" << binding_info.set << ", binding=" << binding_info.binding << std::endl;
            continue;
        }
        // If we are restricted to buffers and this is not a restriction, skip.
        if (buffer_group->restricted_to_keys.size() && (buffer_group->restricted_to_keys.find(name)) == buffer_group->restricted_to_keys.end())
            continue;

        auto vk_descriptor_type = static_cast<VkDescriptorType>(binding_info.descriptor_type);

        switch (binding_info.descriptor_type)
        {
        default:
        {
            std::cerr << "Warning: Unhandled descriptor type: " << vk_descriptor_type << std::endl;
            break;
        }
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_IMAGE:
        {
            if (buffer_group->images.count(name))
            {
                auto& image_data = buffer_group->images[name];
                image_data.descriptor_types[shader] = vk_descriptor_type;
                image_data.expected_layouts[shader] = infer_required_image_layout(vk_descriptor_type);
                std::cout << "Added Descriptor Type: " << vk_descriptor_type << " to Reflected image '" << name << "'" << std::endl;
                continue;
            }


            ShaderImage image_data{};
            image_data.name = name;
            image_data.set = binding_info.set;
            image_data.binding = binding_info.binding;
            image_data.descriptor_types[shader] = vk_descriptor_type;
            image_data.viewType = infer_view_type(binding_info.image);
            image_data.format = infer_format(binding_info.image.image_format);
            image_data.expected_layouts[shader] = infer_required_image_layout(vk_descriptor_type);

            buffer_group->images[name] = image_data;
            std::cout << "Reflected image '" << name << "' (Set=" << image_data.set << ", Binding=" << image_data.binding
                    << ", Descriptor Type=" << vk_descriptor_type << ", View Type=" << image_data.viewType
                    << ", Format=" << image_data.format << ")" << std::endl;

            break;
        }
        case SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
        case SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER:
        {
            // If another shader stage already reflected this buffer, skip.
            if (buffer_group->buffers.count(name)) {
                continue;
            }

            ShaderBuffer buffer_data{};
            buffer_data.name = name;
            buffer_data.set = binding_info.set;
            buffer_data.binding = binding_info.binding;
            buffer_data.descriptor_type = vk_descriptor_type;
            buffer_data.static_size = binding_info.block.size; // This is the total size of the block from reflection

            // Determine element_type and related properties
            const auto& block = binding_info.block; // The entire block (struct or wrapper for content)
            const SpvReflectTypeDescription* element_type_desc_ptr = nullptr; // Pointer to the type description of one element

            if (binding_info.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                // For UBOs, the "element" is the entire block struct itself.
                element_type_desc_ptr = block.type_description;
                buffer_data.element_stride = block.size; // Stride is the size of the whole UBO struct
                buffer_data.element_count = 1; // UBOs typically have one instance
                buffer_data.is_dynamic_sized = false; // UBOs are fixed size
                std::cout << "Detected UBO '" << name << "': single struct element." << std::endl;
            } else if (binding_info.descriptor_type == SPV_REFLECT_DESCRIPTOR_TYPE_STORAGE_BUFFER) {
                // For SSBOs, we need to look into its members for the actual element type,
                // especially if it's an array. SSBOs are typically structs containing arrays.
                if (block.member_count > 0) {
                    const auto& primary_member = block.members[0]; // Assume the first member holds the data structure we care about

                    if (primary_member.type_description->op == SpvOpTypeRuntimeArray) {
                        // This is an SSBO with a runtime array as its primary content (e.g., `buffer MyBlock { SomeType data[]; }`)
                        element_type_desc_ptr = primary_member.type_description;
                        buffer_data.is_dynamic_sized = true;
                        if (element_type_desc_ptr->struct_member_name)
                        {
                            buffer_data.element_stride = GetMinimumTypeSizeInBytes(*primary_member.type_description);
                        }
                        else
                        {
                            buffer_data.element_stride = primary_member.type_description->traits.array.stride;
                        }
                        buffer_data.element_count = 0; // Requires application to set size for dynamic buffers
                        std::cout << "Detected dynamic SSBO '" << name << "': runtime array of " << element_type_desc_ptr->type_name << "s." << std::endl;
                    } else if (primary_member.type_description->op == SpvOpTypeArray) {
                        // This is an SSBO with a fixed-size array as its primary content (e.g., `buffer MyBlock { SomeType data[10]; }`)
                        element_type_desc_ptr = primary_member.type_description;
                        buffer_data.is_dynamic_sized = false; // Fixed size
                        buffer_data.element_stride = primary_member.type_description->traits.array.stride;
                        // buffer_data.element_count = primary_member.type_description->traits.array.length; // Explicit length
                        std::cout << "Detected fixed-size array SSBO '" << name << "': array of " << element_type_desc_ptr->type_name << "s with count " << buffer_data.element_count << "." << std::endl;
                    } else {
                        // This is an SSBO that contains a single struct or primitive as its primary member
                        // (e.g., `buffer MyBlock { MyStruct data; }` or `buffer MyBlock { float value; }`)
                        element_type_desc_ptr = primary_member.type_description;
                        buffer_data.is_dynamic_sized = false;
                        buffer_data.element_stride = primary_member.size; // Stride is simply the size of that single member
                        buffer_data.element_count = 1;
                        std::cout << "Detected single-element SSBO '" << name << "': element type " << element_type_desc_ptr->type_name << "." << std::endl;
                    }
                } else {
                    // This is an SSBO block with no members. This is unusual for data buffers.
                    // It might be a simple SSBO with just a type binding_info.type_description
                    // if it's not a block, but here we are in the 'block' context.
                    std::cerr << "Warning: SSBO '" << name << "' block has no members. Cannot reliably determine element type." << std::endl;
                    // Fallback: If it's a direct binding to a non-struct type, use that.
                    if (!(binding_info.type_description->type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) && binding_info.type_description->op != SpvOpTypeStruct) {
                        element_type_desc_ptr = binding_info.type_description;
                        buffer_data.is_dynamic_sized = false;
                        buffer_data.element_stride = GetMinimumTypeSizeInBytes(*binding_info.type_description);
                        buffer_data.element_count = 1;
                        std::cout << "Detected simple (non-struct) SSBO '" << name << "' via binding_info.type_description." << std::endl;
                    } else {
                        std::cerr << "Error: Skipping SSBO '" << name << "' due to ambiguous element type." << std::endl;
                        continue;
                    }
                }
            }

            // Assign the reflected element type
            if (element_type_desc_ptr) {
                buffer_data.element_type = CreateReflectedType(*element_type_desc_ptr);

                // Populate the ReflectedStruct into the shader's reflection if the element type is a struct
                if (buffer_data.element_type.type_kind == "struct") {
                    // Check if it's already there to avoid duplicates across shader stages
                    if (reflection.structs_by_name.find(buffer_data.element_type.name) == reflection.structs_by_name.end()) {
                        // Create a ReflectedStruct using the type_desc of the element
                        auto s = ReflectedStruct(buffer_data.element_type.name, buffer_data.element_type.id, *element_type_desc_ptr);
                        reflection.structs_by_name.emplace(buffer_data.element_type.name, s);
                        reflection.structs_by_id.emplace(buffer_data.element_type.id, s);
                        std::cout << "Added reflected struct '" << buffer_data.element_type.name << "' (ID: " << buffer_data.element_type.id << ") to global reflection data." << std::endl;
                    }
                }
            } else {
                std::cerr << "Error: Could not determine element type for buffer '" << name << "'." << std::endl;
                continue; // Skip this buffer if element type can't be determined
            }

            buffer_group->buffers[name] = buffer_data;
            std::cout << "Reflected buffer '" << name << "' (Set=" << buffer_data.set << ", Binding=" << buffer_data.binding
                    << ", Static Size=" << buffer_data.static_size << ", Element Stride=" << buffer_data.element_stride
                    << ", Element Type=" << buffer_data.element_type.name << ", Dynamic=" << (buffer_data.is_dynamic_sized ? "Yes" : "No") << ")" << std::endl;
            break;
        }
        }
    }
}

void ReflectPushConstants(const SpvReflectShaderModule& module, SPIRVReflection& out)
{
    for (uint32_t i = 0; i < module.push_constant_block_count; ++i)
    {
        const SpvReflectBlockVariable& push = module.push_constant_blocks[i];
        ReflectedBlock block;
        block.name = push.name ? push.name : "";
        block.binding = 0; // Push constants don't have explicit bindings/sets
        block.set = 0;     // in the same way as descriptor sets
        block.type = CreateReflectedType(*push.type_description); // Populate block type

        for (uint32_t j = 0; j < push.member_count; ++j)
        {
            const SpvReflectBlockVariable& member = push.members[j];
            ReflectedVariable m;
            m.name = member.name ? member.name : "";
            m.offset = member.offset;
            m.array_stride = member.array.stride;
            m.type = CreateReflectedType(*member.type_description);
            // m.decorations = member.decorations;
            block.members.push_back(m);
        }
        out.push_constants.push_back(block); // Store push constants in their own vector
    }
}


const char* CustomSpvReflectExecutionModelToString(SpvExecutionModel model) {
    switch (model) {
        case SpvExecutionModelVertex: return "Vertex";
        case SpvExecutionModelTessellationControl: return "TessellationControl";
        case SpvExecutionModelTessellationEvaluation: return "TessellationEvaluation";
        case SpvExecutionModelGeometry: return "Geometry";
        case SpvExecutionModelFragment: return "Fragment";
        case SpvExecutionModelGLCompute: return "GLCompute";
        case SpvExecutionModelKernel: return "Kernel";
        case SpvExecutionModelTaskNV: return "TaskNV";
        case SpvExecutionModelMeshNV: return "MeshNV";
        case SpvExecutionModelRayGenerationKHR: return "RayGenerationKHR";
        case SpvExecutionModelIntersectionKHR: return "IntersectionKHR";
        case SpvExecutionModelAnyHitKHR: return "AnyHitKHR";
        case SpvExecutionModelClosestHitKHR: return "ClosestHitKHR";
        case SpvExecutionModelMissKHR: return "MissKHR";
        case SpvExecutionModelCallableKHR: return "CallableKHR";
        default: return "Unknown";
    }
}


const char* CustomSpvReflectShaderStageToString(SpvReflectShaderStageFlagBits stage_flags) {
    // Note: Shader stage flags are a bitmask, but in the context of an entry point,
    // usually only one flag is set. We can still handle multiple for robustness.
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_VERTEX_BIT) return "Vertex";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_TESSELLATION_CONTROL_BIT) return "TessellationControl";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_TESSELLATION_EVALUATION_BIT) return "TessellationEvaluation";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_GEOMETRY_BIT) return "Geometry";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_FRAGMENT_BIT) return "Fragment";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_COMPUTE_BIT) return "Compute";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_TASK_BIT_NV) return "TaskNV";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_MESH_BIT_NV) return "MeshNV";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_RAYGEN_BIT_KHR) return "RayGenerationKHR";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_INTERSECTION_BIT_KHR) return "IntersectionKHR";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_ANY_HIT_BIT_KHR) return "AnyHitKHR";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_CLOSEST_HIT_BIT_KHR) return "ClosestHitKHR";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_MISS_BIT_KHR) return "MissKHR";
    if (stage_flags & SPV_REFLECT_SHADER_STAGE_CALLABLE_BIT_KHR) return "CallableKHR";
    return "Unknown";
}

void ReflectEntryPoints(const SpvReflectShaderModule& module)
{
    for (uint32_t i = 0; i < module.entry_point_count; ++i)
    {
        const SpvReflectEntryPoint& entry = module.entry_points[i];
        std::cout << "Entry Point: " << entry.name << std::endl;
        std::cout << "  Execution Model: " << CustomSpvReflectExecutionModelToString(entry.spirv_execution_model) << std::endl;
        std::cout << "  Shader Stage: " << CustomSpvReflectShaderStageToString(entry.shader_stage) << std::endl;
        std::cout << "  Local Size: X=" << entry.local_size.x << ", Y=" << entry.local_size.y << ", Z=" << entry.local_size.z << std::endl;
    }
}

void PrintTypeTree(const SpvReflectTypeDescription& type, int indent = 0)
{
    std::string pad(indent, ' ');
    auto size = 0;
    if (type.type_name ? (std::string(type.type_name) == "Entity") : false)
    {
        size *= 2;
    }
    size = GetMinimumTypeSizeInBytes(type);
    auto type_name = (type.struct_member_name ? type.struct_member_name : type.type_name);
    std::cout << pad << "Type ID: " << type.id
              << ", Name: " << (type_name ? type_name : "<anon>")
              << ", Kind: " << GetTypeKindString(type)
              << ", Size: " << size << " bytes";
    std::cout << ", Flags: " << type.type_flags << std::endl;


    if ((type.type_flags & SPV_REFLECT_TYPE_FLAG_STRUCT) != 0)
    {
        for (uint32_t i = 0; i < type.member_count; ++i)
        {
            const auto& member = type.members[i];
            std::cout << pad << "  Member: " << (member.struct_member_name ? member.struct_member_name : "<unnamed>") << std::endl;
            PrintTypeTree(member, indent + 4);
        }
    }
}

void PrintType(const SPIRVReflection& reflection, const ReflectedType& type, int indent = 0);

void PrintMember(const SPIRVReflection& reflection, const SpvReflectTypeDescription& member, int indent = 0);

void PrintStruct(const SPIRVReflection& reflection, const ReflectedStruct& s, int indent = 0)
{
    std::string pad(indent, ' ');
    std::cout << pad << "Struct: " << s.name << " (ID: " << s.id << "), Size: " << CalculateStructSize(s.type) << std::endl;
    if (s.member_type_descs.empty()) {
        std::cout << pad << "  (No members)" << std::endl;
    } else {
        for (const auto& member : s.member_type_descs)
        {
            std::cout << pad << "  Member:" << std::endl;
            PrintMember(reflection, member.second, indent + 4);
        }
    }
}

bool hasCanonicalStruct(const ReflectedType& type)
{
    return (type.type_desc.struct_type_description != nullptr);
}

const ReflectedStruct& getCanonicalStruct(const SPIRVReflection& reflection, const ReflectedType& type)
{
    const SpvReflectTypeDescription* struct_definition_desc = type.type_desc.struct_type_description;
    auto it = reflection.structs_by_name.find(struct_definition_desc->type_name);
    assert (it != reflection.structs_by_name.end());
    return it->second;
}

void PrintType(const SPIRVReflection& reflection, const ReflectedType& type, int indent)
{
    std::string pad(indent, ' ');
    if (hasCanonicalStruct(type))
    {
        std::cout << pad << "ReflectedVariable '" << type.name << "' (Named Type ID: " << type.id << ")";
        std::cout << pad << " refers to canonical Struct Definition ID: " << type.type_desc.struct_type_description->id << std::endl;
        auto& canonical_truct = getCanonicalStruct(reflection, type);
        std::cout << pad << "Accessed canonical struct '" << canonical_truct.name << "' (ID: " << canonical_truct.id << ")" << std::endl;
        PrintStruct(reflection, canonical_truct, indent + 2);
    }
    else
    {
        PrintTypeTree(type.type_desc, indent + 2);
    }
    std::string decorations_str = GetDecorationString(type.decorations);
    if (!decorations_str.empty()) {
        std::cout << ", Decorations: [" << decorations_str << "]";
    }
}

void PrintMember(const SPIRVReflection& reflection, const SpvReflectTypeDescription& member, int indent)
{
    std::string pad(indent, ' ');
    PrintTypeTree(member, indent + 2);
}

void PrintVariable(const SPIRVReflection& reflection, const ReflectedVariable& var, int indent = 0)
{
    std::string pad(indent, ' ');
    std::cout << pad << "Variable: " << (var.name.empty() ? "<unnamed>" : var.name) << std::endl;

    if (var.location != static_cast<uint32_t>(-1))
        std::cout << pad << "  Location: " << var.location << std::endl;

    if (var.binding != static_cast<uint32_t>(-1))
        std::cout << pad << "  Binding: " << var.binding << std::endl;

    if (var.set != static_cast<uint32_t>(-1))
        std::cout << pad << "  Set: " << var.set << std::endl;

    if (var.offset != static_cast<uint32_t>(-1))
        std::cout << pad << "  Offset: " << var.offset << " bytes" << std::endl;

    if (var.array_stride > 0)
        std::cout << pad << "  Array Stride: " << var.array_stride << " bytes" << std::endl;

    std::string decorations_str = GetDecorationString(var.decorations);
    if (!decorations_str.empty()) {
        std::cout << pad << "  Decorations: [" << decorations_str << "]" << std::endl;
    }

    if (var.descriptor_type != (SpvReflectDescriptorType)-1)
         std::cout << pad << "  Descriptor Type: " << var.descriptor_type << std::endl;

    std::cout << pad << "  Type:" << std::endl;
    PrintType(reflection, var.type, indent + 4);
}

void PrintBlock(const SPIRVReflection& reflection, const ReflectedBlock& block, int indent = 0)
{
    std::string pad(indent, ' ');
    std::cout << pad << "Block: " << (block.name.empty() ? "<unnamed>" : block.name) << std::endl;
    std::cout << pad << "  Binding: " << block.binding << std::endl;
    std::cout << pad << "  Set: " << block.set << std::endl;
    std::cout << pad << "  Type (Root):" << std::endl;
    PrintType(reflection, block.type, indent + 4);

    std::cout << pad << "  Members:" << std::endl;
    if (block.members.empty()) {
        std::cout << pad << "    (No members)" << std::endl;
    } else {
        for (const auto& m : block.members)
            PrintVariable(reflection, m, indent + 4);
    }
}

void PrintSPIRVReflection(const SPIRVReflection& refl)
{
    std::cout << "=== INPUTS ===" << std::endl;
    if (refl.inputs.empty()) std::cout << "  (No inputs)" << std::endl;
    for (const auto& input : refl.inputs)
        PrintVariable(refl, input, 2);

    std::cout << "=== OUTPUTS ===" << std::endl;
    if (refl.outputs.empty()) std::cout << "  (No outputs)" << std::endl;
    for (const auto& output : refl.outputs)
        PrintVariable(refl, output, 2);

    std::cout << "=== SSBOs ===" << std::endl;
    if (refl.ssbos.empty()) std::cout << "  (No SSBOs)" << std::endl;
    for (const auto& ssbo : refl.ssbos)
        PrintBlock(refl, ssbo, 2);

    std::cout << "=== UBOs ===" << std::endl;
    if (refl.ubos.empty()) std::cout << "  (No UBOs)" << std::endl;
    for (const auto& ubo : refl.ubos)
        PrintBlock(refl, ubo, 2);

    std::cout << "=== PUSH CONSTANTS ===" << std::endl;
    if (refl.push_constants.empty()) std::cout << "  (No Push Constants)" << std::endl;
    for (const auto& pc : refl.push_constants)
        PrintBlock(refl, pc, 2);

    std::cout << "=== STRUCTS BY ID ===" << std::endl;
    if (refl.structs_by_name.empty()) std::cout << "  (No structs by ID)" << std::endl;
    for (const auto& pair : refl.structs_by_name) // Use pair to avoid copying
        PrintStruct(refl, pair.second, 2);

    std::cout << "=== NAMED TYPES ===" << std::endl;
    if (refl.named_types.empty()) std::cout << "  (No named types)" << std::endl;
    for (const auto& pair : refl.named_types) // Use pair to avoid copying
    {
        std::cout << "Type Name: " << pair.first << std::endl;
        PrintType(refl, pair.second, 2);
    }
}

void PrintShaderReflection(Shader* shader)
{
    std::cout << "===============" << std::endl;
    std::cout << "  Shader Dump  " << std::endl;
    std::cout << "===============" << std::endl;
    for (auto& shaderModulePair : shader->module_map)
        PrintSPIRVReflection(shaderModulePair.second.reflection);
    std::cout << std::endl;
}

void shader_reflect(Shader* shader, ShaderModuleType module_type, shaderc_shader_kind stage)
{
    auto& shader_module = shader->module_map[module_type];
    auto& spirv_vec = shader_module.spirv_vec;

    auto& module = *shader_module.reflection.module_ptr.get();
    SpvReflectResult result = spvReflectCreateShaderModule(spirv_vec.size() * sizeof(uint32_t), spirv_vec.data(), &module);
    assert(result == SPV_REFLECT_RESULT_SUCCESS);

    ReflectAllTypes(module, shader_module.reflection);
    ReflectIO(module, shader_module.reflection);
    ReflectPushConstants(module, shader_module.reflection);
    ReflectEntryPoints(module);

    for (auto& bgp : shader->buffer_groups)
    {
        auto buffer_group = bgp.first;
        ReflectAndPrepareBuffers(buffer_group, module, shader, shader_module.reflection);
    }

    // PrintShaderReflection(shader);

    std::cout << "Reflection Complete for Stage" << std::endl;
}


bool CreateDescriptorSetLayouts(VkDevice device, Shader* shader)
{
    std::map<uint32_t, std::vector<VkDescriptorSetLayoutBinding>> set_bindings;

    // 1. Aggregate bindings from all shader stages
    for (auto const& [stage, module] : shader->module_map)
    {
        SpvReflectShaderModule* reflect_module = module.reflection.module_ptr.get();

        for (uint32_t i = 0; i < reflect_module->descriptor_binding_count; ++i)
        {
            const SpvReflectDescriptorBinding& binding_info = reflect_module->descriptor_bindings[i];

            VkDescriptorSetLayoutBinding layout_binding{};
            layout_binding.binding = binding_info.binding;
            layout_binding.descriptorType = static_cast<VkDescriptorType>(binding_info.descriptor_type);
            layout_binding.descriptorCount = binding_info.count;
            // The stageFlags should be OR'd for bindings that appear in multiple stages.
            // For simplicity here, we get the stage from the module type.
            // A more robust system might map SpvExecutionModel to VkShaderStageFlagBits.
            VkShaderStageFlags vk_stage = 0;
            switch(stage) {
                case ShaderModuleType::Vertex:   vk_stage = VK_SHADER_STAGE_VERTEX_BIT;   break;
                case ShaderModuleType::Fragment: vk_stage = VK_SHADER_STAGE_FRAGMENT_BIT; break;
                case ShaderModuleType::Compute:  vk_stage = VK_SHADER_STAGE_COMPUTE_BIT;  break;
                // ... add other stages
            }
            layout_binding.stageFlags = vk_stage;
            layout_binding.pImmutableSamplers = nullptr;

            // Add the binding to the correct set
            set_bindings[binding_info.set].push_back(layout_binding);
        }
    }

    // 2. Create a VkDescriptorSetLayout for each set
    for (auto const& [set_num, bindings] : set_bindings)
    {
        VkDescriptorSetLayoutCreateInfo layout_info{};
        layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
        layout_info.pBindings = bindings.data();

        VkDescriptorSetLayout layout;
        if (vkCreateDescriptorSetLayout(device, &layout_info, nullptr, &layout) != VK_SUCCESS)
        {
            std::cerr << "Failed to create descriptor set layout for set " << set_num << std::endl;
            // Cleanup previously created layouts before returning
            for(auto const& [num, l] : shader->descriptor_set_layouts) {
                vkDestroyDescriptorSetLayout(device, l, nullptr);
            }
            shader->descriptor_set_layouts.clear();
            return false;
        }
        shader->descriptor_set_layouts[set_num] = layout;
        std::cout << "Successfully created descriptor set layout for set " << set_num << std::endl;
    }

    return true;
}

bool CreateDescriptorPool(VkDevice device, Shader* shader, uint32_t max_sets_per_pool)
{
    std::map<VkDescriptorType, uint32_t> descriptor_counts;

    // 1. Aggregate descriptor counts from all shader modules
    for (auto const& [stage, module] : shader->module_map)
    {
        SpvReflectShaderModule* reflect_module = module.reflection.module_ptr.get();
        for (uint32_t i = 0; i < reflect_module->descriptor_binding_count; ++i)
        {
            const SpvReflectDescriptorBinding& binding_info = reflect_module->descriptor_bindings[i];
            VkDescriptorType type = static_cast<VkDescriptorType>(binding_info.descriptor_type);
            descriptor_counts[type] += binding_info.count;
        }
    }

    if (descriptor_counts.empty()) {
        std::cout << "No descriptors found in shader, no pool to create." << std::endl;
        return true; // Not an error, just nothing to do.
    }

    // 2. Create VkDescriptorPoolSize structures
    std::vector<VkDescriptorPoolSize> pool_sizes;
    for (auto const& [type, count] : descriptor_counts)
    {
        // We multiply by max_sets_per_pool to allow for multiple sets of this type to be allocated.
        pool_sizes.push_back({type, count * max_sets_per_pool});
    }

    // 3. Create the descriptor pool
    VkDescriptorPoolCreateInfo pool_info{};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
    pool_info.pPoolSizes = pool_sizes.data();
    pool_info.maxSets = max_sets_per_pool * static_cast<uint32_t>(shader->descriptor_set_layouts.size());
     // You might want VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT if you need to free individual sets
    pool_info.flags = 0;

    if (vkCreateDescriptorPool(device, &pool_info, nullptr, &shader->descriptor_pool) != VK_SUCCESS)
    {
        std::cerr << "Failed to create descriptor pool." << std::endl;
        return false;
    }

    std::cout << "Successfully created descriptor pool." << std::endl;
    return true;
}

bool AllocateDescriptorSets(VkDevice device, Shader* shader)
{
    if (shader->descriptor_set_layouts.empty()) {
        return true; // Nothing to allocate
    }

    for (auto const& [set_num, layout] : shader->descriptor_set_layouts)
    {
        VkDescriptorSetAllocateInfo alloc_info{};
        alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        alloc_info.descriptorPool = shader->descriptor_pool;
        alloc_info.descriptorSetCount = 1;
        alloc_info.pSetLayouts = &layout;

        VkDescriptorSet descriptor_set;
        if (vkAllocateDescriptorSets(device, &alloc_info, &descriptor_set) != VK_SUCCESS)
        {
            std::cerr << "Failed to allocate descriptor set for set " << set_num << std::endl;
            return false;
        }
        shader->descriptor_sets[set_num] = descriptor_set;
        std::cout << "Successfully allocated descriptor set for set " << set_num << std::endl;
    }
    return true;
}

VkImageLayout infer_image_layout(Shader* shader, std::unordered_map<Shader*, VkDescriptorType>& types)
{
    auto& type = types[shader];
    switch (type)
    {
        case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
        case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
        case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            return VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

        case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            return VK_IMAGE_LAYOUT_GENERAL;

        default:
            return VK_IMAGE_LAYOUT_UNDEFINED; // or assert
    }
}

/**
 * @brief The core creation function. It iterates the prepared ShaderBuffer map, creates the
 * actual Vulkan buffers, copies initial data, and performs the shared_ptr swap.
 */
bool CreateAndBindShaderBuffers(BufferGroup* buffer_group, Shader* shader)
{
	auto direct_registry = get_direct_registry();
    std::vector<VkWriteDescriptorSet> descriptor_writes;
    std::vector<VkDescriptorBufferInfo> buffer_infos; 
    std::vector<VkDescriptorImageInfo> image_infos;

    for (auto& [name, buffer] : buffer_group->buffers) {
        if (buffer.gpu_buffer.mapped_memory)
            continue;

        VkDeviceSize buffer_size = 0;
        if (buffer.is_dynamic_sized) {
            if (buffer.element_count == 0) {
                 std::cout << "Skipping dynamic buffer '" << name << "' as its element count was not set by the application." << std::endl;
                 continue;
            }
            buffer_size = buffer.element_count * buffer.element_stride;
        } else {
            buffer_size = buffer.static_size;
        }

        if (buffer_size == 0) {
            std::cerr << "Warning: Skipping buffer '" << name << "' with zero size." << std::endl;
            continue;
        }

        // Create the GpuBuffer
        VkBufferUsageFlags usage = (buffer.descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
            : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        
        VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = buffer_size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(direct_registry->device, &buffer_info, nullptr, &buffer.gpu_buffer.buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer for " + name);
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(direct_registry->device, buffer.gpu_buffer.buffer, &mem_reqs);
        
        VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc_info.allocationSize = mem_reqs.size;
        alloc_info.memoryTypeIndex = FindMemoryType(direct_registry->physicalDevice, mem_reqs.memoryTypeBits, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

        if (vkAllocateMemory(direct_registry->device, &alloc_info, nullptr, &buffer.gpu_buffer.memory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate memory for buffer " + name);
        }
        vkBindBufferMemory(direct_registry->device, buffer.gpu_buffer.buffer, buffer.gpu_buffer.memory, 0);

        // Persistently map the memory
        vkMapMemory(direct_registry->device, buffer.gpu_buffer.memory, 0, buffer_size, 0, &buffer.gpu_buffer.mapped_memory);
        buffer.gpu_buffer.size = buffer_size;

        // --- Data Upload and Pointer Swap ---
        // Ensure data_ptr exists for fixed-size buffers if user wants to get it later
        if (!buffer.data_ptr) {
            buffer.data_ptr = buffer_group_get_buffer_data_ptr(buffer_group, name);
        }

        // Copy from CPU staging pointer to mapped GPU pointer
        if (buffer.data_ptr) {
            memcpy(buffer.gpu_buffer.mapped_memory, buffer.data_ptr.get(), buffer_size);
            std::cout << "Copied initial data to GPU for buffer '" << name << "'." << std::endl;
        }
        
        // ** THE MAGIC **
        // Reset the shared_ptr to point to the GPU mapped memory.
        // The custom empty deleter prevents the shared_ptr from trying to free Vulkan's memory.
        buffer.data_ptr.reset(static_cast<uint8_t*>(buffer.gpu_buffer.mapped_memory), [](uint8_t*){ /* Do nothing */ });
        std::cout << "Remapped data_ptr for '" << name << "' to point directly to GPU memory." << std::endl;
    }
    for (auto& [name, buffer] : buffer_group->buffers) {
        // Prepare the descriptor set write
        buffer_infos.emplace_back();
        buffer_infos.back().buffer = buffer.gpu_buffer.buffer;
        buffer_infos.back().offset = 0;
        buffer_infos.back().range = buffer.gpu_buffer.size;
    }
    
    size_t i = 0;
    for (auto& [name, buffer] : buffer_group->buffers) {
        VkWriteDescriptorSet write_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write_set.dstSet = shader->descriptor_sets.at(buffer.set);
        write_set.dstBinding = buffer.binding;
        write_set.dstArrayElement = 0;
        write_set.descriptorType = buffer.descriptor_type;
        write_set.descriptorCount = 1;
        write_set.pBufferInfo = &buffer_infos[i];
        i++;
        descriptor_writes.push_back(write_set);
    }

    for (auto& [name, image_ref] : buffer_group->images) {
        auto image_it = buffer_group->runtime_images.find(name);
        if (image_it == buffer_group->runtime_images.end())
        {
            std::cerr << "Warning: Buffer group has not image defined for key: " << name << std::endl;
            continue;
        }
        Image* img = image_it->second.get();
        image_infos.emplace_back();
        image_infos.back().imageView = img->imageView;
        image_infos.back().imageLayout = infer_image_layout(shader, image_ref.descriptor_types);
        image_infos.back().sampler = img->sampler; // Use a real sampler if necessary
    }

    size_t j = 0;
    for (auto& [name, image_ref] : buffer_group->images) {
        VkWriteDescriptorSet write_set{VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
        write_set.dstSet = shader->descriptor_sets.at(image_ref.set);
        write_set.dstBinding = image_ref.binding;
        write_set.dstArrayElement = 0;
        auto type = image_ref.descriptor_types[shader];
        write_set.descriptorType = type;
        write_set.descriptorCount = 1;
        write_set.pImageInfo = &image_infos[j];
        j++;
        descriptor_writes.push_back(write_set);
    }
    
    if (!descriptor_writes.empty()) {
        vkUpdateDescriptorSets(direct_registry->device, descriptor_writes.size(), descriptor_writes.data(), 0, nullptr);
    }

    return true;
}


/**
 * @brief Updates all descriptor sets based on the current state of shader buffers.
 * This should be called after buffer data has been written to and needs to be
 * made visible to the shader via descriptor updates.
 *
 * @param shader The shader object.
 */
void shader_update_descriptor_sets(Shader* shader) {
    // std::vector<VkWriteDescriptorSet> writes;
    // std::vector<VkDescriptorBufferInfo> buffer_infos; // Store these so pointers remain valid

    // // Reserve space to avoid reallocations
    // writes.reserve(shader->current_buffer_group->buffers.size());
    // buffer_infos.reserve(shader->current_buffer_group->buffers.size());

    // for (const auto& pair : shader->current_buffer_group->buffers) {
    //     const auto& buffer = pair.second;

    //     // Ensure the descriptor set exists for this binding's set
    //     auto dst_set_it = shader->descriptor_sets.find(buffer.set);
    //     if (dst_set_it == shader->descriptor_sets.end()) {
    //         std::cerr << "Warning: No descriptor set found for set " << buffer.set
    //                   << " for buffer '" << buffer.name << "'. Skipping update." << std::endl;
    //         continue;
    //     }
    //     VkDescriptorSet dst_set = dst_set_it->second;

    //     // Calculate the range for the descriptor info
    //     VkDeviceSize range = buffer.is_dynamic_sized ?
    //                          (buffer.element_count * buffer.element_stride) :
    //                          buffer.static_size;

    //     // Ensure a non-zero range, especially for mock buffers that might start with 0 size
    //     if (range == 0) {
    //          std::cerr << "Warning: Buffer '" << buffer.name << "' has a range of 0. Skipping descriptor update." << std::endl;
    //          continue;
    //     }

    //     // Create VkDescriptorBufferInfo
    //     VkDescriptorBufferInfo buffer_info{};
    //     buffer_info.buffer = buffer.gpu_buffer.buffer;
    //     buffer_info.offset = 0; // Assuming the entire buffer is described by this descriptor
    //     buffer_info.range = range;
    //     buffer_infos.push_back(buffer_info); // Add to vector to keep it alive

    //     // Create VkWriteDescriptorSet
    //     VkWriteDescriptorSet write{};
    //     write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    //     write.dstSet = dst_set;
    //     write.dstBinding = buffer.binding;
    //     write.dstArrayElement = 0; // Not using array of descriptors here
    //     write.descriptorCount = 1;
    //     write.descriptorType = buffer.descriptor_type;
    //     write.pBufferInfo = &buffer_infos.back(); // Point to the last added buffer_info

    //     writes.push_back(write);
    // }

    // if (!writes.empty()) {
	//     auto direct_registry = get_direct_registry();
    //     // Call the Vulkan API function (mocked here)
    //     vkUpdateDescriptorSets(direct_registry->device,
    //                            static_cast<uint32_t>(writes.size()),
    //                            writes.data(),
    //                            0, nullptr); // No descriptor copies in this case
    //     std::cout << "Successfully updated " << writes.size() << " descriptor sets." << std::endl;
    // } else {
    //     std::cout << "No shader buffers found to update descriptor sets for." << std::endl;
    // }
}

void shader_initialize(Shader* shader)
{
    if (shader->initialized)
        return;
    shader_create_resources(shader);
    shader_compile(shader);
    shader->initialized = true;
}

void shader_create_resources(Shader* shader)
{
	auto direct_registry = get_direct_registry();
    auto& device = direct_registry->device;

    if (!CreateDescriptorSetLayouts(device, shader)) return;
    if (!CreateDescriptorPool(device, shader, 10)) return; // Max 10 sets of each type
    if (!AllocateDescriptorSets(device, shader)) return;

    std::cout << "Shader resources created and bound successfully using data-driven approach." << std::endl;
}

void shader_set_define(Shader* shader, const std::string& key, const std::string& value)
{
    shader->define_map.emplace(key, value);
}

void shader_init_module(Shader* shader, ShaderModule& shader_module);

size_t find_newline_after_token(const std::string& input, const std::string& token)
{
    // Find the first occurrence of the token in the string
    size_t token_pos = input.find(token);

    // If token is not found, return std::string::npos as an indicator of failure
    if (token_pos == std::string::npos)
    {
        return std::string::npos;
    }

    // Calculate the position after the token ends to start searching for newline from there
    size_t search_start = token_pos + token.length();

    // Find the first occurrence of newline '\n' after the token position
    size_t newline_pos = input.find('\n', search_start);

    // Return the index of the newline (or npos if not found)
    return newline_pos;
}

std::string addLineNumbers(const std::string &input)
{
    std::stringstream inputStream(input);
    std::stringstream outputStream;
    std::string line;
    std::size_t lineIndex = 0;

    while (std::getline(inputStream, line))
    {
        outputStream << std::setw(4) << std::setfill(' ') << lineIndex << ": " << line << "\n";
        ++lineIndex;
    }

    // If input ends with a newline, preserve that
    if (!input.empty() && input.back() == '\n' && (input.length() == 1 || input[input.length() - 2] != '\r'))
    {
        // Remove the last newline and add an empty line with a line number
        if (input.back() == '\n' && input[input.length() - 2] != '\n')
        {
            outputStream << std::setw(4) << std::setfill(' ') << lineIndex << ": \n";
        }
    }

    return outputStream.str();
}

void shader_add_module(
    Shader* shader,
    ShaderModuleType module_type,
    const std::string& glsl_source
)
{
	if (!shader)
		throw std::runtime_error("shader is null");
    
    auto shaderString = glsl_source;
    auto after_version_idx = find_newline_after_token(shaderString, "#version") + 1;
    for (auto& define_pair : shader->define_map)
    {
        std::string defineString("#define ");
        defineString += define_pair.first + " " + define_pair.second + "\n";
        auto next_it = shaderString.insert(shaderString.begin() + after_version_idx, defineString.begin(), defineString.end());
        after_version_idx = std::distance(shaderString.begin(), next_it + defineString.size());
    }
    
	shaderc::Compiler compiler;
	shaderc::CompileOptions compileOptions;

    std::unique_ptr<DynamicIncluder> includer = std::make_unique<DynamicIncluder>(shader);
    compileOptions.SetIncluder(std::move(includer));

	auto stage = stageEShaderc[module_type];

    // auto pre_result = compiler.PreprocessGlsl(shaderString.c_str(), stage, "Shader", compileOptions);
    // std::string preprocessed(pre_result.begin(), pre_result.end());
    // std::cout << "--- Preprocessed Shader ---\n" << preprocessed << "\n";

	shaderc::SpvCompilationResult compiled_module = compiler.CompileGlslToSpv(
        shaderString.c_str(),
        shaderString.size(),
        stage,
        "main",
        compileOptions
    );
    auto status = compiled_module.GetCompilationStatus();
    if (status != shaderc_compilation_status_success)
    {
        std::cerr << "Shader Source:" << std::endl << std::endl << addLineNumbers(shaderString) << std::endl;
        std::cerr << "Shader Compile Error: " << compiled_module.GetErrorMessage() << std::endl;
        return;
    }
	auto& shader_module = shader->module_map[module_type];
	shader_module.spirv_vec = {compiled_module.cbegin(), compiled_module.cend()};
    shader_module.type = module_type;
    shader_init_module(shader, shader_module);
	shader_reflect(shader, module_type, stage);
	return;
}

void shader_add_module_from_file(
    Shader* shader,
    const std::filesystem::path& file_path
)
{
    
}

void shader_init_module(Shader* shader, ShaderModule& shader_module)
{
    VkShaderModuleCreateInfo create_info{};
	create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	create_info.codeSize = 4 * shader_module.spirv_vec.size();
	create_info.pCode = shader_module.spirv_vec.data();
	auto direct_registry = get_direct_registry();
    vkCreateShaderModule(direct_registry->device, &create_info, 0, &shader_module.vk_module);
}

void shader_add_buffer_group(Shader* shader, BufferGroup* buffer_group)
{
    buffer_group->shaders[shader] = true;
    shader->buffer_groups[buffer_group] = true;
}

void shader_remove_buffer_group(Shader* shader, BufferGroup* buffer_group)
{
    buffer_group->shaders.erase(shader);
    shader->buffer_groups.erase(buffer_group);
}

struct TransitionInfo
{
    VkAccessFlags srcAccessMask;
    VkAccessFlags dstAccessMask;
    VkPipelineStageFlags srcStage;
    VkPipelineStageFlags dstStage;
};

const std::map<std::pair<VkImageLayout, VkImageLayout>, TransitionInfo> kLayoutTransitions =
{
    // Undefined to Usable Layouts
    {{VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL}, {0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT}},
    {{VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_GENERAL}, {0, VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT}},
    {{VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, {0, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT}},
    {{VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL}, {0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}},
    {{VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL}, {0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT}},
    
    // Transfer Write to Shader Read
    {{VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, {VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT}},

    // General Compute to Shader Read
    {{VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, {VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT}},

    // Shader Read to General Write
    {{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_GENERAL}, {VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_SHADER_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT}},

    // General to Transfer Destination
    {{VK_IMAGE_LAYOUT_GENERAL, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL}, {VK_ACCESS_SHADER_WRITE_BIT, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT}},

    // Depth Read/Write Variants
    {{VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL}, {0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT}},
    {{VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL}, {0, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT}},
    
    // Present to Shader Read (e.g., post-processing)
    {{VK_IMAGE_LAYOUT_PRESENT_SRC_KHR, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, {VK_ACCESS_MEMORY_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT}},
    
    // Feedback loop
    {{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT}, {VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT}},
    {{VK_IMAGE_LAYOUT_ATTACHMENT_FEEDBACK_LOOP_OPTIMAL_EXT, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, {VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT}},

    // Read/Write only compatibility
    {{VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL}, {VK_ACCESS_SHADER_READ_BIT, VK_ACCESS_TRANSFER_READ_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT}},
    {{VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL}, {VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT}},
};

void transition_image_layout(VkImage image, VkImageLayout old_layout, VkImageLayout new_layout)
{
    auto device = get_direct_registry()->device;
    auto command_buffer = begin_single_time_commands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = old_layout;
    barrier.newLayout = new_layout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    auto it = kLayoutTransitions.find({old_layout, new_layout});
    if (it == kLayoutTransitions.end())
    {
        throw std::runtime_error("Unsupported image layout transition!");
    }

    const TransitionInfo& info = it->second;
    barrier.srcAccessMask = info.srcAccessMask;
    barrier.dstAccessMask = info.dstAccessMask;

    vkCmdPipelineBarrier(
        command_buffer,
        info.srcStage,
        info.dstStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    end_single_time_commands(command_buffer);
}

void shader_ensure_image_layouts(Shader* shader)
{
    for (auto& bound_buffer_group : shader->bound_buffer_groups)
    {
        for (auto& [name, shader_image] : bound_buffer_group->images)
        {
            auto runtime_ite = bound_buffer_group->runtime_images.find(name);
            if (runtime_ite == bound_buffer_group->runtime_images.end())
                continue;

            auto& image = *runtime_ite->second;;
    
            auto exp_lay_ite = shader_image.expected_layouts.find(shader);
            if (exp_lay_ite == shader_image.expected_layouts.end())
                continue;

            auto required = exp_lay_ite->second;
    
            if (image.current_layout != required)
            {
                transition_image_layout(image.image, image.current_layout, required);
                image.current_layout = required;
            }
        }
    }
}

void shader_dispatch(Shader* shader, vec<int32_t, 3> dispatch_layout)
{
    shader_ensure_image_layouts(shader);
    auto direct_registry = get_direct_registry();
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    vkBeginCommandBuffer(direct_registry->computeCommandBuffer, &beginInfo);
    vkCmdBindPipeline(
        direct_registry->computeCommandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        shader->graphics_pipeline
    );
    std::vector<VkDescriptorSet> sets;
    sets.reserve(shader->descriptor_sets.size());
    for (auto& set_pair : shader->descriptor_sets)
    {
        sets.push_back(set_pair.second);
    }
    vkCmdBindDescriptorSets(
        direct_registry->computeCommandBuffer,
        VK_PIPELINE_BIND_POINT_COMPUTE,
        shader->pipeline_layout,
        0,
        sets.size(),
        sets.data(),
        0, nullptr
    );
    vkCmdDispatch(
        direct_registry->computeCommandBuffer,
        dispatch_layout[0],
        dispatch_layout[1],
        dispatch_layout[2]
    );
    vkEndCommandBuffer(direct_registry->computeCommandBuffer);
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &direct_registry->computeCommandBuffer;
    vkQueueSubmit(direct_registry->computeQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(direct_registry->computeQueue);
}

void shader_compile(Shader* shader)
{
    auto direct_registry = get_direct_registry();
    auto device = direct_registry->device;

    // --- Create Pipeline Layout ---
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;

    std::vector<VkDescriptorSetLayout> layouts;
    layouts.reserve(shader->descriptor_set_layouts.size());
    for (auto& layout : shader->descriptor_set_layouts)
    {
        layouts.push_back(layout.second);
    }

    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 0;
    pipelineLayoutInfo.pPushConstantRanges = nullptr;

    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &shader->pipeline_layout) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create pipeline layout!");
    }

    std::cout << "Created pipeline layout: " << shader->pipeline_layout << std::endl;

    // --- Determine Shader Stages ---
    std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
    bool requires_rasterization = false;
    bool uses_compute = false;

    for (auto& [module_type, shader_module] : shader->module_map)
    {
        VkPipelineShaderStageCreateInfo stage_info{};
        stage_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stage_info.stage = stageFlags[shader_module.type];
        stage_info.module = shader_module.vk_module;
        stage_info.pName = "main";

        switch (stage_info.stage)
        {
        case VK_SHADER_STAGE_VERTEX_BIT:
        case VK_SHADER_STAGE_FRAGMENT_BIT:
            requires_rasterization = true;
            break;
        case VK_SHADER_STAGE_COMPUTE_BIT:
            uses_compute = true;
            break;
        default:
            break;
        }

        shaderStages.push_back(stage_info);
    }

    if (uses_compute && requires_rasterization)
    {
        throw std::runtime_error("Cannot mix compute and raster shader stages in one pipeline");
    }

    // --- Create Compute Pipeline ---
    if (uses_compute)
    {
        if (shaderStages.size() != 1 || shaderStages[0].stage != VK_SHADER_STAGE_COMPUTE_BIT)
        {
            throw std::runtime_error("Compute pipelines must have exactly one compute shader stage");
        }

        VkComputePipelineCreateInfo computeInfo{};
        computeInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        computeInfo.stage = shaderStages[0];
        computeInfo.layout = shader->pipeline_layout;
        computeInfo.basePipelineHandle = VK_NULL_HANDLE;
        computeInfo.basePipelineIndex = -1;

        if (vkCreateComputePipelines(device, VK_NULL_HANDLE, 1, &computeInfo, nullptr, &shader->graphics_pipeline) != VK_SUCCESS)
        {
            throw std::runtime_error("Failed to create compute pipeline!");
        }

        std::cout << "Created compute pipeline: " << shader->graphics_pipeline << std::endl;
        return;
    }

    // --- Setup Graphics Pipeline States ---
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport dummyViewport{};
    dummyViewport.x = 0.0f;
    dummyViewport.y = 0.0f;
    dummyViewport.width = 1.0f;
    dummyViewport.height = 1.0f;
    dummyViewport.minDepth = 0.0f;
    dummyViewport.maxDepth = 1.0f;

    VkRect2D dummyScissor{};
    dummyScissor.offset = {0, 0};
    dummyScissor.extent = {1, 1};

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &dummyViewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &dummyScissor;

    std::vector<VkDynamicState> dynamicStates =
    {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.sampleShadingEnable = VK_FALSE;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.blendEnable = VK_FALSE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
        VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;

    // --- Create Graphics Pipeline ---
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
    pipelineInfo.pStages = shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = shader->pipeline_layout;
    pipelineInfo.renderPass = direct_registry->surfaceRenderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &shader->graphics_pipeline) != VK_SUCCESS)
    {
        throw std::runtime_error("Failed to create graphics pipeline!");
    }

    std::cout << "Created graphics pipeline: " << shader->graphics_pipeline << std::endl;
}

void shader_bind(Shader* shader)
{
    if (shader->graphics_pipeline == VK_NULL_HANDLE)
    {
        std::cerr << "Error: Attempted to bind a null graphics pipeline." << std::endl;
        return;
    }
    auto direct_registry = get_direct_registry();
    vkCmdBindPipeline(
        *direct_registry->commandBuffer,
        VK_PIPELINE_BIND_POINT_GRAPHICS,
        shader->graphics_pipeline
    );
}

void renderer_render(Renderer* renderer)
{
    auto direct_registry = get_direct_registry();
    pre_begin_render_pass(renderer);
    begin_render_pass(renderer);
    auto& window = *renderer->window;
    vkCmdSetViewport(*direct_registry->commandBuffer, 0, 1, &window.viewport);
    vkCmdSetScissor(*direct_registry->commandBuffer, 0, 1, &window.scissor);
    for (auto& draw_mgr_group_vec_pair : window.draw_list_managers)
    {
        auto& draw_mgr = *draw_mgr_group_vec_pair.first;
        auto& group_vec = draw_mgr_group_vec_pair.second;
        for (auto& buffer_group : group_vec)
        {
            auto shaderDrawList = draw_mgr.genDrawList(buffer_group);
            for (auto& shader_pair : shaderDrawList)
            {
                auto shader = shader_pair.first;
                auto& draw_list = shader_pair.second;
                shader_ensure_image_layouts(shader);
                shader_bind(shader);
			    renderer_draw_commands(renderer, shader, draw_list);
            }
        }
    }
    post_render_pass(renderer);
    swap_buffers(renderer);
}

void renderer_draw_commands(Renderer* renderer, Shader* shader, const std::vector<DrawIndirectCommand>& commands)
{
	auto direct_registry = get_direct_registry();
	auto drawsSize = commands.size();
	auto drawBufferSize = sizeof(VkDrawIndirectCommand) * drawsSize;
    auto buffer_hash = drawBufferSize ^ size_t(shader) << 2;
	VkCommandBuffer passCB = *direct_registry->commandBuffer;

	auto& drawBufferPair = renderer->drawBuffers[buffer_hash];
	if (!drawBufferPair.first)
	{
		createBuffer(
			renderer,
			drawBufferSize,
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			drawBufferPair.first,
			drawBufferPair.second
		);
	}

	auto& countBufferPair = renderer->countBuffers[buffer_hash];
	if (!countBufferPair.first)
	{
		createBuffer(
			renderer,
			sizeof(uint32_t),
			VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			countBufferPair.first,
			countBufferPair.second
		);
	}

	std::vector<VkDrawIndirectCommand> i_commands(drawsSize, VkDrawIndirectCommand{});
	memcpy(i_commands.data(), commands.data(), i_commands.size() * sizeof(VkDrawIndirectCommand));

	// Write draw commands to GPU-visible indirect buffer
	void* drawBufferData;
	vkMapMemory(direct_registry->device, drawBufferPair.second, 0, VK_WHOLE_SIZE, 0, &drawBufferData);
	memcpy(drawBufferData, i_commands.data(), drawBufferSize);
	vkUnmapMemory(direct_registry->device, drawBufferPair.second);

	// Write draw count to count buffer
	uint32_t drawCount = static_cast<uint32_t>(drawsSize);
	void* countBufferData;
	vkMapMemory(direct_registry->device, countBufferPair.second, 0, VK_WHOLE_SIZE, 0, &countBufferData);
	memcpy(countBufferData, &drawCount, sizeof(uint32_t));
	vkUnmapMemory(direct_registry->device, countBufferPair.second);

	// Descriptor sets
	std::vector<VkDescriptorSet> sets;
	sets.reserve(shader->descriptor_sets.size());
	for (auto& set_pair : shader->descriptor_sets)
	{
		sets.push_back(set_pair.second);
	}

	vkCmdBindDescriptorSets(
		passCB,
		VK_PIPELINE_BIND_POINT_GRAPHICS,
		shader->pipeline_layout,
		0,
		sets.size(),
		sets.data(),
		0,
		nullptr
	);

	// Detect whether indirect count is supported
#ifdef __ANDROID__
	static constexpr bool supportsIndirectCount = false;
#else
    static constexpr bool supportsIndirectCount = true;
#endif

	if (supportsIndirectCount)
	{
#ifdef __ANDROID__
		vkCmdDrawIndirectCount(
			passCB,
			drawBufferPair.first,
			0,
			countBufferPair.first,
			0,
			drawCount,
			sizeof(VkDrawIndirectCommand)
		);
#endif
	}
	else
	{
		// Manually read count from mapped memory
		uint32_t fallbackDrawCount = 0;
		void* mappedCount = nullptr;
		vkMapMemory(direct_registry->device, countBufferPair.second, 0, sizeof(uint32_t), 0, &mappedCount);
		memcpy(&fallbackDrawCount, mappedCount, sizeof(uint32_t));
		vkUnmapMemory(direct_registry->device, countBufferPair.second);
		fallbackDrawCount = std::min(drawCount, fallbackDrawCount);

		for (uint32_t i = 0; i < fallbackDrawCount; ++i)
		{
			VkDeviceSize commandOffset = i * sizeof(VkDrawIndirectCommand);
			vkCmdDrawIndirect(passCB, drawBufferPair.first, commandOffset, 1, sizeof(VkDrawIndirectCommand));
		}
	}
}

void shader_destroy(Shader* shader)
{
	auto direct_registry = get_direct_registry();
    auto& device = direct_registry->device;
    for (auto& pair : shader->descriptor_set_layouts)
    {
        vkDestroyDescriptorSetLayout(device, pair.second, 0);
    }
    if (shader->descriptor_pool != VK_NULL_HANDLE)
        vkDestroyDescriptorPool(device, shader->descriptor_pool, 0);
    for (auto& modulePair : shader->module_map)
    {
        auto& module = modulePair.second;
        vkDestroyShaderModule(device, module.vk_module, 0);
    }
    if (shader->render_pass != VK_NULL_HANDLE)
        vkDestroyRenderPass(device, shader->render_pass, 0);
    if (shader->graphics_pipeline != VK_NULL_HANDLE)
        vkDestroyPipeline(device, shader->graphics_pipeline, 0);
    if (shader->pipeline_layout != VK_NULL_HANDLE)
        vkDestroyPipelineLayout(device, shader->pipeline_layout, 0);
}