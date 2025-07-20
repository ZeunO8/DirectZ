#include <AppKit/AppKit.h>
#import <Cocoa/Cocoa.h>
#import <QuartzCore/CAMetalLayer.h>
#import <TargetConditionals.h>
#include <Metal/Metal.h>
#include <ApplicationServices/ApplicationServices.h>
#include <mach-o/dyld.h>
#include <DirectZ.hpp>
#include "Directz.cpp.hpp"
#include "MetalView.mm"
namespace dz
{
	#include "WindowImpl.hpp"
	#include "RendererImpl.hpp"

	void create_surface(Renderer* renderer) {
		auto& window = *renderer->window;
		auto& dr = *get_direct_registry();
		auto& windowType = dr.windowType;

		auto metalLayer = [(MetalView*)window.metalView metalLayer];

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
#include "DZWindow.mm"
namespace dz
{
	#include "path.mm"
	uint8_t get_window_type_platform() {
		return WINDOW_TYPE_MACOS;
	}
	void WINDOW::create_platform() {
		@autoreleasepool {
			if (NSApp == nil) {
				[NSApplication sharedApplication];
				[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
			}

			int32_t windowX = x == -1 ? 128 : x;
			int32_t windowY = y == -1 ? 128 : y;
			NSRect rect = NSMakeRect(windowX, windowY, *width, *height);

			NSUInteger styleMask =
				NSWindowStyleMaskTitled | NSWindowStyleMaskClosable |
				NSWindowStyleMaskResizable | NSWindowStyleMaskMiniaturizable;

			if (borderless) {
				styleMask = NSWindowStyleMaskBorderless;
			}

			nsWindow = [
				[DZWindow alloc] initWithContentRect:rect
				styleMask:styleMask
				backing:NSBackingStoreBuffered
				defer:NO
			];

			DZWindow* dzWindow = (DZWindow*)nsWindow;
			dzWindow->window_ptr = this;

			NSString *nsTitle = [NSString stringWithUTF8String:title.c_str()];
			[dzWindow setTitle:nsTitle];
			[dzWindow setDelegate:[[WINDOWDelegate alloc] initWithWindow:this]];
			[dzWindow makeKeyAndOrderFront:nil];

			NSRect frame = NSMakeRect(0, 0, *width, *height);
			metalView = [[MetalView alloc] initWithFrame:frame];
			[dzWindow setContentView:(MetalView*)metalView];
		}
	}
	bool handle_macos_event(WINDOW& window, NSEvent* event);
	bool WINDOW::poll_events_platform() {
		if (closed)
			return false;
		@autoreleasepool
		{
			NSEvent *event;
			while ((event = [NSApp nextEventMatchingMask:NSEventMaskAny
									untilDate:nil
									inMode:NSDefaultRunLoopMode
									dequeue:YES])) {
				handle_macos_event(*this, event);
				[NSApp sendEvent:event];
				[NSApp updateWindows];
			}
		}
		return true;
	}
	void WINDOW::post_init_platform() {
	}
	void WINDOW::destroy_platform() {
#if defined(MACOS)
		if (nsWindow)
		{
			DZWindow* window = (DZWindow*)nsWindow;
			[window close];
		}
#endif
		nsWindow = nullptr;
		nsView = nullptr;
		nsImage = nullptr;
		nsImageView = nullptr;
		metalView = nullptr;
	}

	bool handle_macos_event(WINDOW& window, NSEvent* event) {
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
				KEYCODES keycode = KEYCODES::NUL;
				if (characters.length > 0)
				{
					keycode = (KEYCODES)[characters characterAtIndex:0];
				}
				*window.mod = mod;
				window.event_interface->key_press(keycode, pressed);
				break;
			}
			case NSEventTypeMouseMoved:
			case NSEventTypeLeftMouseDragged:
			case NSEventTypeRightMouseDragged:
			case NSEventTypeOtherMouseDragged:
			{
				NSPoint location = [event locationInWindow];
				auto x = location.x;
				auto y = ((*window.height) - location.y) - 1;
				window.event_interface->cursor_move(x, y);
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
					window.event_interface->cursor_press(button, true);
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
					window.event_interface->cursor_press(button, false);
				}
				break;
			}
			case NSEventTypeApplicationDefined:
				return false;
			default: break;
		}
		return true;
	}

	void window_set_title(WINDOW* window_ptr, const std::string& new_title) {
		@autoreleasepool
		{
			NSString* title_str = [NSString stringWithUTF8String:new_title.c_str()];
			[(DZWindow*)window_ptr->nsWindow setTitle:title_str];
		}
		window_ptr->title = new_title;
	}

