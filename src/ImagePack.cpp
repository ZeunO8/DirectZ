#include <dz/ImagePack.hpp>
#include "Image.cpp.hpp"
#include "Directz.cpp.hpp"

#include <stdexcept>
#include <algorithm>
#include <cstring>
#include <memory>
#include <vector>
#include <cstdint>
#include <cmath>
#include <thread>
#include <execution>

// using Format = zg::images::Image::Format;
bool dz::ImagePack::is_dirty()
{
    if (image_vec.size() != rect_vec.size())
        return true;
	bool any_dirty = false;
	for (auto& tp : image_vec)
	{
		// auto& t = *tp;
		// if (t.isDirty)
		// {
		// 	any_dirty = true;
		// 	t.isDirty = false;
		// }
	}
	return any_dirty;
}
void dz::ImagePack::repack()
{

	constexpr int padding = 0; // padding in pixels around each image
	size_t image_vec_size = image_vec.size();
	rect_vec.resize(image_vec_size);
	auto image_vec_data = image_vec.data();
	auto rect_vec_data = rect_vec.data();

	std::transform(image_vec_data, image_vec_data + image_vec_size, rect_vec_data,
		[&](Image* image_ptr) -> rect_type
		{
			rect_type rect;
			rect.x = 0;
			rect.y = 0;
			rect.h = image_ptr->height + 2 * padding; // <--- add vertical padding
			rect.w = image_ptr->width + 2 * padding; // <--- add horizontal padding
			return rect;
		});

	if (image_vec_size > 1)
	{
		const int max_side = dr.physicalDeviceProperties.limits.maxImageDimension2D;
		const int discard_step = -4;
		auto report_successful = [](rect_type&) { return rectpack2D::callback_result::CONTINUE_PACKING; };
		auto report_unsuccessful = [](rect_type&) { return rectpack2D::callback_result::ABORT_PACKING; };

		auto result_size = rectpack2D::find_best_packing<spaces_type>(
			rect_vec,
			make_finder_input(max_side, discard_step, report_successful, report_unsuccessful, runtime_flipping_mode));

		int atlas_width = result_size.w;
		int atlas_height = result_size.h;
		size_t pixel_size = get_format_pixel_size(atlas_format);
		size_t byte_size = atlas_width * atlas_height * pixel_size;
		if (rgba_buffer.size() < byte_size)
			rgba_buffer.resize(byte_size);
		uint8_t* atlas_data = rgba_buffer.data();
		memset(atlas_data, 0, byte_size);

		for (size_t index = 0; index < image_vec_size; ++index)
		{
			auto& image_ptr = image_vec_data[index];
			auto& image = *image_ptr;
			auto channels = image_get_channels_size_of_t(image_ptr);
			auto sizeof_channels = image_get_sizeof_channels(channels);
			auto format = image.format;
			auto image_data = (char*)image.data.get();
			auto& rect = rect_vec_data[index];

			if (image.width != rect.w - 2 * padding || image.height != rect.h - 2 * padding)
			{
				continue;
			}

			for (int y = 0; y < image.height; ++y)
			{
				uint8_t* dst_row = &atlas_data[((rect.y + padding + y) * atlas_width + (rect.x + padding)) * pixel_size];
				const uint8_t* src_row = reinterpret_cast<const uint8_t*>(&image_data[(y * image.width) * sizeof_channels]);

				for (int x = 0; x < image.width; ++x)
				{
					const void* src_pixel = &src_row[x * sizeof_channels];
					void* dst_pixel = &dst_row[x * pixel_size];

					convert_pixel(format, atlas_format, src_pixel, dst_pixel);
				}
			}
		}

		if (atlas && atlas->width == atlas_width && atlas->height == atlas_height)
		{
			image_upload_data(atlas, atlas_data);
		}
		else
		{
			atlas = image_create({
				.width = uint32_t(atlas_width),
				.height = uint32_t(atlas_height),
				.format = atlas_format,
				.data = atlas_data,
			});
			owns_atlas = true;
		}
	}
	else if (image_vec_size)
	{
		atlas = image_vec[0];
		owns_atlas = false;
	}
}
size_t dz::ImagePack::findImageIndex(Image* image)
{
	size_t index = 0;
	auto image_vec_size = image_vec.size();
	auto image_vec_data = image_vec.data();
	for (; index < image_vec_size; ++index)
	{
		if (image == image_vec_data[index])
			break;
	}
	if (index < image_vec_size)
		return index;
	else
		throw std::runtime_error("Image not found");
}
ImagePack::~ImagePack() {
	if (owns_atlas)
		image_free(atlas);
}
void ImagePack::SetOwnAtlas(bool owns) {
	owns_atlas = owns;
}
void ImagePack::SetAtlasFormat(VkFormat new_format) {
	atlas_format = new_format;
}
void dz::ImagePack::addImage(Image* image)
{
	try
	{
		findImageIndex(image);
	}
	catch (...)
	{
		image_vec.push_back(image);
	}
}
bool dz::ImagePack::check()
{
	auto isDirty = is_dirty();
	if (!isDirty) {
		if (!atlas) {
			atlas = image_create({
				.width = 1,
				.height = 1
			});
		}
		return false;
	}
	repack();
	return true;
}
Image* dz::ImagePack::getAtlas() { return atlas; }
vec<float, 2> dz::ImagePack::getUV(vec<float, 2> original_uv, Image* image)
{
    return {};
	// auto& image_ref = *image;
	// float pixel_u = original_uv.x * image_ref.size.x;
	// float pixel_v = original_uv.y * image_ref.size.y;
	// auto& packed_rect = findPackedRect(image);
	// auto& atlas_size = atlas->size;
	// float atlas_u = (packed_rect.x + pixel_u) / float(atlas_size.x);
	// float atlas_v = (packed_rect.y + pixel_v) / float(atlas_size.y);
	// return {atlas_u, atlas_v};
}
std::vector<vec<float, 2>> dz::ImagePack::getUVs(const std::vector<vec<float, 2>>& original_uvs, Image* image)
{
	auto& image_ref = *image;
	auto& packed_rect = findPackedRect(image);
	std::vector<vec<float, 2>> new_uvs;
	new_uvs.reserve(original_uvs.size());
	// for (auto& original_uv : original_uvs)
	// {
	// 	float pixel_u = original_uv.x * image_ref.size.x;
	// 	float pixel_v = original_uv.y * image_ref.size.y;
	// 	auto& atlas_size = atlas->size;
	// 	float atlas_u = (packed_rect.x + pixel_u) / float(atlas_size.x);
	// 	float atlas_v = (packed_rect.y + pixel_v) / float(atlas_size.y);
	// 	new_uvs.push_back({atlas_u, atlas_v});
	// }
	return new_uvs;
}
dz::ImagePack::rect_type& dz::ImagePack::findPackedRect(Image* image)
{
	return rect_vec[findImageIndex(image)];
}
size_t dz::ImagePack::size() const { return image_vec.size(); }
bool dz::ImagePack::empty() const { return image_vec.empty(); }


