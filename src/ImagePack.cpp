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

static constexpr auto PADDING = 0; // PADDING in pixels around each image

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
			rect.h = image_ptr->height + 2 * PADDING; // <--- add vertical PADDING
			rect.w = image_ptr->width + 2 * PADDING; // <--- add horizontal PADDING
			return rect;
		});

	if (image_vec_size > 1)
	{
		const int max_side = dr_ptr->physicalDeviceProperties.limits.maxImageDimension2D;
		const int discard_step = -4;
		auto report_successful = [](rect_type&) { return rectpack2D::callback_result::CONTINUE_PACKING; };
		auto report_unsuccessful = [](rect_type&) { return rectpack2D::callback_result::ABORT_PACKING; };

		auto result_size = rectpack2D::find_best_packing<spaces_type>(
			rect_vec,
			make_finder_input(max_side, discard_step, report_successful, report_unsuccessful, runtime_flipping_mode));

		int atlas_width = result_size.w;
		int atlas_height = result_size.h;
		size_t pixel_size = get_format_pixel_size(atlas_format);

		uint32_t atlas_mip_levels = 0;

		for (size_t index = 0; index < image_vec_size; ++index)
		{
			auto& image_ptr = image_vec_data[index];
			auto& image = *image_ptr;
			
			if (atlas_mip_levels == 0) {
				atlas_mip_levels = image.mip_levels;
			}
			else if (atlas_mip_levels != image.mip_levels) {
				if (enforce_same_miplvl) {
					throw std::runtime_error("Atlas Pack: Image index [" + std::to_string(index) + "] does not match atlas mip_levels, failing");
				}
				continue;
			}
		}

		atlas_buffer_sizes.resize(atlas_mip_levels);
		atlas_buffers.resize(atlas_mip_levels);

		for (auto mip = 0; mip < atlas_mip_levels; mip++) {
			auto mipWidth = (std::max)(1, atlas_width >> mip);
			auto mipHeight = (std::max)(1, atlas_height >> mip);
			size_t byte_size = mipWidth * mipHeight * pixel_size;
			auto& atlas_buffer_size = atlas_buffer_sizes[mip];
			auto& atlas_buffer = atlas_buffers[mip];
			if (atlas_buffer_size != byte_size) {
				auto new_buffer = std::shared_ptr<void>(malloc(byte_size), free);
				if (atlas_buffer) {
					memcpy(new_buffer.get(), atlas_buffer.get(), (std::min)(atlas_buffer_size, byte_size));
				}
				atlas_buffer = new_buffer;
				atlas_buffer_size = byte_size;
			}
			memset(atlas_buffer.get(), 0, atlas_buffer_size);
		}

		CPU_Image_Copy(atlas_width, atlas_height, pixel_size, atlas_mip_levels);

		if (atlas && atlas->width == atlas_width && atlas->height == atlas_height)
		{
			for (auto mip = 0; mip < atlas_mip_levels; ++mip)
				image_upload_data(atlas, mip, atlas_buffers[mip].get());
		}
		else
		{
			atlas = image_create({
				.width = uint32_t(atlas_width),
				.height = uint32_t(atlas_height),
				.format = atlas_format,
				.datas = atlas_buffers,
				.mip_levels = atlas_mip_levels
			});
			owns_atlas = true;
		}

		GPU_Image_Copy(atlas_width, atlas_height, pixel_size, atlas_mip_levels);
	}
	else if (image_vec_size)
	{
		atlas = image_vec[0];
		owns_atlas = false;
	}
}

void ImagePack::CPU_Image_Copy(int atlas_width, int atlas_height, size_t pixel_size, uint32_t atlas_mip_levels) {
	auto image_vec_size = image_vec.size();
	auto image_vec_data = image_vec.data();
	auto rect_vec_data = rect_vec.data();

	for (size_t index = 0; index < image_vec_size; ++index) {
		auto& image_ptr = image_vec_data[index];
		auto& image = *image_ptr;
		auto channels = image_get_channels_size_of_t(image_ptr);
		auto sizeof_channels = image_get_sizeof_channels(channels);
		auto format = image.format;

		if (enforce_same_format && format != atlas_format) {
			std::cout << "Atlas Pack: Image index [" << index << "] is not the same format as atlas_format, skipping" << std::endl; 
			continue;
		}

		if (image.data_is_cpu_side && !image.data_is_gpu_side) {
			for (auto mip = 0; mip < atlas_mip_levels; mip++) {
				auto image_data = (unsigned char*)image.datas[mip].get();
				auto atlas_data = (unsigned char*)atlas_buffers[mip].get();
				auto& rect = rect_vec_data[index];

				auto image_mip_width = (std::max)(1, int(image.width) >> mip);
				auto image_mip_height = (std::max)(1, int(image.height) >> mip);
				auto atlas_mip_width = (std::max)(1, atlas_width >> mip);
				auto rect_mip_w = (std::max)(1, rect.w >> mip);
				auto rect_mip_h = (std::max)(1, rect.h >> mip);
				auto rect_mip_y = (std::max)(1, rect.y >> mip);
				auto rect_mip_x = (std::max)(1, rect.x >> mip);

				if (image_mip_width != rect_mip_w - 2 * PADDING || image_mip_height != rect.h - 2 * PADDING)
				{
					continue;
				}

				for (int y = 0; y < image_mip_height; ++y)
				{
					uint8_t* dst_row = &atlas_data[((rect_mip_y + PADDING + y) * atlas_mip_width + (rect_mip_x + PADDING)) * pixel_size];
					const uint8_t* src_row = reinterpret_cast<const uint8_t*>(&image_data[(y * image_mip_width) * sizeof_channels]);

					for (int x = 0; x < image_mip_width; ++x)
					{
						const void* src_pixel = &src_row[x * sizeof_channels];
						void* dst_pixel = &dst_row[x * pixel_size];

						convert_pixel(format, atlas_format, src_pixel, dst_pixel);
					}
				}
			}
		}

	}
}

