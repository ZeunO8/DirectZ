/**
 * @file Image.hpp
 * @brief Provides image resizing utilities for 2D and 3D image buffers.
 */
#pragma once
#include <cstdint>

namespace dz
{
    struct Image;

    /**
     * @brief Resizes a 2D image to the specified dimensions.
     * 
     * @param image Pointer to the Image structure.
     * @param image_width Width of the image in pixels.
     * @param image_height Height of the image in pixels.
     */
    void image_resize_2D(Image* image, uint32_t image_width, uint32_t image_height);

    /**
     * @brief Resizes a 3D image to the specified dimensions.
     * 
     * @param image Pointer to the Image structure.
     * @param image_width Width of the image in pixels.
     * @param image_height Height of the image in pixels.
     * @param image_depth Depth of the image in pixels.
     */
    void image_resize_3D(Image* image, uint32_t image_width, uint32_t image_height, uint32_t image_depth);
}