#pragma once

#include <string>
#include <vector>
#include <typeinfo>
#include <map>
#include "GlobalUID.hpp"

enum class ReflectableTypeHint {
    FLOAT,
    VEC2,
    VEC3,
    VEC3_RGB,
    VEC4,
    VEC4_RGBA,
    MAT4,
    STRUCT
};

struct Reflectable {
    virtual ~Reflectable() = default;

    virtual int GetID() = 0;
    virtual std::string& GetName() = 0;
    virtual ReflectableTypeHint GetTypeHint() {
        return ReflectableTypeHint::STRUCT;
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
    virtual void NotifyChange(int prop_index) { }
    virtual const std::vector<bool>& GetDisabledProperties() {
        static std::vector<bool> dummy_disables = {};
        return dummy_disables;
    }
};

struct ReflectableGroup {
    enum GroupType {
        Generic,
        Window,
        Scene,
        Entity,
        Camera,
        Light,
        Provider
    };
    bool disabled = false;
    size_t id;
    int index = -1;
    bool is_child = false;
    ReflectableGroup* parent_ptr = nullptr;
    virtual ~ReflectableGroup() = default;
    virtual GroupType GetGroupType() {
        return Generic;
    }
    virtual std::string& GetName() {
        static std::string dummy_name = "<unknown>";
        return dummy_name;
    }
    virtual void NotifyNameChanged() { }
    virtual const std::vector<Reflectable*>& GetReflectables() {
        static std::vector<Reflectable*> dummy_reflectables = {};
        return dummy_reflectables;
    }
    virtual std::vector<std::shared_ptr<ReflectableGroup>>& GetChildren() {
        static std::vector<std::shared_ptr<ReflectableGroup>> dummy_children = {};
        return dummy_children;
    }
    virtual void UpdateChildren() {}
};

// Reflection helpers

#define DEF_GET_ID int GetID() override { \
    return id; \
}

#define DEF_GET_NAME(TYPE) std::string& GetName() override { \
    return GetProviderName(); \
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

// #define DEF_GET_VOID_PROPERTY_BY_INDEX(NAME_INDEXES, INDEX_NAMES) void* GetVoidPropertyByIndex(int prop_index) override { \
//     auto data = i->GetComponentDataVoid(index); \
//     auto index_it = INDEX_NAMES.find(prop_index); \
//     if (index_it == INDEX_NAMES.end()) \
//         return nullptr; \
//     auto& prop_name = index_it->second; \
//     auto it = NAME_INDEXES.find(prop_name); \
//     if (it == NAME_INDEXES.end()) \
//         return nullptr; \
//     auto offset = it->second.second; \
//     return ((char*)data) + offset; \
// }

#define DEF_GET_VOID_PROPERTY_BY_NAME void* GetVoidPropertyByName(const std::string& prop_name) override { \
    auto prop_index = GetPropertyIndexByName(prop_name); \
    if (prop_index == -1) \
        return 0; \
    return GetVoidPropertyByIndex(prop_index); \
}

#define DEF_GET_PROPERTY_TYPEINFOS(TYPEINFOS) const std::vector<const std::type_info*>& GetPropertyTypeinfos() override { \
    return TYPEINFOS; }

#define DEF_GET_DISABLED_PROPERTIES(DISABLED_PROPERTIES) const std::vector<bool>& GetDisabledProperties() override { \
    return DISABLED_PROPERTIES; }
