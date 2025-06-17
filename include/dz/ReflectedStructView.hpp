/**
 * @file ReflectedStructView.hpp
 * @brief A reflected view of a buffer offset allowing access to struct members 
 */
#pragma once
#include <string>
namespace dz {
    struct ReflectedStruct;
    class ReflectedStructView {
    public:
        /**
        * @brief Constructs a ReflectedStructView.
        * @param base_ptr A pointer to the start of the struct element's data in the buffer.
        * @param struct_def The ReflectedStruct definition for this element.
        */
        ReflectedStructView(uint8_t* base_ptr, const ReflectedStruct& struct_def);

        /**
        * @brief Sets the value of a member within the struct.
        * The data is copied to the correct offset in the underlying buffer memory,
        * respecting the shader's memory layout (padding and alignment).
        *
        * @tparam T The type of the value to set (e.g., float, int, glm::vec3, glm::mat4).
        * T must be trivially copyable.
        * @param member_name The name of the struct member (as defined in GLSL, e.g., "model", "color").
        * @param value The value to set.
        */
        template<typename T>
        void set_member(const std::string& member_name, const T& value)
        {
            set_member(member_name, &value, sizeof(value));
        }

        /**
        * @brief Overload to set a member from a raw pointer and size.
        * Useful for types like `glm::mat4` where `glm::value_ptr()` provides a raw `float*`,
        * or for C-style arrays.
        *
        * @param member_name The name of the struct member.
        * @param data_ptr A pointer to the source data.
        * @param data_size_bytes The size of the source data in bytes.
        */
        void set_member(const std::string& member_name, const void* data_ptr, size_t data_size_bytes);

        /**
        * @brief Gets the member as a modifiable value 
        */
        template <typename T>
        T& get_member(const std::string& member_name)
        {
            return *(T*)get_member(member_name, sizeof(T));
        }

        uint8_t* get_member(const std::string& member_name, size_t data_size_bytes);

        template <typename T>
        T& as_struct()
        {
            return *(T*)m_base_ptr;
        }
    private:
        uint8_t* m_base_ptr;          // Pointer to the start of the current struct element in memory
        const ReflectedStruct& m_struct_def; // Reference to the struct's reflection definition
    };
}