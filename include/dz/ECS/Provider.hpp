#pragma once
#include <string>
#include <vector>
#include <map>

namespace dz {
    inline static const std::string kEmptyString = "";

    template <typename T>
    struct Provider {
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
    };
}