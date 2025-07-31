#pragma once

#include <stdexcept>
#include <type_traits>

namespace dz {
	template <typename... TLoaders>
	struct TypeLoader {
	private:
		template <typename TType, typename TInfo, typename TLoader>
		static bool TryLoadSingle(TType& out_type, const TInfo& info, bool& loaded) {
			if (loaded)
				return false;

			if constexpr (
				std::is_same_v<TType, typename TLoader::value_type> &&
				std::is_same_v<TInfo, typename TLoader::info_type>
			) {
				out_type = TLoader::Load(info);
				loaded = true;
				return true;
			}
			return false;
		}

	public:
		template <typename TType, typename TInfo>
		static TType Load(const TInfo& info) {
			TType out_type{};
			bool loaded = false;
			(TryLoadSingle<TType, TInfo, TLoaders>(out_type, info, loaded), ...);
			if (!loaded)
				throw std::runtime_error("Unable to Load TType with TInfo. (Loader not found?)");
			return out_type;
		}
	};
}
