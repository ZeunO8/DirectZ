#pragma once
#include <string>
#include <vector>
#include <map>
#include <iostreams/Serial.hpp>

namespace dz {
    inline static const std::string kEmptyString = "";

    template <typename T>
    struct Provider {
        inline static constexpr size_t GetPID() {
            if constexpr (requires { T::PID; }) {
                return T::PID;
            }
            return 0;
        }

        inline static constexpr bool GetIsComponent() {
            if constexpr (requires { T::IsComponent; }) {
                return T::IsComponent;
            }
            return false;
        }

        inline static constexpr char GetComponentID() {
            if constexpr (requires { T::ComponentID; }) {
                return T::ComponentID;
            }
            return 0;
        }

        inline static constexpr bool GetIsDrawProvider() {
            if constexpr (requires { T::IsDrawProvider; }) {
                return T::IsDrawProvider;
            }
            return false;
        }

        inline static constexpr bool GetIsCameraProvider() {
            if constexpr (requires { T::IsCameraProvider; }) {
                return T::IsCameraProvider;
            }
            return false;
        }

        inline static constexpr bool GetIsSceneProvider() {
            if constexpr (requires { T::IsSceneProvider; }) {
                return T::IsSceneProvider;
            }
            return false;
        }

        inline static float GetPriority() {
            if constexpr (requires { T::Priority; }) {
                return T::Priority;
            }
            else {
                return 0.0f;
            }
        }

        inline static const std::string& GetProviderName() {
            if constexpr (requires { T::ProviderName; }) {
                return T::ProviderName;
            }
            else {
                return kEmptyString;
            }
        }

        inline static const std::string& GetStructName() {
            if constexpr (requires { T::StructName; }) {
                return T::StructName;
            }
            else {
                return kEmptyString;
            }
        }

        inline static const std::string& GetGLSLStruct() {
            if constexpr (requires { T::GLSLStruct; }) {
                return T::GLSLStruct;
            }
            else {
                return kEmptyString;
            }
        }

        inline static const std::unordered_map<ShaderModuleType, std::string>& GetGLSLMethods() {
            if constexpr (requires { T::GLSLMethods; }) {
                return T::GLSLMethods;
            }
            else {
                static std::unordered_map<ShaderModuleType, std::string> kEmptyVec = {};
                return kEmptyVec;
            }
        }

        inline static std::vector<std::tuple<float, std::string, ShaderModuleType>>& GetGLSLMain() {
            if constexpr (requires { T::GLSLMain; }) {
                return T::GLSLMain;
            }
            else {
                static std::vector<std::tuple<float, std::string, ShaderModuleType>> kEmptyPriorityVector = {};
                return kEmptyPriorityVector;
            }
        }

        inline static std::shared_ptr<ReflectableGroup> TryMakeGroup(BufferGroup* buffer_group) {
            return std::make_shared<typename T::ReflectableGroup>(buffer_group);
        }

        inline static std::shared_ptr<ReflectableGroup> TryMakeGroupFromSerial(BufferGroup* buffer_group, Serial& serial) {
            return std::make_shared<typename T::ReflectableGroup>(buffer_group, serial);
        }
    };

    template<typename T>
    struct IsDrawProvider
    {
        static constexpr bool value = Provider<T>::GetIsDrawProvider();
    };

    template<typename T>
    struct IsCameraProvider
    {
        static constexpr bool value = Provider<T>::GetIsCameraProvider();
    };

    template<typename T>
    struct IsSceneProvider
    {
        static constexpr bool value = Provider<T>::GetIsSceneProvider();
    };

    template<template<typename> class Trait, typename... Ts>
    struct FirstMatchingOrDefault;

    template<template<typename> class Trait, typename T, typename... Ts>
    struct FirstMatchingOrDefault<Trait, T, Ts...>
    {
        using type = std::conditional_t<
            Trait<T>::value,
            T,
            typename FirstMatchingOrDefault<Trait, Ts...>::type
        >;
    };

    template<template<typename> class Trait>
    struct FirstMatchingOrDefault<Trait>
    {
        using type = void; // or a custom "NotFound" type
    };
}