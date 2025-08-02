#include <dz/BufferGroup.hpp>
#include <memory>
#include "BufferGroup.cpp.hpp"
#include "Directz.cpp.hpp"
#include "Shader.cpp.hpp"

namespace dz {
    BufferGroup* buffer_group_create(const std::string& group_name) {
        auto& bg = (dr.buffer_groups[group_name] = std::shared_ptr<BufferGroup>(
            new BufferGroup{
                .group_name = group_name
            },
            [](BufferGroup* bg) {
                buffer_group_destroy(bg);
                delete bg;
            }
        ));
        return bg.get();
    }

    void buffer_group_initialize(BufferGroup* buffer_group) {
        for (auto& sp : buffer_group->shaders) {
            auto shader = sp.first;
            shader->bound_buffer_groups.push_back(buffer_group);
            shader_initialize(shader);
            CreateAndBindShaderBuffers(buffer_group, shader);
            // shader_update_descriptor_sets(shader);
        }
    }

    VkImageUsageFlags infer_image_usage_flags(const std::unordered_map<Shader*, VkDescriptorType>& types) {
        VkImageUsageFlags flags = 0;
        for (auto& pair : types) {
            auto& type = pair.second;
            switch (type) {
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                    flags |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    flags |= VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        
                case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                    flags |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        
                default:
                    flags |= VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT; // fallback
            }
        }
        return flags;
    }

    Image* buffer_group_define_image_2D(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t image_width, uint32_t image_height, void* data_pointer) {
        ShaderImage& shader_image = buffer_group->images[buffer_name];

        ImageCreateInfoInternal create_info{};
        create_info.width = image_width;
        create_info.height = image_height;
        create_info.depth = 1;
        create_info.format = shader_image.format;
        create_info.image_type = VK_IMAGE_TYPE_2D;
        create_info.view_type = VK_IMAGE_VIEW_TYPE_2D;
        create_info.data = data_pointer;
        create_info.usage = infer_image_usage_flags(shader_image.descriptor_types);

        auto image = image_create_internal(create_info);

        buffer_group->runtime_images[buffer_name] = std::shared_ptr<Image>(image, image_free);

        return image;
    }

    Image* buffer_group_define_image_3D(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t image_width, uint32_t image_height, uint32_t image_depth, void* data_pointer) {
        ShaderImage& shader_image = buffer_group->images[buffer_name];

        ImageCreateInfoInternal create_info{
            .width = image_width,
            .height = image_height,
            .depth = image_depth,
            .format = shader_image.format,
            .usage = infer_image_usage_flags(shader_image.descriptor_types),
            .image_type = VK_IMAGE_TYPE_3D,
            .view_type = VK_IMAGE_VIEW_TYPE_3D,
            .data = data_pointer
        };

        auto image = image_create_internal(create_info);

        buffer_group->runtime_images[buffer_name] = std::shared_ptr<Image>(image, image_free);

        return image;
    }

    void buffer_group_restrict_to_keys(BufferGroup* buffer_group, const std::vector<std::string>& restruct_keys) {
        for (auto& key : restruct_keys)
            buffer_group->restricted_to_keys[key] = true;
    }