void ImagePack::copy_bytes(const void* src, void* dst, size_t size)
{
	std::memcpy(dst, src, size);
}

uint8_t ImagePack::float_to_u8(float val)
{
	return static_cast<uint8_t>(std::clamp(val, 0.0f, 1.0f) * 255.0f);
}

float ImagePack::u8_to_float(uint8_t val)
{
	return static_cast<float>(val) / 255.0f;
}

void ImagePack::convert_R32G32B32A32_SFLOAT_to_R8G8B8A8_UNORM(const void* src, void* dst)
{
	const float* f = reinterpret_cast<const float*>(src);
	uint8_t* d = reinterpret_cast<uint8_t*>(dst);
	d[0] = float_to_u8(f[0]);
	d[1] = float_to_u8(f[1]);
	d[2] = float_to_u8(f[2]);
	d[3] = float_to_u8(f[3]);
}

void ImagePack::convert_R8G8B8A8_UNORM_to_R32G32B32A32_SFLOAT(const void* src, void* dst)
{
	const uint8_t* s = reinterpret_cast<const uint8_t*>(src);
	float* f = reinterpret_cast<float*>(dst);
	f[0] = u8_to_float(s[0]);
	f[1] = u8_to_float(s[1]);
	f[2] = u8_to_float(s[2]);
	f[3] = u8_to_float(s[3]);
}

int ImagePack::format_index(VkFormat fmt)
{
	for (int i = 0; i < FMT_COUNT; ++i)
		if (supported_formats[i] == fmt)
			return i;
	return -1;
}

void ImagePack::init_conversion_matrix()
{
	static bool initialized = false;
	if (initialized) return;
	initialized = true;

	// Set format sizes
	format_sizes[format_index(VK_FORMAT_R8_UNORM)] = 1;
	format_sizes[format_index(VK_FORMAT_R8G8_UNORM)] = 2;
	format_sizes[format_index(VK_FORMAT_R8G8B8_UNORM)] = 3;
	format_sizes[format_index(VK_FORMAT_R8G8B8A8_UNORM)] = 4;
	format_sizes[format_index(VK_FORMAT_R32_SFLOAT)] = 4;
	format_sizes[format_index(VK_FORMAT_R32G32_SFLOAT)] = 8;
	format_sizes[format_index(VK_FORMAT_R32G32B32_SFLOAT)] = 12;
	format_sizes[format_index(VK_FORMAT_R32G32B32A32_SFLOAT)] = 16;

	// Identity copies
	for (int i = 0; i < FMT_COUNT; ++i)
		conversion_matrix[i][i] = [=](const void* src, void* dst) { copy_bytes(src, dst, format_sizes[i]); };

	// Specific conversions
	int r8g8b8a8_idx = format_index(VK_FORMAT_R8G8B8A8_UNORM);
	int r32g32b32a32_idx = format_index(VK_FORMAT_R32G32B32A32_SFLOAT);

	conversion_matrix[r32g32b32a32_idx][r8g8b8a8_idx] = convert_R32G32B32A32_SFLOAT_to_R8G8B8A8_UNORM;
	conversion_matrix[r8g8b8a8_idx][r32g32b32a32_idx] = convert_R8G8B8A8_UNORM_to_R32G32B32A32_SFLOAT;
}

void ImagePack::convert_pixel(VkFormat src_format, VkFormat dst_format, const void* src, void* dst)
{
	init_conversion_matrix();

	int src_idx = format_index(src_format);
	int dst_idx = format_index(dst_format);

	if (src_idx == -1 || dst_idx == -1)
		throw std::runtime_error("Unsupported format in convert_pixel");

	ConvertFunc func = conversion_matrix[src_idx][dst_idx];

	if (!func)
		throw std::runtime_error("Conversion path not implemented between formats");

	func(src, dst);
}