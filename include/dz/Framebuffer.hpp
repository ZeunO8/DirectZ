#pragma once
#include "Image.hpp"
#include "BlendState.hpp"

namespace dz {

    struct Framebuffer;
    struct FramebufferInfo;

    /**
    * @brief Initializes a Framebuffer given the info provided
    *
    * @returns a Framebuffer pointer for use in framebuffer_X calls
    */
    Framebuffer* framebuffer_create(const FramebufferInfo&);

    /**
    * @brief Sets the clear color of a framebuffer
    */
    void framebuffer_set_clear_color(Framebuffer*, float r = 0.f, float g = 0.f, float b = 0.f, float a = 0.f);

    /**
    * @brief Sets the clear depth of a framebuffer
    */
    void framebuffer_set_clear_depth_stencil(Framebuffer*, float depth = 1.f, uint32_t stencil = 0);

    /**
    * @brief Binds a Framebuffer as the current render target
    */
    void framebuffer_bind(Framebuffer*, bool clear);

    /**
    * @brief Unbinds a Framebuffer
    */
    void framebuffer_unbind(Framebuffer*);

    /**
    * @brief Destroys a Framebuffer, note does not destroy the attached Images
    *  Sets framebuffer to nullptr once destroyed
    *
    * @returns bool value indicating destroy success
    */
    bool framebuffer_destroy(Framebuffer*&);

    /**
    * @brief Resizes a framebuffer and associated Image*
    *
    * @returns bool value indicating resize success
    */
    bool framebuffer_resize(Framebuffer*, uint32_t width, uint32_t height);

    /**
    * @returns bool indicating whether the Framebuffer is about to change
    */
    bool framebuffer_changed(Framebuffer*);

    enum class AttachmentType {
        Color,
        Depth,
        DepthStencil,
        Stencil,
        ColorResolve,
        DepthResolve
    };

    /**
    * @brief attempts to return an underlying Image based on AttachmentType
    *
    * @returns nullptr if not found
    */
    Image* framebuffer_get_image(Framebuffer*, AttachmentType, bool new_image = false);

    struct FramebufferInfo {
        Image** pImages = 0;
        int imagesCount = 0;
        AttachmentType* pAttachmentTypes = 0;
        int attachmentTypesCount = 0;
        BlendState blendState = BlendState::MainFramebuffer;
        bool own_images = false;
    };
}