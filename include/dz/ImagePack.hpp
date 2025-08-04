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
		std::vector<Image*> image_vec;
		std::vector<rect_type> rect_vec;
		std::vector<uint8_t> rgba_buffer;
		bool is_dirty();
		void repack();
		size_t findImageIndex(Image* image);

	public:
		~ImagePack();
		void SetOwnAtlas(bool owns = false);
		void addImage(Image* image);
		bool check();
		Image* getAtlas();
		vec<float, 2> getUV(vec<float, 2> original_uv, Image* image);
		std::vector<vec<float, 2>> getUVs(const std::vector<vec<float, 2>>& original_uvs, Image* image);
		rect_type& findPackedRect(Image* image);
		size_t size() const;
		bool empty() const;
	};
}