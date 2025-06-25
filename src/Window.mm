#include <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#include <Metal/Metal.h>
#include <ApplicationServices/ApplicationServices.h>
#include <mach-o/dyld.h>
#include <DirectZ.hpp>
#include "Directz.cpp.hpp"
namespace dz
{
	#include "WindowImpl.hpp"
	#include "RendererImpl.hpp"

	void create_surface(Renderer* renderer)
	{
		auto& window = *renderer->window;
		auto& dr = *window.registry;
		auto& windowType = dr.windowType;

		CAMetalLayer* metalLayer = (CAMetalLayer*)[(NSView*)window.nsView layer];

		VkMetalSurfaceCreateInfoEXT surfaceCreateInfo{};
		surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_METAL_SURFACE_CREATE_INFO_EXT;
		surfaceCreateInfo.pNext = nullptr;
		surfaceCreateInfo.flags = 0;
		surfaceCreateInfo.pLayer = metalLayer;

		vk_check("vkCreateMetalSurfaceEXT",
			vkCreateMetalSurfaceEXT(dr.instance, &surfaceCreateInfo, nullptr, &renderer->surface));
		// VkMacOSSurfaceCreateInfoMVK surfaceCreateInfo{};
		// surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_HEADLESS_SURFACE_CREATE_INFO_EXT;
		// surfaceCreateInfo.pView = (NSView*)window.nsView;
		// vk_check("vkCreateMacOSSurfaceMVK",
		// 	vkCreateMacOSSurfaceMVK(dr.instance, &surfaceCreateInfo, 0, &renderer->surface));
	}
}
#include "WINDOWDelegateImpl.mm"
namespace dz
{
	#include "path.mm"
	uint8_t get_window_type_platform()
	{
		return WINDOW_TYPE_MACOS;
	}
	void WINDOW::create_platform()
	{
		@autoreleasepool
		{
			if (NSApp == nil)
			{
				[NSApplication sharedApplication];
				[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
			}
			int32_t windowX = x == -1 ? 128 : x,
					windowY = y == -1 ? 128 : y;
			NSRect rect = NSMakeRect(windowX, windowY, *width, *height);
			nsWindow = [[NSWindow alloc] initWithContentRect:rect
								styleMask:(NSWindowStyleMaskTitled |
											NSWindowStyleMaskClosable |
											NSWindowStyleMaskResizable |
											NSWindowStyleMaskMiniaturizable)
								backing:NSBackingStoreBuffered
								defer:NO];
			NSString *nsTitle = [NSString stringWithUTF8String:title.c_str()];
			[(NSWindow*)nsWindow setTitle:nsTitle];
			[(NSWindow*)nsWindow setDelegate:[[WINDOWDelegate alloc] initWithWindow:this]];
			[(NSWindow*)nsWindow makeKeyAndOrderFront:nil];
			nsView = [(NSWindow*)nsWindow contentView];
			NSMenu *mainMenu = [[NSMenu alloc] initWithTitle:nsTitle];
			[NSApp setMainMenu:mainMenu];
			CAMetalLayer* metalLayer = [CAMetalLayer layer];
			metalLayer.device = MTLCreateSystemDefaultDevice();
			metalLayer.pixelFormat = MTLPixelFormatBGRA8Unorm;
			metalLayer.framebufferOnly = YES;
			metalLayer.contentsScale = [(NSView*)nsView window].backingScaleFactor;
			metalLayer.frame = NSMakeRect(0, 0, *width, *height);
			[(NSView*)nsView setWantsLayer:YES];
			[(NSView*)nsView setLayer:metalLayer];
		}
		// nsImage = [[NSImage alloc] initWithSize:NSMakeSize(*width, *height)];
		// NSRect rect = NSMakeRect(0, 0, *width, *height);
		// nsImageView = [[NSImageView alloc] initWithFrame:rect];
		// [(NSView*)nsView addSubview:(NSImageView*)nsImageView];
	}
	bool handle_macos_event(WINDOW& window, NSEvent* event);
	bool WINDOW::poll_events_platform()
	{
		if (closed)
			return false;
		@autoreleasepool
		{
			NSEvent *event;
			while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
									untilDate:nil
									inMode:NSDefaultRunLoopMode
									dequeue:YES]))
			{
				handle_macos_event(*this, event);
				[NSApp sendEvent:event];
				[NSApp updateWindows];
			}
		}
		return true;
	}
	void WINDOW::post_init_platform()
	{
	}
	void WINDOW::destroy_platform()
	{
		// [(NSView*)nsView setLayer:nil];
		if (nsWindow)
			[(NSWindow*)nsWindow release];
	}

	bool handle_macos_event(WINDOW& window, NSEvent* event)
	{
		switch ([event type])
		{
			case NSEventTypeKeyDown:
			case NSEventTypeKeyUp:
			{
				auto pressed = [event type] == NSEventTypeKeyDown;
				int32_t mod = 0;
				NSEventModifierFlags flags = [event modifierFlags];
				mod |= (flags & NSEventModifierFlagControl) ? (1 << 0) : 0;
				mod |= (flags & NSEventModifierFlagShift) ? (1 << 1) : 0;
				mod |= (flags & NSEventModifierFlagOption) ? (1 << 2) : 0;
				mod |= (flags & NSEventModifierFlagCommand) ? (1 << 3) : 0;
				NSString *characters = [event characters];
				uint32_t keycode = 0;
				if (characters.length > 0)
				{
					keycode = [characters characterAtIndex:0];
				}
				*window.mod = mod;
				window.keys.get()[keycode] = pressed;
				break;
			}
			case NSEventTypeMouseMoved:
			case NSEventTypeLeftMouseDragged:
			case NSEventTypeRightMouseDragged:
			case NSEventTypeOtherMouseDragged:
			{
				NSPoint location = [event locationInWindow];
				auto x = location.x;
				auto y = location.y;
				auto cursor = window.cursor.get();
				cursor[0] = x;
				cursor[1] = y;
				break;
			}
			case NSEventTypeLeftMouseDown:
			case NSEventTypeRightMouseDown:
			case NSEventTypeOtherMouseDown:
			{
				auto buttons = window.buttons.get();
				NSInteger button = [event buttonNumber]; // 0 = left, 1 = right, 2+ = middle/extra
				if (button >= 0 && button < 8)
				{
					buttons[button] = true;
				}
				break;
			}
			case NSEventTypeLeftMouseUp:
			case NSEventTypeRightMouseUp:
			case NSEventTypeOtherMouseUp:
			{
				auto buttons = window.buttons.get();
				NSInteger button = [event buttonNumber];
				if (button >= 0 && button < 8)
				{
					buttons[button] = false;
				}
				break;
			}
			case NSEventTypeApplicationDefined:
				return false;
			default: break;
		}
		return true;
	}
}