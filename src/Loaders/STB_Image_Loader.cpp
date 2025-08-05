#include <dz/Loaders/STB_Image_Loader.hpp>
#include <dz/Image.hpp>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>
#include "../Directz.cpp.hpp"

int STB_Image_minChannels() {
    int minChannels = 4;
    if (dr.formats_supported.R8G8B8A8_UNORM && dr.formats_supported.R8G8B8_UNORM &&
        dr.formats_supported.R8G8_UNORM && dr.formats_supported.R8_UNORM) {
        minChannels = 0;
    }
    return minChannels;
}

dz::Image* STB_Image_load_image(const uint8_t* ptr, int width, int height, int nrChannels) {
    VkFormat format;
    switch (nrChannels) {
    case 1:
        format = VK_FORMAT_R8_UNORM;
        break;
    case 2:
        format = VK_FORMAT_R8G8_UNORM;
        break;
    case 3:
        format = VK_FORMAT_R8G8B8_UNORM;
        break;
    case 4:
        format = VK_FORMAT_R8G8B8A8_UNORM;
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

dz::Image* STB_Image_load_path(const std::filesystem::path& path) {
    auto minChannels = STB_Image_minChannels();
	int nrChannels;
    int width = 0, height = 0;
    std::string path_string = path.string();
	uint8_t *imageData = stbi_load(
        path_string.c_str(), &width, &height, &nrChannels, (std::max)(nrChannels, minChannels));
	if (!imageData)
		throw std::runtime_error("Failed to load image from memory.");
    return STB_Image_load_image(imageData, width, height, (std::max)(nrChannels, minChannels));
}

dz::Image* STB_Image_load_bytes(const std::shared_ptr<char>& bytes, size_t bytes_length) {
    auto minChannels = STB_Image_minChannels();
	int nrChannels;
    int width = 0, height = 0;
	uint8_t *imageData = stbi_load_from_memory(
        (stbi_uc const *)bytes.get(),
        bytes_length, &width, &height, &nrChannels, minChannels);
	if (!imageData)
		throw std::runtime_error("Failed to load image from memory.");
    return STB_Image_load_image(imageData, width, height, (std::max)(nrChannels, minChannels));
}

dz::Image* dz::loaders::STB_Image_Loader::Load(const dz::loaders::STB_Image_Info& info) {
    if (!info.path.empty())
        return STB_Image_load_path(info.path);
    if (info.bytes && info.bytes_length)
        return STB_Image_load_bytes(info.bytes, info.bytes_length);
    throw std::runtime_error("Neither bytes nor path were provided to info!");
}