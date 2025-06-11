#pragma once
namespace dz
{
    struct Image;
    void image_resize_2D(Image* image, uint32_t image_width, uint32_t image_height);
    void image_resize_3D(Image* image, uint32_t image_width, uint32_t image_height, uint32_t image_depth);
}