    /**
    * @brief For dynamic SSBOs, sets the number of elements the buffer should hold.
    * This MUST be called before shader_create_resources().
    * This function also allocates the initial CPU-side staging buffer.
    *
    * @param shader The shader object.
    * @param buffer_name The GLSL variable name of the buffer.
    * @param element_count The number of elements to allocate space for.
    */
    void buffer_group_set_buffer_element_count(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t element_count) {
        if (buffer_group->buffers.find(buffer_name) == buffer_group->buffers.end()) {
            std::cerr << "Warning: Cannot set element count for buffer '" << buffer_name << "'. It was not found in reflection." << std::endl;
            return;
        }

        auto& buffer = buffer_group->buffers.at(buffer_name);
        if (!buffer.is_dynamic_sized) {
            std::cerr << "Warning: Buffer '" << buffer_name << "' is not a dynamic, runtime-sized buffer." << std::endl;
            return;
        }

        auto old_element_count = buffer.element_count;
        auto old_data_ptr = buffer.data_ptr;

        buffer.element_count = element_count;
        size_t old_size = old_element_count * buffer.element_stride;
        size_t new_size = buffer.element_count * buffer.element_stride;

        if (buffer.data_ptr && !buffer.gpu_buffer.mapped_memory) {
            auto new_buffer = std::shared_ptr<uint8_t>(new uint8_t[new_size], std::default_delete<uint8_t[]>());
            memset(new_buffer.get(), 0, new_size);
            memcpy(new_buffer.get(), buffer.data_ptr.get(), (std::min)(old_size, new_size));
            buffer.data_ptr = new_buffer;
        }
        else if (!buffer.gpu_buffer.mapped_memory) {
            // Allocate the initial CPU-side buffer. Use a custom deleter for array `new[]`.
            buffer.data_ptr = std::shared_ptr<uint8_t>(new uint8_t[new_size], std::default_delete<uint8_t[]>());
            memset(buffer.data_ptr.get(), 0, new_size);
        
            if (old_element_count && old_data_ptr) {
                auto copy_size = std::min(old_element_count, element_count);
                memcpy(buffer.data_ptr.get(), old_data_ptr.get(), copy_size);
                std::cout << "Resized dynamic CPU buffer '" << buffer_name << "' to hold " << element_count << " elements (" << new_size << " bytes). CPU staging buffer created." << std::endl;
            }
            else {
                std::cout << "Set dynamic CPU buffer '" << buffer_name << "' to hold " << element_count << " elements (" << new_size << " bytes). CPU staging buffer created." << std::endl;
            }
        }
        else {
            buffer_group_resize_gpu_buffer(buffer_name, buffer);
            for (auto& [shader, _] : buffer_group->shaders)
                shader_update_descriptor_sets(shader);
        }
    }

    uint32_t buffer_group_get_buffer_element_count(BufferGroup* buffer_group, const std::string& buffer_name) {
        if (buffer_group->buffers.find(buffer_name) == buffer_group->buffers.end()) {
            throw std::runtime_error("Warning: Cannot get element count for buffer '" + buffer_name + "'. It was not found in reflection.");
        }

        auto& buffer = buffer_group->buffers.at(buffer_name);

        return buffer.element_count;
    }
    
    uint32_t buffer_group_get_buffer_element_size(BufferGroup* buffer_group, const std::string& buffer_name) {
        if (buffer_group->buffers.find(buffer_name) == buffer_group->buffers.end()) {
            std::cerr << "Warning: Cannot get element size for buffer '" << buffer_name << "'. It was not found in reflection." << std::endl;
            return 0;
        }

        auto& buffer = buffer_group->buffers.at(buffer_name);
        if (!buffer.is_dynamic_sized) {
            std::cerr << "Warning: Buffer '" << buffer_name << "' is not a dynamic, runtime-sized buffer." << std::endl;
            return 0;
        }

        return buffer.element_stride;
    }

    /**
    * @brief Gets the shared pointer to the buffer's data.
    * Initially, this points to a CPU-side buffer. After resource creation, it will point
    * directly to the mapped GPU memory. The application can use this pointer to update data.
    *
    * @param shader The shader object.
    * @param buffer_name The GLSL variable name of the buffer.
    * @return A shared_ptr to the raw buffer data.
    */
    std::shared_ptr<uint8_t> buffer_group_get_buffer_data_ptr(BufferGroup* buffer_group, const std::string& buffer_name) {
        if (buffer_group->buffers.find(buffer_name) == buffer_group->buffers.end()) {
            std::cerr << "Error: Buffer '" << buffer_name << "' not found." << std::endl;
            return nullptr;
        }
        auto& buffer = buffer_group->buffers.at(buffer_name);

        // If this is a fixed-size buffer and the data hasn't been allocated yet, do it now.
        if (!buffer.data_ptr && !buffer.is_dynamic_sized) {
            buffer.data_ptr = std::shared_ptr<uint8_t>(new uint8_t[buffer.static_size], std::default_delete<uint8_t[]>());
        }

        return buffer.data_ptr;
    }

