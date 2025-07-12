#pragma once

#include <string>
#include <vector>
#include <typeinfo>
#include "GlobalUID.hpp"

enum class ReflectableTypehint {
    FLOAT,
    VEC4,
    VEC4_RGBA,
    MAT4,
    STRUCT
};

struct Reflectable {
    virtual ~Reflectable() = default;

    virtual int GetID() = 0;
    virtual std::string& GetName() = 0;
    virtual ReflectableTypehint GetTypeHint() {
        return ReflectableTypehint::STRUCT;
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

struct ReflectableGroup {
    bool disabled = false;
    size_t id = dz::GlobalUID::GetNew("ReflectableGroup");
    int index = -1;
    bool is_child = false;
    virtual ~ReflectableGroup() = default;
    virtual std::string& GetName() {
        static std::string dummy_name = "<unknown>";
        return dummy_name;
    }
    virtual const std::vector<Reflectable*>& GetReflectables() {
        static std::vector<Reflectable*> dummy_reflectables = {};
        return dummy_reflectables;
    }
    virtual const std::vector<ReflectableGroup*>& GetChildren() {
        static std::vector<ReflectableGroup*> dummy_children = {};
        return dummy_children;
    }
};

// Reflection helpers

#define DEF_GET_ID int GetID() override { \
    return id; \
}

#define DEF_GET_NAME(TYPE) std::string& GetName() override { \
    return ComponentComponentName<TYPE>::string; \
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

#define DEF_GET_VOID_PROPERTY_BY_INDEX(NAME_INDEXES, INDEX_NAMES) void* GetVoidPropertyByIndex(int prop_index) override { \
    auto& data = GetData<PositionComponent>(); \
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

#define DEF_GET_VOID_PROPERTY_BY_NAME void* GetVoidPropertyByName(const std::string& prop_name) override { \
    auto prop_index = GetPropertyIndexByName(prop_name); \
    if (prop_index == -1) \
        return 0; \
    return GetVoidPropertyByIndex(prop_index); \
}

#define DEF_GET_PROPERTY_TYPEINFOS(TYPEINFOS) const std::vector<const std::type_info*>& GetPropertyTypeinfos() override { \
    return TYPEINFOS; \
}