void ImagePack::GPU_Image_Copy(int atlas_width, int atlas_height, size_t pixel_size, uint32_t atlas_mip_levels) {
	auto image_vec_size = image_vec.size();
	auto image_vec_data = image_vec.data();
	auto rect_vec_data = rect_vec.data();

	image_copy_begin();
	
	auto region_count = 0;
	for (size_t index = 0; index < image_vec_size; ++index) {
		auto& image_ptr = image_vec_data[index];
		auto& image = *image_ptr;

		if (image.data_is_gpu_side)
			region_count += atlas_mip_levels;
	}

	image_copy_reserve_regions(region_count);
	
	auto region_index = 0;
	for (size_t index = 0; index < image_vec_size; ++index) {
		auto& image_ptr = image_vec_data[index];
		auto& image = *image_ptr;

		auto format = image.format;

		if (enforce_same_format && format != atlas_format) {
			std::cout << "Atlas Pack: Image index [" << index << "] is not the same format as atlas_format, skipping" << std::endl; 
			continue;
		}

		if (image.data_is_gpu_side) {
			for (auto mip = 0; mip < atlas_mip_levels; mip++) {
				auto& rect = rect_vec_data[index];

				auto image_mip_width = (std::max)(1u, image.width >> mip);
				auto image_mip_height = (std::max)(1u, image.height >> mip);
				auto image_mip_depth = (std::max)(1u, image.depth >> mip);
				auto atlas_mip_width = (std::max)(1, atlas_width >> mip);
				auto rect_mip_w = (std::max)(1, rect.w >> mip);
				auto rect_mip_h = (std::max)(1, rect.h >> mip);
				auto rect_mip_y = (std::max)(0, rect.y >> mip);
				auto rect_mip_x = (std::max)(0, rect.x >> mip);

				if (image_mip_width != rect_mip_w - 2 * PADDING || image_mip_height != rect_mip_h - 2 * PADDING)
				{
					continue;
				}

                VkImageCopy region{};
                region.srcSubresource.aspectMask = image_get_aspect_mask(atlas);
                region.srcSubresource.mipLevel = mip;
                region.srcSubresource.baseArrayLayer = 0;
                region.srcSubresource.layerCount = 1;
                region.srcOffset = { 0, 0, 0 };

                region.dstSubresource = region.srcSubresource;
				region.dstSubresource.mipLevel = mip;
                region.dstOffset = { rect_mip_x + PADDING, rect_mip_y + PADDING, 0 };
                region.extent.width = image_mip_width;
                region.extent.height = image_mip_height;
                region.extent.depth = image_mip_depth;

                image_copy_image(atlas, image_ptr, region);
				region_index++;
			}
		}

	}

	image_copy_end();
}

bool dz::ImagePack::findImageIndex(Image* image, size_t& out_index)
{
	auto image_vec_begin = image_vec.begin();
	auto image_vec_end = image_vec.end();
	auto image_it = std::find(image_vec_begin, image_vec_end, image);
	if (image_it == image_vec_end) {
		return false;
	}
	out_index = std::distance(image_vec_begin, image_it);
	return true;
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
	size_t out_index = 0;
	if (!findImageIndex(image, out_index)) {
		image_vec.push_back(image);
	}
	else {
		std::cout << "Image already added to pack!" << std::endl;
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
dz::ImagePack::rect_type& dz::ImagePack::findPackedRect(Image* image)
{
	size_t out_index = 0;
	if (findImageIndex(image, out_index))
		return rect_vec[out_index];
	throw std::runtime_error("Image does not exist in Pack!");
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