#pragma once

namespace dz {
    template <typename... TLoaders>
    struct TypeLoader {
    private:
        template <typename TType, typename TInfo, typename TLoader>
        static void LoadSingle(TType*& out_type_ptr, const TInfo& info) {
            if (out_type_ptr)
                return;
            if constexpr (
                !(std::is_same_v<TType, typename TLoader::ptr_type>) ||
                !(std::is_same_v<TInfo, typename TLoader::info_type>)
            )
                return;
            out_type_ptr = TLoader::Load(info);
        }
    public:
        template <typename TType, typename TInfo>
        static TType* Load(const TInfo& info) {
            TType* out_type_ptr = nullptr;
            (LoadSingle<TType, TInfo, TLoaders>(out_type_ptr, info), ...);
            if (!out_type_ptr)
                throw std::runtime_error("Unable to Load TType with TInfo. (Loader not found?)");
            return out_type_ptr;
        }
    };
}