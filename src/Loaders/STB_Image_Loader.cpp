#include <dz/Loaders/STB_Image_Loader.hpp>
#include <dz/Image.hpp>
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_STATIC
#include <stb_image.h>

dz::Image* STB_Image_load_rgba(const uint8_t* ptr, int width, int height) {
    dz::ImageCreateInfo info{
        .width = (uint32_t)width,
        .height = (uint32_t)height,
        .data = (void*)ptr
    };
    return dz::image_create(info);
}

dz::Image* STB_Image_load_path(const std::filesystem::path& path) {
	int nrChannels;
    int width = 0, height = 0;
    std::string path_string = path.string();
	uint8_t *imageData = stbi_load(
        path_string.c_str(), &width, &height, &nrChannels, 4);
	if (!imageData)
		throw std::runtime_error("Failed to load image from memory.");
    return STB_Image_load_rgba(imageData, width, height);
}

dz::Image* STB_Image_load_bytes(const std::shared_ptr<char>& bytes, size_t bytes_length) {
	int nrChannels;
    int width = 0, height = 0;
	uint8_t *imageData = stbi_load_from_memory(
        (stbi_uc const *)bytes.get(),
        bytes_length, &width, &height, &nrChannels, 4);
	if (!imageData)
		throw std::runtime_error("Failed to load image from memory.");
    return STB_Image_load_rgba(imageData, width, height);
}

dz::Image* dz::loaders::STB_Image_Loader::Load(const dz::loaders::STB_Image_Info& info) {
    if (!info.path.empty())
        return STB_Image_load_path(info.path);
    if (info.bytes && info.bytes_length)
        return STB_Image_load_bytes(info.bytes, info.bytes_length);
    throw std::runtime_error("Neither bytes nor path were provided to info!");
}