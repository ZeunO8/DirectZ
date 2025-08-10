/**
 * @file Image.hpp
 * @brief Provides image resizing utilities for 2D and 3D image buffers.
 */
#pragma once
#include "Renderer.hpp"
#include <cstdint>
#include <iostreams/Serial.hpp>

namespace dz
{
    struct Image;

    enum class SurfaceType {
        BaseColor,
        Diffuse,
        Specular,
        Normal,
        Height,
        AmbientOcclusion,
        DiffuseRoughness,
        Metalness,
        Shininess,
        MetalnessRoughness
    };

    struct ImageCreateInfo {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        VkFormat format = (VkFormat)ColorSpace::SRGB;
        VkImageUsageFlags usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        VkImageType image_type = VK_IMAGE_TYPE_2D;
        VkImageViewType view_type = VK_IMAGE_VIEW_TYPE_2D;
        VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
        VkMemoryPropertyFlags memory_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        VkSampleCountFlagBits multisampling = VK_SAMPLE_COUNT_1_BIT;
        bool is_framebuffer_attachment = false;
        std::vector<std::shared_ptr<void>> datas;
        SurfaceType surfaceType = SurfaceType::BaseColor;
        uint32_t mip_levels = 1;
    };

    /**
    * @brief Creates a image given the passed info
    *
    * @returns an Image pointer used in various image_X, framebuffer_Y functions
    */
    Image* image_create(const ImageCreateInfo&);

    /**
    * @brief Uploads new data to an Image at a given mip level
    *
    * @note if just image_ptr is provided, will upload whatever exists in CPU buffer
    * @note if data provided, data must contain the bytes in the correct format
    */
    void image_upload_data(Image* image_ptr, uint32_t mip = 0, void* data = nullptr);

    /**
     * @brief Resizes a 2D image to the specified dimensions.
     * 
     * @param image Pointer to the Image structure.
     * @param image_width Width of the image in pixels.
     * @param image_height Height of the image in pixels.
     */
    void image_resize_2D(Image*& image, uint32_t image_width, uint32_t image_height, void* data = nullptr, bool create_new = false);

    /**
     * @brief Resizes a 3D image to the specified dimensions.
     * 
     * @param image Pointer to the Image structure.
     * @param image_width Width of the image in pixels.
     * @param image_height Height of the image in pixels.
     * @param image_depth Depth of the image in pixels.
     */
    void image_resize_3D(Image*& image, uint32_t image_width, uint32_t image_height, uint32_t image_depth, void* data = nullptr, bool create_new = false);

    /**
    * @brief Frees an image, should only be called for self owned Images. Images created by BufferGroups should not be passed to this function
    */
    void image_free(Image* image);
    
    /**
    * @brief Creates a single descriptor set for a given Image*
    *
    * @note no need to free the descriptor as this is done by the DirectRegistry
    *
    * @returns a pair containing the SetLayout and Set for usage
    */
    std::pair<VkDescriptorSetLayout, VkDescriptorSet> image_create_descriptor_set(Image* image, uint32_t mip_level = 0);

    /**
    * @brief Gets the per channel sizes from a VkFormat
     */
    std::vector<float> format_get_channels_size_of_t(VkFormat format);

    /**
    * @brief Gets the per channel sizes from an Image
    */
    std::vector<float> image_get_channels_size_of_t(Image* image);

    /**
    * @brief Computes the total size of a channels vector
    *
    * @returns the channels pixel stride in bytes
    */
    size_t image_get_sizeof_channels(const std::vector<float>& channels);

    /**
     * @brief returns the underlying surface type of an Image
     */
    SurfaceType image_get_surface_type(Image*);

    /**
     * @brief returns the underlying width of an Image
     */
    uint32_t image_get_width(Image*);

    /**
     * @brief returns the underlying height of an Image
     */
    uint32_t image_get_height(Image*);

    /**
     * @brief returns the underlying depth of an Image
     */
    uint32_t image_get_depth(Image*);

    /**
     * @brief returns the underlying layout of an Image
     */
    VkImageLayout image_get_layout(Image*, int mip);

    /**
     * @brief returns the underlying format of an Image
     */
    VkFormat image_get_format(Image*);

    /**
     * @brief transitions the underlying layout of an Image
     */
    void transition_image_layout(Image* image_ptr, VkImageLayout new_layout, int mip = 0);

    /**
     * @brief helper function to return the pixel size for a given VkFormat in bytes
     */
    size_t get_format_pixel_size(VkFormat format);

    /**
     * @brief Begins the copy command buffer
     * 
     * @note is not thread safe, i.e. only one image copy can be in process at once 
     */
    void image_copy_begin();

    /**
     * @brief Reserves 'count' regions in the current copy command queue
     * 
     * @note this must always be called with the exact number of regions you expect to copy, before calling image_copy_image
     */
    void image_copy_reserve_regions(uint32_t count);

    /**
     * @brief Copys srcImage into dstImage with given region
     * 
     * @note must be called between image_copy_begin and image_copy_end
     */
    void image_copy_image(Image* dstImage, Image* srcImage, VkImageCopy region);

    /**
     * @brief Ends the copy command buffer
     */
    void image_copy_end();

    /**
     * @brief Attempts to serialize an Image
     * 
     * @returns bool value indicating success
     */
    bool image_serialize(Image* image_ptr, Serial&);

    /**
     * @brief attempts to load an Image from Serial
     * 
     * @returns Image pointer as if created via `image_create`
     */
    Image* image_from_serial(Serial&);

    /**
     * @brief returns a ImageCreateInfo structure based on the given Image
     */
    ImageCreateInfo image_to_info(Image* image_ptr);

    /**
     * @brief copies a given mip level into CPU memory
     * 
     * @note must call image_free_copied_data when done using data
     */
    void* image_get_data(Image* image_ptr, int mip = 0);

    /**
     * @brief unmaps 
     */
    void image_free_copied_data(void* ptr);
}