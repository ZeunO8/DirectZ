#pragma once
#undef min
#undef max
#include <rectpack2D/finders_interface.h>
#include <vector>
#include "Image.hpp"
namespace dz {
	struct ImagePack
	{
	private:
		static constexpr bool allow_flip = false;
		const rectpack2D::flipping_option runtime_flipping_mode = rectpack2D::flipping_option::DISABLED;
		using spaces_type = rectpack2D::empty_spaces<allow_flip, rectpack2D::default_empty_spaces>;
		using rect_type = rectpack2D::output_rect_t<spaces_type>;
		Image* atlas = nullptr;
		bool owns_atlas = true;
		VkFormat atlas_format = VK_FORMAT_R8G8B8A8_UNORM;
		bool format_changed = false;
		std::vector<Image*> image_vec;
		std::vector<rect_type> rect_vec;
		std::vector<uint8_t> rgba_buffer;
		bool is_dirty();
		void repack();
		size_t findImageIndex(Image* image);

	public:
		~ImagePack();
		void SetOwnAtlas(bool owns = false);
		void SetAtlasFormat(VkFormat new_format);
		void addImage(Image* image);
		bool check();
		Image* getAtlas();
		vec<float, 2> getUV(vec<float, 2> original_uv, Image* image);
		std::vector<vec<float, 2>> getUVs(const std::vector<vec<float, 2>>& original_uvs, Image* image);
		rect_type& findPackedRect(Image* image);
		size_t size() const;
		bool empty() const;

	private:

		using ConvertFunc = std::function<void(const void*, void*)>;

		// Conversion helpers
		static void copy_bytes(const void* src, void* dst, size_t size);

		// Float conversion helpers
		static uint8_t float_to_u8(float val);

		static float u8_to_float(uint8_t val);

		static void convert_R32G32B32A32_SFLOAT_to_R8G8B8A8_UNORM(const void* src, void* dst);

		static void convert_R8G8B8A8_UNORM_to_R32G32B32A32_SFLOAT(const void* src, void* dst);

		// Extend with more detailed conversions as needed...

		inline static const VkFormat supported_formats[] = {
			VK_FORMAT_R8_UNORM,
			VK_FORMAT_R8G8_UNORM,
			VK_FORMAT_R8G8B8_UNORM,
			VK_FORMAT_R8G8B8A8_UNORM,
			VK_FORMAT_R32_SFLOAT,
			VK_FORMAT_R32G32_SFLOAT,
			VK_FORMAT_R32G32B32_SFLOAT,
			VK_FORMAT_R32G32B32A32_SFLOAT
		};

		inline static constexpr int FMT_COUNT = sizeof(supported_formats) / sizeof(*supported_formats);

		inline static ConvertFunc conversion_matrix[FMT_COUNT][FMT_COUNT] = {};
		inline static size_t format_sizes[FMT_COUNT] = {};

		static int format_index(VkFormat fmt);

		static void init_conversion_matrix();

		void convert_pixel(VkFormat src_format, VkFormat dst_format, const void* src, void* dst);

	};
}