#include <dz/Loaders/STB_Image_Loader.hpp>
#include <dz/Image.hpp>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>
#include "../Directz.cpp.hpp"

int STB_Image_minChannelsu() {
    int minChannels = 4;
    if (dr.formats_supported.R8G8B8A8_UNORM && dr.formats_supported.R8G8B8_UNORM &&
        dr.formats_supported.R8G8_UNORM && dr.formats_supported.R8_UNORM) {
        minChannels = 0;
    }
    return minChannels;
}

int STB_Image_minChannelsf() {
    int minChannels = 4;
    if (dr.formats_supported.R32G32B32A32_SFLOAT && dr.formats_supported.R32G32B32_SFLOAT &&
        dr.formats_supported.R32G32_SFLOAT && dr.formats_supported.R32_SFLOAT) {
        minChannels = 0;
    }
    return minChannels;
}

dz::Image* STB_Image_load_image_uf(const void* ptr, int width, int height, int nrChannels, bool load_float) {
    VkFormat format;
    switch (nrChannels) {
    case 1:
        format = (load_float ? VK_FORMAT_R32_SFLOAT : VK_FORMAT_R8_UNORM);
        break;
    case 2:
        format = (load_float ? VK_FORMAT_R32G32_SFLOAT : VK_FORMAT_R8G8_UNORM);
        break;
    case 3:
        format = (load_float ? VK_FORMAT_R32G32B32_SFLOAT : VK_FORMAT_R8G8B8_UNORM);
        break;
    case 4:
        format = (load_float ? VK_FORMAT_R32G32B32A32_SFLOAT : VK_FORMAT_R8G8B8A8_UNORM);
        break;
    }
    dz::ImageCreateInfo info{
        .width = (uint32_t)width,
        .height = (uint32_t)height,
        .format = format,
        .data = (void*)ptr
    };
    return dz::image_create(info);
}

dz::Image* STB_Image_load_pathu(const std::filesystem::path& path) {
    auto minChannels = STB_Image_minChannelsu();
	int nrChannels = 0, width = 0, height = 0;
    std::string path_string = path.string();
	uint8_t *imageData = stbi_load(
        path_string.c_str(), &width, &height, &nrChannels, (std::max)(nrChannels, minChannels));
	if (!imageData)
		throw std::runtime_error("Failed to load image from memory.");
    return STB_Image_load_image_uf(imageData, width, height, (std::max)(nrChannels, minChannels), false);
}

dz::Image* STB_Image_load_bytesu(const std::shared_ptr<char>& bytes, size_t bytes_length) {
    auto minChannels = STB_Image_minChannelsu();
	int nrChannels = 0, width = 0, height = 0;
	uint8_t *imageData = stbi_load_from_memory(
        (stbi_uc const *)bytes.get(),
        bytes_length, &width, &height, &nrChannels, minChannels);
	if (!imageData)
		throw std::runtime_error("Failed to load image from memory.");
    return STB_Image_load_image_uf(imageData, width, height, (std::max)(nrChannels, minChannels), false);
}

dz::Image* STB_Image_load_pathf(const std::filesystem::path& path) {
    auto minChannels = STB_Image_minChannelsf();
	int nrChannels = 0, width = 0, height = 0;
    std::string path_string = path.string();
	float *imageData = stbi_loadf(
        path_string.c_str(), &width, &height, &nrChannels, (std::max)(nrChannels, minChannels));
	if (!imageData)
		throw std::runtime_error("Failed to load image from memory.");
    return STB_Image_load_image_uf(imageData, width, height, (std::max)(nrChannels, minChannels), true);
}

dz::Image* STB_Image_load_bytesf(const std::shared_ptr<char>& bytes, size_t bytes_length) {
    auto minChannels = STB_Image_minChannelsf();
	int nrChannels = 0, width = 0, height = 0;
	float *imageData = stbi_loadf_from_memory(
        (stbi_uc const *)bytes.get(),
        bytes_length, &width, &height, &nrChannels, minChannels);
	if (!imageData)
		throw std::runtime_error("Failed to load image from memory.");
    return STB_Image_load_image_uf(imageData, width, height, (std::max)(nrChannels, minChannels), true);
}

dz::Image* dz::loaders::STB_Image_Loader::Load(const dz::loaders::STB_Image_Info& info) {
    if (!info.path.empty())
        return (info.load_float) ? STB_Image_load_pathf(info.path) : STB_Image_load_pathu(info.path);
    if (info.bytes && info.bytes_length)
        return (info.load_float) ? STB_Image_load_bytesf(info.bytes, info.bytes_length) : STB_Image_load_bytesu(info.bytes, info.bytes_length);
    throw std::runtime_error("Neither bytes nor path were provided to info!");
}