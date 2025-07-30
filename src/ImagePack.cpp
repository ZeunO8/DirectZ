#include <dz/ImagePack.hpp>

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
bool ImagePack::is_dirty()
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
void ImagePack::repack()
{

	constexpr int padding = 0; // padding in pixels around each image
	size_t image_vec_size = image_vec.size();
	rect_vec.resize(image_vec_size);
	auto image_vec_data = image_vec.data();
	auto rect_vec_data = rect_vec.data();

	std::transform(std::execution::par_unseq, image_vec_data, image_vec_data + image_vec_size, rect_vec_data,
		[](Image* image_ptr)
		{
			auto& image = *image_ptr;
			rect_type rect;
			rect.x = 0;
			rect.y = 0;
			rect.h = image.height + 2 * padding; // <--- add vertical padding
			rect.w = image.width + 2 * padding; // <--- add horizontal padding
			return rect;
		});

	if (image_vec_size > 1)
	{
		const int max_side = 4096;
		const int discard_step = -4;
		auto report_successful = [](rect_type&) { return rectpack2D::callback_result::CONTINUE_PACKING; };
		auto report_unsuccessful = [](rect_type&) { return rectpack2D::callback_result::ABORT_PACKING; };

		auto result_size = rectpack2D::find_best_packing<spaces_type>(
			rect_vec,
			make_finder_input(max_side, discard_step, report_successful, report_unsuccessful, runtime_flipping_mode));

		int atlas_width = result_size.w;
		int atlas_height = result_size.h;
		size_t byte_size = atlas_width * atlas_height * 4;
		if (rgba_buffer.size() < byte_size)
			rgba_buffer.resize(byte_size);
		uint8_t* rgba = rgba_buffer.data();
		memset(rgba, 0, byte_size);

		for (size_t index = 0; index < image_vec_size; ++index)
		{
			auto& image_ptr = image_vec_data[index];
			auto& image = *image_ptr;
			auto [channels, sizeoftype] = image_get_channels_size_of_t(image_ptr);
			auto format = image.format;
			auto* image_data = (char*)image.data.get();
			auto& rect = rect_vec_data[index];

			// Ensure sizes match (excluding padding)
			if (image.width != rect.w - 2 * padding || image.height != rect.h - 2 * padding)
			{
				continue;
			}

			for (int y = 0; y < image.height; ++y)
			{
				uint8_t* dst_row = &rgba[((rect.y + padding + y) * atlas_width + (rect.x + padding)) * 4];
				const char* src_row = &image_data[(y * image.width) * channels * sizeoftype];

				for (int x = 0; x < image.width; ++x)
				{
					const char* src_pixel = src_row + x * channels * sizeoftype;
					uint8_t* dst_pixel = dst_row + x * 4;

					switch (format)
					{
					case VK_FORMAT_R8_UNORM:
						dst_pixel[0] = *(const uint8_t*)src_pixel;
						dst_pixel[1] = 0;
						dst_pixel[2] = 0;
						dst_pixel[3] = 255;
						break;
					case VK_FORMAT_R8G8_UNORM:
						dst_pixel[0] = ((const uint8_t*)src_pixel)[0];
						dst_pixel[1] = ((const uint8_t*)src_pixel)[1];
						dst_pixel[2] = 0;
						dst_pixel[3] = 255;
						break;
					case VK_FORMAT_R8G8B8_UNORM:
						dst_pixel[0] = ((const uint8_t*)src_pixel)[0];
						dst_pixel[1] = ((const uint8_t*)src_pixel)[1];
						dst_pixel[2] = ((const uint8_t*)src_pixel)[2];
						dst_pixel[3] = 255;
						break;
					case VK_FORMAT_R8G8B8A8_UNORM:
						std::memcpy(dst_pixel, src_pixel, 4);
						break;
					case VK_FORMAT_R32G32B32A32_SFLOAT:
						{
							const float* fp = (const float*)src_pixel;
							dst_pixel[0] = static_cast<uint8_t>(std::clamp(fp[0], 0.0f, 1.0f) * 255.0f);
							dst_pixel[1] = static_cast<uint8_t>(std::clamp(fp[1], 0.0f, 1.0f) * 255.0f);
							dst_pixel[2] = static_cast<uint8_t>(std::clamp(fp[2], 0.0f, 1.0f) * 255.0f);
							dst_pixel[3] = static_cast<uint8_t>(std::clamp(fp[3], 0.0f, 1.0f) * 255.0f);
							break;
						}
					case VK_FORMAT_D32_SFLOAT:
						{
							float d = *(const float*)src_pixel;
							dst_pixel[0] = dst_pixel[1] = dst_pixel[2] = static_cast<uint8_t>(std::clamp(d, 0.0f, 1.0f) * 255.0f);
							dst_pixel[3] = 255;
							break;
						}
					case VK_FORMAT_R8_UINT:
						{
							uint8_t s = *(const uint8_t*)src_pixel;
							dst_pixel[0] = dst_pixel[1] = dst_pixel[2] = s;
							dst_pixel[3] = 255;
							break;
						}
					case VK_FORMAT_D32_SFLOAT_S8_UINT:
						{
							uint32_t packed = *(const uint32_t*)src_pixel;
							float depth = *((float*)&packed);
							dst_pixel[0] = packed & 0xFF;
							dst_pixel[1] = dst_pixel[2] = static_cast<uint8_t>(std::clamp(depth, 0.0f, 1.0f) * 255.0f);
							dst_pixel[3] = 255;
							break;
						}
					// case VK_FORMAT_R32_UINT:
					// 	{
					// 		uint32_t val = *(const unsigned int32_t*)src_pixel;
					// 		dst_pixel[0] = dst_pixel[1] = dst_pixel[2] = static_cast<uint8_t>(std::clamp(val, 0, 255));
					// 		dst_pixel[3] = 255;
					// 		break;
					// 	}
					default:
						dst_pixel[0] = dst_pixel[1] = dst_pixel[2] = 0;
						dst_pixel[3] = 255;
						break;
					}
				}
			}
		}

		if (atlas && atlas->width == atlas_width && atlas->height == atlas_height)
		{
			image_upload_data(atlas, rgba);
		}
		else
		{
			atlas = image_create({
				.width = uint32_t(atlas_width),
				.height = uint32_t(atlas_height),
				.data = rgba
			});
		}
	}
	else if (image_vec_size)
	{
		atlas = image_vec[0];
	}
}
size_t ImagePack::findImageIndex(Image* image)
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
void ImagePack::addImage(Image* image)
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
bool ImagePack::check()
{
	auto isDirty = is_dirty();
	if (!isDirty)
		return false;
	repack();
	return true;
}
Image* ImagePack::getAtlas() { return atlas; }
vec<float, 2> ImagePack::getUV(vec<float, 2> original_uv, Image* image)
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
std::vector<vec<float, 2>> ImagePack::getUVs(const std::vector<vec<float, 2>>& original_uvs, Image* image)
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
ImagePack::rect_type& ImagePack::findPackedRect(Image* image)
{
	return rect_vec[findImageIndex(image)];
}
size_t ImagePack::size() const { return image_vec.size(); }
bool ImagePack::empty() const { return image_vec.empty(); }
