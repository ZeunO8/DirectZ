#pragma once
#include "Image.hpp"

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
    void framebuffer_bind(Framebuffer*);

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

    enum class AttachmentType {
        Color,
        Depth,
        DepthStencil,
        Stencil,
        ColorResolve,
        DepthResolve
    };

	enum class BlendFactor
	{
		Zero = 0,
		One = 1,
    	SrcColor = 2,
    	OneMinusSrcColor = 3,
    	DstColor = 4,
    	OneMinusDstColor = 5,
    	SrcAlpha = 6,
    	OneMinusSrcAlpha = 7,
    	DstAlpha = 8,
    	OneMinusDstAlpha = 9,
    	ConstantColor = 10,
    	OneMinusConstantColor = 11,
    	ConstantAlpha = 12,
    	OneMinusConstantAlpha = 13,
    	SrcAlphaSaturate = 14,
    	Src1Color = 15,
    	OneMinusSrc1Color = 16,
    	Src1Alpha = 17,
    	OneMinusSrc1Alpha = 18
	};

	enum class BlendOp
	{
		Add = 0,
		Subtract = 1,
		ReverseSubtract = 2,
		Min = 3,
		Max = 4
	};

	struct BlendState
	{
		bool enable = true;
		BlendFactor srcColor = BlendFactor::SrcAlpha;
		BlendFactor dstColor = BlendFactor::OneMinusSrcColor;
		BlendFactor srcAlpha = BlendFactor::SrcAlpha;
		BlendFactor dstAlpha = BlendFactor::OneMinusSrcAlpha;
		BlendOp colorOp = BlendOp::Add;
        BlendOp alphaOp = BlendOp::Add;
        static BlendState MainFramebuffer;
        static BlendState Layout;
        static BlendState Text;
		static BlendState SrcAlpha;
	};

    struct FramebufferInfo {
        Image** pImages = 0;
        int imagesCount = 0;
        AttachmentType* pAttachmentTypes = 0;
        int attachmentTypesCount = 0;
        BlendState blendState = BlendState::MainFramebuffer;
        bool own_images = false;
    };
}