    void window_set_capture(WINDOW* window_ptr, bool should_capture) {
		if (window_ptr->capture == should_capture)
			return;
		if (should_capture)
		{
			DZWindow* nsWindow = (DZWindow*)window_ptr->nsWindow;
			[NSApp preventWindowOrdering];
			[(nsWindow) setIgnoresMouseEvents:NO];
			[[(nsWindow) contentView] addTrackingArea:
				[[NSTrackingArea alloc] initWithRect:[[(nsWindow) contentView] bounds]
											options:NSTrackingMouseEnteredAndExited | NSTrackingMouseMoved | NSTrackingActiveAlways
												owner:(nsWindow)
											userInfo:nil]];
			CGAssociateMouseAndMouseCursorPosition(false);
		}
		else
		{
			CGAssociateMouseAndMouseCursorPosition(true);
		}
		window_ptr->capture = should_capture;
	}

	bool window_get_minimized(WINDOW* window_ptr) {
#if defined(MACOS)
		DZWindow* nsWindow = (DZWindow*)window_ptr;
		return (window_ptr->minimized = [nsWindow isMiniaturized]);
#elif defined(IOS)
		return (window_ptr->minimized = false);
#endif
	}

	void window_set_focused(WINDOW* window_ptr) {
#if defined(MACOS)
		if (!window_ptr || !window_ptr->nsWindow)
			return;
		DZWindow* nsWindow = (DZWindow*)window_ptr->nsWindow;
		[nsWindow makeKeyAndOrderFront:nil];
		[nsWindow makeMainWindow];
		[nsWindow makeKeyWindow];

#elif defined(IOS)
#endif
		*window_ptr->focused = true;
	}

	void window_set_size(WINDOW* window_ptr, float width, float height) {
#if defined(MACOS)
		if (!window_ptr || !window_ptr->nsWindow)
			return;
		DZWindow* nsWindow = (DZWindow*)window_ptr->nsWindow;
		NSRect frame = [nsWindow frame];
		frame.size = NSMakeSize(width, height);
		[nsWindow setFrame:frame display:YES animate:NO];

#elif defined(IOS)
		// iOS does not allow resizing the main window manually
#endif
		*window_ptr->width = width;
		*window_ptr->height = height;
	}

	ImVec2 window_get_position(WINDOW* window_ptr) {
#if defined(MACOS)
		if (!window_ptr || !window_ptr->nsWindow)
			return ImVec2(window_ptr->x, window_ptr->y);

		DZWindow* nsWindow = (DZWindow*)window_ptr->nsWindow;
		NSRect frame = [nsWindow frame];
		window_ptr->x = frame.origin.x;
		window_ptr->y = frame.origin.y;
		return ImVec2(window_ptr->x, window_ptr->y);

#elif defined(IOS)
		return ImVec2(window_ptr->x, window_ptr->y);
#endif
	}

	void window_set_position(WINDOW* window_ptr, float x, float y) {
#if defined(MACOS)
		if (!window_ptr || !window_ptr->nsWindow)
			return;

		DZWindow* nsWindow = (DZWindow*)window_ptr->nsWindow;
		NSRect frame = [nsWindow frame];
		NSRect newFrame = NSMakeRect(x, y, frame.size.width, frame.size.height);
		[nsWindow setFrame:newFrame display:YES animate:NO];

#elif defined(IOS)
#endif
		window_ptr->x = x;
		window_ptr->y = y;
	}
	
    void window_request_drag(WINDOW* window_ptr) {
#if defined(MACOS)
    	[(DZWindow*)window_ptr->nsWindow performWindowDragWithEvent:[NSApp currentEvent]];
#elif defined(IOS)
#endif
	}
	void window_cancel_drag(WINDOW* window_ptr) {
		window_ptr->drag_in_progress = false;
		window_ptr->event_interface->cursor_press(0, false);
	}
}

int dz::displays_get_count()
{
	return (int)[[NSScreen screens] count];
}

dz::DisplayDescription dz::displays_describe(int display_index)
{
	dz::DisplayDescription desc = {};
	NSArray<NSScreen*>* screens = [NSScreen screens];
	if (display_index < 0 || display_index >= [screens count]) return desc;

	NSScreen* screen = [screens objectAtIndex:display_index];
	NSRect frame = [screen frame];
	NSRect visible = [screen visibleFrame];
	CGFloat scale = [screen backingScaleFactor];

	desc.x = (int)frame.origin.x;
	desc.y = (int)frame.origin.y;
	desc.width = (int)frame.size.width;
	desc.height = (int)frame.size.height;
	desc.work_x = (int)visible.origin.x;
	desc.work_y = (int)visible.origin.y;
	desc.work_width = (int)visible.size.width;
	desc.work_height = (int)visible.size.height;
	desc.dpi_scale = (float)scale;

	return desc;
}