    /**
    * @brief Gets a reflected view of a specific struct element within a shader buffer.
    * This view allows updating individual members of the struct by name, handling
    * memory layout differences (padding, alignment) automatically.
    *
    * This function is intended for buffers whose `element_type` is a struct (e.g., UBOs, SSBOs of structs).
    *
    * @param shader The shader object.
    * @param buffer_name The GLSL variable name of the buffer (e.g., "ubo_scene", "particles").
    * @param index The 0-based index of the element to view (for SSBOs). For UBOs, this is usually 0.
    * @return A ReflectedStructView object.
    * @throws std::runtime_error if the buffer/element is not found, data_ptr is null,
    * index is out of bounds, or the element type is not a struct.
    */
    ReflectedStructView buffer_group_get_buffer_element_view(BufferGroup* buffer_group, const std::string& buffer_name, uint32_t index) {
        // Find the ShaderBuffer
        auto it = buffer_group->buffers.find(buffer_name);
        if (it == buffer_group->buffers.end()) {
            throw std::runtime_error("shader_get_buffer_element_view: Buffer '" + buffer_name + "' not found.");
        }
        ShaderBuffer& buffer = it->second;

        // Ensure data_ptr is valid (allocated CPU-side or mapped GPU-side)
        if (!buffer.data_ptr) {
            // If it's a fixed-size buffer and data isn't allocated, attempt to allocate it now.
            if (!buffer.is_dynamic_sized && buffer.static_size > 0) {
                buffer.data_ptr = std::shared_ptr<uint8_t>(new uint8_t[buffer.static_size], std::default_delete<uint8_t[]>());
                std::cout << "Info: Allocating CPU-side buffer for fixed-size buffer '" << buffer_name << "' on first view access." << std::endl;
            } else {
                throw std::runtime_error("shader_get_buffer_element_view: Buffer data_ptr is null for '" + buffer_name + "'. "
                                        "Ensure shader_set_buffer_element_count() was called for dynamic buffers, or it's mapped/allocated.");
            }
        }

        // Validate index for dynamic buffers
        if (index >= buffer.element_count) {
            throw std::out_of_range("shader_get_buffer_element_view: Index " + std::to_string(index) +
                                    " is out of bounds for buffer '" + buffer_name + "' (element count: " + std::to_string(buffer.element_count) + ").");
        }

        // Calculate the base address of the desired element within the buffer's data_ptr
        uint8_t* element_base_ptr = buffer.data_ptr.get() + (index * buffer.element_stride);

        // Get the ReflectedStruct definition for the element type
        if (buffer.element_type.type_kind != "struct") {
            throw std::runtime_error("shader_get_buffer_element_view: Buffer '" + buffer_name + "' element is not a struct type. Type kind: " + buffer.element_type.type_kind);
        }

        for (auto& shader_pair : buffer_group->shaders) {
            auto shader = shader_pair.first;
            for (auto& shaderModulePair : shader->module_map) {
                const ReflectedStruct& reflected_struct_def = getCanonicalStruct(shaderModulePair.second.reflection, buffer.element_type);
                // Return the ReflectedStructView
                return ReflectedStructView(element_base_ptr, reflected_struct_def);
            }
        }
        throw std::runtime_error("Could not get ReflectedStruct");
    }

    void buffer_group_destroy(BufferGroup* buffer_group) {
        auto& device = dr.device;
        if (device == VK_NULL_HANDLE)
            return;
        for (auto& bufferPair : buffer_group->buffers) {
            vkDestroyBuffer(device, bufferPair.second.gpu_buffer.buffer, 0);
            vkFreeMemory(device, bufferPair.second.gpu_buffer.memory, 0);
        }
    }

    VkDeviceSize ensure_buffer_size(const std::string& name, ShaderBuffer& buffer) {
        if (buffer.is_dynamic_sized) {
            if (buffer.element_count == 0) {
                    std::cout << "Skipping dynamic buffer '" << name << "' as its element count was not set by the application." << std::endl;
                    return 0;
            }
            return buffer.element_count * buffer.element_stride;
        } else {
            return buffer.static_size;
        }
    }

