#pragma once

#include <string>
#include <vector>
#include <typeinfo>

enum class ComponentTypeHint {
    FLOAT,
    VEC4,
    VEC4_RGBA,
    MAT4,
    STRUCT
};

struct Reflectable {
    virtual ~Reflectable() = default;

    virtual const std::string& GetName() = 0;
    virtual ComponentTypeHint GetTypeHint() {
        return ComponentTypeHint::STRUCT;
    }

    virtual int GetPropertyIndexByName(const std::string& prop_name) = 0;
    virtual const std::vector<std::string>& GetPropertyNames() = 0;
    virtual void* GetVoidPropertyByIndex(int prop_index) = 0;
    virtual void* GetVoidPropertyByName(const std::string& prop_name) = 0;
    virtual const std::vector<const std::type_info*>& GetPropertyTypeinfos() = 0;
    template <typename T>
    T& GetPropertyByIndex(int prop_index) {
        return *(T*)GetVoidPropertyByIndex(prop_index);
    }
    template <typename T>
    T& GetPropertyByName(const std::string& prop_name) {
        return *(T*)GetVoidPropertyByName(prop_name);
    }
};