    void buffer_group_make_gpu_buffer(const std::string& name, ShaderBuffer& buffer) {
        VkDeviceSize buffer_size = ensure_buffer_size(name, buffer);

        if (buffer_size == 0) {
            std::cerr << "Warning: Skipping buffer '" << name << "' with zero size." << std::endl;
            return;
        }

        // Create the GpuBuffer
        VkBufferUsageFlags usage = (buffer.descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
            : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        
        VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = buffer_size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(dr.device, &buffer_info, nullptr, &buffer.gpu_buffer.buffer) != VK_SUCCESS) {
            throw std::runtime_error("Failed to create buffer for " + name);
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(dr.device, buffer.gpu_buffer.buffer, &mem_reqs);
        
        VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc_info.allocationSize = mem_reqs.size;
        alloc_info.memoryTypeIndex = FindMemoryType(dr.physicalDevice, mem_reqs.memoryTypeBits, 
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

        if (vkAllocateMemory(dr.device, &alloc_info, nullptr, &buffer.gpu_buffer.memory) != VK_SUCCESS) {
            throw std::runtime_error("Failed to allocate memory for buffer " + name);
        }
        vkBindBufferMemory(dr.device, buffer.gpu_buffer.buffer, buffer.gpu_buffer.memory, 0);

        // Persistently map the memory
        vkMapMemory(dr.device, buffer.gpu_buffer.memory, 0, buffer_size, 0, &buffer.gpu_buffer.mapped_memory);
        buffer.gpu_buffer.size = buffer_size;

        // Copy from CPU staging pointer to mapped GPU pointer
        if (buffer.data_ptr) {
            memcpy(buffer.gpu_buffer.mapped_memory, buffer.data_ptr.get(), buffer_size);
            std::cout << "Copied initial data to GPU for buffer '" << name << "'." << std::endl;
        }

        buffer.data_ptr.reset(static_cast<uint8_t*>(buffer.gpu_buffer.mapped_memory), [](uint8_t*){ /* Do nothing */ });
        std::cout << "Remapped data_ptr for '" << name << "' to point directly to GPU memory." << std::endl;
    }

    bool buffer_group_resize_gpu_buffer(const std::string& name, ShaderBuffer& buffer) {
        VkDeviceSize old_size = buffer.gpu_buffer.size;
        VkDeviceSize new_size = ensure_buffer_size(name, buffer);

        if (new_size == 0) {
            std::cerr << "Warning: Skipping resize of buffer '" << name << "' with zero size." << std::endl;
            return false;
        }

        if (new_size <= old_size) {
            std::cout << "No resize needed for buffer '" << name << "'." << std::endl;
            return false;
        }

        VkBufferUsageFlags usage = (buffer.descriptor_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER)
            ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT
            : VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

        VkBufferCreateInfo buffer_info{VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
        buffer_info.size = new_size;
        buffer_info.usage = usage;
        buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        VkBuffer new_buffer;
        if (vkCreateBuffer(dr.device, &buffer_info, nullptr, &new_buffer) != VK_SUCCESS) {
            std::cerr << "Failed to create new buffer for resize: " << name << std::endl;
            return false;
        }

        VkMemoryRequirements mem_reqs;
        vkGetBufferMemoryRequirements(dr.device, new_buffer, &mem_reqs);

        VkMemoryAllocateInfo alloc_info{VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
        alloc_info.allocationSize = mem_reqs.size;
        alloc_info.memoryTypeIndex = FindMemoryType(dr.physicalDevice, mem_reqs.memoryTypeBits,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT);

        VkDeviceMemory new_memory;
        if (vkAllocateMemory(dr.device, &alloc_info, nullptr, &new_memory) != VK_SUCCESS) {
            std::cerr << "Failed to allocate memory for new buffer: " << name << std::endl;
            vkDestroyBuffer(dr.device, new_buffer, nullptr);
            return false;
        }

        if (vkBindBufferMemory(dr.device, new_buffer, new_memory, 0) != VK_SUCCESS) {
            std::cerr << "Failed to bind new buffer memory: " << name << std::endl;
            vkDestroyBuffer(dr.device, new_buffer, nullptr);
            vkFreeMemory(dr.device, new_memory, nullptr);
            return false;
        }

        void* new_mapped_memory;
        if (vkMapMemory(dr.device, new_memory, 0, new_size, 0, &new_mapped_memory) != VK_SUCCESS) {
            std::cerr << "Failed to map new memory for buffer: " << name << std::endl;
            vkDestroyBuffer(dr.device, new_buffer, nullptr);
            vkFreeMemory(dr.device, new_memory, nullptr);
            return false;
        }

        if (buffer.gpu_buffer.mapped_memory && old_size > 0) {
            memcpy(new_mapped_memory, buffer.gpu_buffer.mapped_memory, (std::min)(old_size, new_size));
            std::cout << "Copied " << old_size << " bytes from old to new buffer for '" << name << "'." << std::endl;
        }

        vkUnmapMemory(dr.device, buffer.gpu_buffer.memory);
        vkDestroyBuffer(dr.device, buffer.gpu_buffer.buffer, nullptr);
        vkFreeMemory(dr.device, buffer.gpu_buffer.memory, nullptr);

        buffer.gpu_buffer.buffer = new_buffer;
        buffer.gpu_buffer.memory = new_memory;
        buffer.gpu_buffer.mapped_memory = new_mapped_memory;
        buffer.gpu_buffer.size = new_size;

        buffer.data_ptr.reset(static_cast<uint8_t*>(new_mapped_memory), [](uint8_t*){ /* Do nothing */ });
        std::cout << "Successfully resized GPU buffer for '" << name << "' to size " << new_size << "." << std::endl;

        return true;
    }

}