#include "Directz.cpp.hpp"

namespace dz {
	
    WindowReflectableGroup::WindowReflectableGroup(WINDOW* window_ptr):
		window_ptr(window_ptr) {
		reflectables.push_back(new WindowMetaReflectable(window_ptr));
		reflectables.push_back(new WindowViewportReflectable(window_ptr));
	}

	WindowReflectableGroup::~WindowReflectableGroup() {
		for (auto reflectable : reflectables)
			delete reflectable;
	}
	
	std::string& WindowReflectableGroup::GetName() {
		return window_ptr->title;
	}

	void WindowReflectableGroup::NotifyNameChanged() {
		window_set_title(window_ptr, window_ptr->title);
	}
	
	const std::vector<Reflectable*>& WindowReflectableGroup::GetReflectables() {
		return reflectables;
	}

	WindowMetaReflectable::WindowMetaReflectable(WINDOW* window_ptr):
		window_ptr(window_ptr),
		uid(GlobalUID::GetNew("Reflectable")),
		name("Window Meta") {}

	int WindowMetaReflectable::GetID() {
		return uid;
	}

	std::string& WindowMetaReflectable::GetName() {
		return name;
	}

	void* WindowMetaReflectable::GetVoidPropertyByIndex(int prop_index) {
		switch (prop_index) {
		case 0: return &window_ptr->title;
		default: return nullptr;
		}
	}

	void WindowMetaReflectable::NotifyChange(int prop_index) {
		switch (prop_index) {
		case 0:
			window_set_title(window_ptr, window_ptr->title);
			break;
		default: break;
		}
	}
	
	WindowViewportReflectable::WindowViewportReflectable(WINDOW* window_ptr):
		window_ptr(window_ptr),
		uid(GlobalUID::GetNew("Reflectable")),
		name("Window Viewport") {}

	int WindowViewportReflectable::GetID() {
		return uid;
	}

	std::string& WindowViewportReflectable::GetName() {
		return name;
	}
	
	void* WindowViewportReflectable::GetVoidPropertyByIndex(int prop_index) {
		switch (prop_index) {
		case 0: return &window_ptr->x;
		case 1: return &window_ptr->y;
		case 2: return window_ptr->width.get();
		case 3: return window_ptr->height.get();
		default: return nullptr;
		}
	}

	WINDOW* window_create_internal(WINDOW* window) {
		if (!dr_ptr->root_window)
			dr_ptr->root_window = window;
		dr_ptr->window_count++;
		dr_ptr->window_ptrs.push_back(window);
		dr_ptr->windows_data = dr_ptr->window_ptrs.data();
		dr_ptr->window_reflectable_entries.push_back(new WindowReflectableGroup(window));

		window->event_interface = new EventInterface(window);

	#ifdef __ANDROID__
		if (!dr_ptr->android_asset_manager)
			dr_ptr->android_asset_manager = window->android_asset_manager;
		if (!dr_ptr->android_config)
			AConfiguration_fromAssetManager(dr_ptr->android_config, dr_ptr->android_asset_manager);
	#endif

		auto& width = *window->width;
		auto& height = *window->height;

		window->float_frametime = std::shared_ptr<float>(zmalloc<float>(1, 0), [](float* fp) { zfree(fp, 1); });
		window->float_frametime_ptr = window->float_frametime.get();
		window->double_frametime = std::shared_ptr<double>(zmalloc<double>(1, 0), [](double* dp) { zfree(dp, 1); });
		window->double_frametime_ptr = window->double_frametime.get();
		window->keys = std::shared_ptr<int32_t>(zmalloc<int32_t>(256, 0), [](int32_t* bp) { zfree(bp, 256); });
		window->keys_ptr = window->keys.get();
		window->buttons = std::shared_ptr<int32_t>(zmalloc<int32_t>(8, 0), [](int32_t* bp) { zfree(bp, 8); });
		window->buttons_ptr = window->buttons.get();
		window->cursor = std::shared_ptr<float>(zmalloc<float>(2, 0), [](float* fp) { zfree(fp, 2); });
		window->cursor_ptr = window->cursor.get();
		window->mod = std::shared_ptr<int32_t>(zmalloc<int32_t>(1, 0), [](int32_t* up) { zfree(up, 1); });
		window->mod_ptr = window->mod.get();
		window->focused = std::shared_ptr<int32_t>(zmalloc<int32_t>(1, 0), [](int32_t* ip) { zfree(ip, 1); });
		window->focused_ptr = window->focused.get();
		window->float_iTime = std::shared_ptr<float>(zmalloc<float>(1, 0), [](float* fp) { zfree(fp, 1); });
		window->float_iTime_ptr = window->float_iTime.get();
		window->double_iTime = std::shared_ptr<double>(zmalloc<double>(1, 0), [](double* fp) { zfree(fp, 1); });
		window->double_iTime_ptr = window->double_iTime.get();

		auto& viewport = window->viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = width;
		viewport.height = height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		auto& scissor = window->scissor;
		scissor.offset = {0, 0};
		scissor.extent = {uint32_t(width), uint32_t(height)};

		dr_ptr->imguiLayer.Init();

		auto& headless = window->headless;

		if (headless) {
			if (!window->headless_image) {
				window->headless_image = image_create({
					.width = uint32_t(*window->width_ptr),
					.height = uint32_t(*window->height_ptr),
					.is_framebuffer_attachment = true
				});
			}
		}
		else {
			window->create_platform();
		}

		window->renderer = renderer_init(window);

		if (headless) {
			auto [l, ds] = image_create_descriptor_set(window->headless_image);
			window->headless_ds = ds;
		}

		if (!headless) {
			window->post_init_platform();
		}

		if (!headless && dr_ptr->window_ptrs.size() == 1) {
			ImGuiViewport* main_viewport = ImGui::GetMainViewport();
			main_viewport->PlatformHandleRaw = window_get_native_handle(window);
			main_viewport->PlatformHandle = window;
			main_viewport->Pos.x = window->x;
			main_viewport->Pos.y = window->y;
			main_viewport->Size.x = width;
			main_viewport->Size.y = height;
			window->imguiViewport = main_viewport;
			main_viewport->PlatformWindowCreated = false;
#ifdef _WIN32
			dr_ptr->hwnd_root = window->hwnd;
#endif
		}

		return window;
	}

	WINDOW* window_create_from_serial(Serial& serial) {
		auto window = new WINDOW(serial);
		return window_create_internal(window);
	}

	WINDOW* window_create(const WindowCreateInfo& info) {
		WINDOW* window = nullptr;
		auto id = GlobalUID::GetNew("DZ:WINDOW");
		if (info.create_shared) {
			auto window_shm = new SharedMemoryPtr<WINDOW>();
			auto shm_name = ("WINDOW_" + std::to_string(id));
			replaceAll(shm_name, " ", "_");
			if (!window_shm->Create(shm_name,
				info.title,
				info.x, info.y,
				info.borderless, info.vsync,
				info.width, info.height
	#ifdef ANDROID
				, info.android_window, info.android_asset_manager
	#endif
				, info.headless, info.headless_image
			)) {
				throw std::runtime_error("Unable to create shared WINDOW!");
			}
			window = window_shm->ptr;
			window->window_shm = window_shm;
		}
		else {
			window = new WINDOW(
				info.title,
				info.x, info.y,
				info.borderless, info.vsync,
				info.width, info.height
	#ifdef ANDROID
				, info.android_window, info.android_asset_manager
	#endif
				, info.headless, info.headless_image
			);
		}

		window->id = id;

		return window_create_internal(window);
	}
	
	ImGuiLayer& get_ImGuiLayer() {
		return dr_ptr->imguiLayer;
	}

	void window_add_drawn_buffer_group(WINDOW* window, IDrawListManager* mgr, BufferGroup* buffer_group) {
		window->draw_list_managers[mgr].insert(buffer_group);
	}
	void window_remove_drawn_buffer_group(WINDOW* window, IDrawListManager* mgr, BufferGroup* buffer_group) {
		auto& vect = window->draw_list_managers[mgr];
		vect.erase(buffer_group);
		if (vect.empty())
			window->draw_list_managers.erase(mgr);
	}
	bool window_free(WINDOW* window) {
		bool destroy_remaining = false;
		if (dr_ptr->window_count) {
			auto window_ptrs_end = dr_ptr->window_ptrs.end();
			auto window_ptrs_begin = dr_ptr->window_ptrs.begin();
			auto window_it = std::find(window_ptrs_begin, window_ptrs_end, window);
			if (window_it != window_ptrs_end) {
				auto index = std::distance(window_ptrs_begin, window_it);
				dr_ptr->window_ptrs.erase(window_it);
				dr_ptr->windows_data = dr_ptr->window_ptrs.data();
				dr_ptr->window_reflectable_entries.erase(dr_ptr->window_reflectable_entries.begin() + index);
				destroy_remaining = (dr_ptr->root_window == window);
			}
		}
		dr_ptr->window_count--;
		auto f_d_r = (dr_ptr->window_count == 0);
		if (window->imguiViewport) {
			auto& vp = *window->imguiViewport;
            vp.PlatformHandle = nullptr;
            vp.PlatformHandleRaw = nullptr;
			if (!destroy_remaining)
	            vp.RendererUserData = nullptr;
			vp.PlatformUserData = nullptr;
		}
		window->destroy_platform();
		auto& event_interface = *window->event_interface; 
		for (auto& [priority, window_free_queue] : event_interface.window_free_priorities) {
			while (!window_free_queue.empty()) {
				auto callback = window_free_queue.front();
				window_free_queue.pop();
				callback();
			}
		}
		renderer_free(window->renderer);

		if (window->window_shm) {
			delete window->window_shm;
		} else {
			delete window;
		}

		if (destroy_remaining) {
			for (size_t index = 0; index < dr_ptr->window_ptrs.size();) {
				if (window_free(dr_ptr->window_ptrs[index]) && !f_d_r) {
					f_d_r = true;
				}
			}
		}

		return f_d_r;
	}
    void window_request_close(WINDOW* window_ptr) {
		window_ptr->close_requested = true;
	}
	bool window_poll_events(WINDOW* window) {
		auto poll_continue =
			!window->close_requested &&
			window->poll_events_platform();
		auto now = std::chrono::time_point_cast<std::chrono::nanoseconds>(
			std::chrono::system_clock::now()
		);
		if (window->lastFrame.time_since_epoch().count() == 0)
			window->lastFrame = now;
		else
		{
			std::chrono::nanoseconds steady_duration_ns = now - window->lastFrame;
			std::chrono::duration<double> steady_duration_seconds = steady_duration_ns;
			*window->float_frametime_ptr = *window->double_frametime_ptr = steady_duration_seconds.count();
			*window->float_iTime_ptr += *window->double_frametime;
			*window->double_iTime_ptr += *window->double_frametime;
			window->lastFrame = now;
		}
		if (!poll_continue)
		{
			window->free_begin_of_pass = true;
			dr_ptr->free_window_begin_of_pass = true;
		}
		return poll_continue;
	}

    bool windows_poll_events() {
		size_t stop_poll_count = 0;
		auto original_size = dr_ptr->window_count.load();
		for (size_t index = 0; index < dr_ptr->window_count; index++) {
			if (!window_poll_events(dr_ptr->windows_data[index])) {
				dr_ptr->windows_data[index]->free_begin_of_pass = true;
				stop_poll_count++;
			}
		}
		static bool f_d_r = false;
		if (dr_ptr->free_window_begin_of_pass) {
			for (auto i = 0; i < dr_ptr->window_ptrs.size(); i++) {
				auto window_ptr = dr_ptr->window_ptrs[i];
				if (window_ptr == dr_ptr->root_window) {
					window_ptr->imguiViewport->PlatformHandle = nullptr;
					window_ptr->imguiViewport->PlatformHandleRaw = nullptr;
		        	dr_ptr->imguiLayer.Shutdown(*dr_ptr);
				}
				if (window_ptr->free_begin_of_pass) {
					if (window_free(window_ptr)) {
						free_direct_registry();
						f_d_r = true;
						break;
					}
				}
			}
			if (!f_d_r)
				dr_ptr->free_window_begin_of_pass = false;
		}
		return !f_d_r;
	}

	void window_render(WINDOW* window, bool multi_window_render) {
		for (auto& [priority, shader_dispatches] : window->priority_shader_dispatches) {
			for (auto& [shader, dispatch_fn] : shader_dispatches) {
				auto count = dispatch_fn();
				shader_dispatch(shader, count, 1, 1);
			}
		}
		if (!window->minimized || !window_get_minimized(window))
			renderer_render(window->renderer);
	}

	bool windows_render() {
		for (size_t index = 0; index < dr_ptr->window_count; index++)
			window_render(dr_ptr->windows_data[index], true);
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(nullptr, nullptr);
		for (auto i = 0; i < dr_ptr->window_count; i++) {
			auto window_ptr = dr_ptr->windows_data[i];
			if (!window_ptr->free_begin_of_pass && window_ptr->renderer->recreate_swapchain_deferred) {
				recreate_swap_chain(window_ptr->renderer);
				window_ptr->renderer->recreate_swapchain_deferred = false;
			}
		}
		return false;
	}

    const std::string& window_get_title_ref(WINDOW* window) {
		return window->title;
	}
#if defined(_WIN32) || (defined(__linux__) && !defined(ANDROID))
    void window_set_title(WINDOW* window_ptr, const std::string& new_title) {
#if defined(_WIN32)
		SetWindowTextA(window_ptr->hwnd, new_title.c_str());
#elif (defined(__linux__) && !defined(ANDROID))
		if (!window_ptr || !window_ptr->display || !window_ptr->window)
			return;

		Window win = window_ptr->window;
		Display* display = window_ptr->display;

		// Set WM_NAME property (legacy)
		XStoreName(display, win, new_title.c_str());

		// Set _NET_WM_NAME for modern desktop environments (UTF-8)
		Atom net_wm_name = XInternAtom(display, "_NET_WM_NAME", False);
		Atom utf8_string = XInternAtom(display, "UTF8_STRING", False);

		if (net_wm_name != None && utf8_string != None)
		{
			XChangeProperty(
				display,
				win,
				net_wm_name,
				utf8_string,
				8,                          // format: 8-bit
				PropModeReplace,
				reinterpret_cast<const unsigned char*>(new_title.c_str()),
				static_cast<int>(new_title.size())
			);
		}

		XFlush(display);
#endif
		window_ptr->title = new_title;
	}
#endif
#ifdef ANDROID
    void window_set_title(WINDOW* window_ptr, const std::string& new_title) {
		window_ptr->title = new_title;
	}
#endif
	size_t window_get_id_ref(WINDOW* window) {
		return window->id;
	}
	float& window_get_float_frametime_ref(WINDOW* window) {
		return *window->float_frametime;
	}
	double& window_get_double_frametime_ref(WINDOW* window) {
		return *window->double_frametime;
	}
	void window_set_float_frametime_pointer(WINDOW* window, float* pointer) {
		window_set_float_frametime_pointer(window, std::shared_ptr<float>(pointer, [](auto p){}));
	}
	void window_set_double_frametime_pointer(WINDOW* window, double* pointer) {
		window_set_double_frametime_pointer(window, std::shared_ptr<double>(pointer, [](auto p){}));
	}
	void window_set_keys_pointer(WINDOW* window, int32_t* pointer) {
		window_set_keys_pointer(window, std::shared_ptr<int32_t>(pointer, [](auto p){}));
	}
	void window_set_buttons_pointer(WINDOW* window, int32_t* pointer) {
		window_set_buttons_pointer(window, std::shared_ptr<int32_t>(pointer, [](auto p){}));
	}
	void window_set_cursor_pointer(WINDOW* window, float* pointer) {
		window_set_cursor_pointer(window, std::shared_ptr<float>(pointer, [](auto p){}));
	}
	void window_set_mod_pointer(WINDOW* window, int32_t* pointer) {
		window_set_mod_pointer(window, std::shared_ptr<int32_t>(pointer, [](auto p){}));
	}
	void window_set_focused_pointer(WINDOW* window, int32_t* pointer) {
		window_set_focused_pointer(window, std::shared_ptr<int32_t>(pointer, [](auto p){}));
	}
	void window_set_width_pointer(WINDOW* window, float* pointer) {
		window_set_width_pointer(window, std::shared_ptr<float>(pointer, [](auto p){}));
	}
	void window_set_height_pointer(WINDOW* window, float* pointer) {
		window_set_height_pointer(window, std::shared_ptr<float>(pointer, [](auto p){}));
	}
	void window_set_float_frametime_pointer(WINDOW* window, const std::shared_ptr<float>& pointer) {
		auto old_float_frametime = *window->float_frametime;
		window->float_frametime = pointer;
		*window->float_frametime = old_float_frametime;
		window->float_frametime_ptr = window->float_frametime_ptr;
	}
	void window_set_double_frametime_pointer(WINDOW* window, const std::shared_ptr<double>& pointer) {
		auto old_double_frametime = *window->double_frametime;
		window->double_frametime = pointer;
		*window->double_frametime = old_double_frametime;
		window->double_frametime_ptr = window->double_frametime_ptr;
	}
	void window_set_keys_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer) {
		auto old_keys = window->keys;
		window->keys = pointer;
		memcpy(window->keys.get(), old_keys.get(), 256 * sizeof(int32_t));
		window->keys_ptr = window->keys.get();
	}
	void window_set_buttons_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer) {
		auto old_buttons = window->buttons;
		window->buttons = pointer;
		memcpy(window->buttons.get(), old_buttons.get(), 8 * sizeof(int32_t));
		window->buttons_ptr = window->buttons.get();
	}
	void window_set_cursor_pointer(WINDOW* window, const std::shared_ptr<float>& pointer) {
		auto old_cursor = window->cursor;
		window->cursor = pointer;
		memcpy(window->cursor.get(), old_cursor.get(), 2 * sizeof(float));
		window->cursor_ptr = window->cursor.get();
	}
	void window_set_mod_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer) {
		auto old_mod = *window->mod;
		window->mod = pointer;
		*window->mod = old_mod;
		window->mod_ptr = window->mod.get();
	}
	void window_set_focused_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer) {
		auto old_focused = *window->focused;
		window->focused = pointer;
		*window->focused = old_focused;
		window->focused_ptr = window->focused.get();
	}
	void window_set_width_pointer(WINDOW* window, const std::shared_ptr<float>& pointer) {
		auto old_width = *window->width;
		window->width = pointer;
		*window->width = old_width;
		window->width_ptr = window->width.get();
	}
	void window_set_height_pointer(WINDOW* window, const std::shared_ptr<float>& pointer) {
		auto old_height = *window->height;
		window->height = pointer;
		*window->height = old_height;
		window->height_ptr = window->height.get();
	}
    void window_set_float_iTime_pointer(WINDOW* window, const std::shared_ptr<float>& pointer) {
		auto old_float_iTime = *window->float_iTime;
		window->float_iTime = pointer;
		*window->float_iTime = old_float_iTime;
		window->float_iTime_ptr = window->float_iTime.get();
	}
    void window_set_double_iTime_pointer(WINDOW* window, const std::shared_ptr<double>& pointer) {
		auto old_double_iTime = *window->double_iTime;
		window->double_iTime = pointer;
		*window->double_iTime = old_double_iTime;
		window->double_iTime_ptr = window->double_iTime.get();
	}

	int32_t& window_get_keypress_ref(WINDOW* window, uint8_t keycode) {
		return window->keys.get()[keycode];
	}

    int32_t& window_get_keypress_ref(WINDOW* window, KEYCODES keycode) {
		return window_get_keypress_ref(window, (uint8_t)keycode);
	}

	std::shared_ptr<int32_t>& window_get_all_keypress_ref(WINDOW* window, uint8_t keycode) {
		return window->keys;
	}
	int32_t& window_get_buttonpress_ref(WINDOW* window, uint8_t button) {
		return window->buttons.get()[button];
	}
	std::shared_ptr<int32_t>& window_get_all_buttonpress_ref(WINDOW* window, uint8_t button) {
		return window->buttons;
	}
	std::shared_ptr<float>& window_get_width_ref(WINDOW* window) {
		return window->width;
	}
	std::shared_ptr<float>& window_get_height_ref(WINDOW* window) {
		return window->height;
	}
    std::shared_ptr<float>& window_get_float_iTime_ref(WINDOW* window) {
		return window->float_iTime;
	}
    std::shared_ptr<double>& window_get_double_iTime_ref(WINDOW* window) {
		return window->double_iTime;
	}

	#ifdef _WIN32
	LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

	uint8_t get_window_type_platform() {
		return WINDOW_TYPE_WIN32;
	}
	void WINDOW::create_platform() {
		if (!setDPIAware)
		{
			HRESULT hr = SetProcessDPIAware();
			if (FAILED(hr))
			{
				throw std::runtime_error("SetProcessDpiAwareness failed");
			}
			setDPIAware = true;
		}
		hInstance = GetModuleHandle(NULL);
		WNDCLASSEXA wc = {0};
		// wc.cbSize = sizeof(WNDCLASS);
		wc.cbSize = sizeof(WNDCLASSEXA);
		wc.style = CS_VREDRAW | CS_HREDRAW;
		wc.lpfnWndProc = wndproc;
		wc.hInstance = hInstance;
		wc.lpszClassName = title.c_str();
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		RegisterClassExA(&wc);
		dpiScale = 1.0f;
		HDC screen = GetDC(NULL);
		int32_t dpi = GetDeviceCaps(screen, LOGPIXELSX);
		ReleaseDC(NULL, screen);
		dpiScale = dpi / 96.0f;
		int adjustedWidth = *width, adjustedHeight = *height;
		auto wsStyle = WS_OVERLAPPEDWINDOW;
		RECT desiredRect = {0, 0, adjustedWidth, adjustedHeight};
		auto exStyle = WS_EX_APPWINDOW;
		AdjustWindowRectEx(&desiredRect, wsStyle, FALSE, exStyle);
		adjustedWidth = desiredRect.right - desiredRect.left;
		adjustedHeight = desiredRect.bottom - desiredRect.top;
		hwnd = CreateWindowExA(
			exStyle,
			title.c_str(),
			title.c_str(),
			wsStyle,
			x == -1 ? CW_USEDEFAULT : x,
			y == -1 ? CW_USEDEFAULT : y,
			adjustedWidth,
			adjustedHeight,
			0,
			NULL,
			hInstance,
			this
		);

		if (hwnd == NULL)
			throw std::runtime_error("Failed to create window");
		SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)this);
		if (borderless)
		{
			SetWindowLong(
				hwnd,
				GWL_STYLE,
				(GetWindowLong(hwnd, GWL_STYLE) & ~WS_CAPTION & ~WS_THICKFRAME & ~WS_SYSMENU) | WS_MINIMIZEBOX |
				WS_MAXIMIZEBOX
			);
			SetWindowLong(hwnd, GWL_EXSTYLE, GetWindowLong(hwnd, GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
			UINT flags = SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW;
			if (x == -1 && y == -1)
				flags |= SWP_NOMOVE;
			SetWindowPos(
				hwnd,
				HWND_TOPMOST,
				(x == -1 ? 0 : x), // Use explicit or default X position
				(y == -1 ? 0 : y), *width,
				*height,
				flags
			);
		}
		// hDeviceContext = GetDC(hwnd);
		// SetupPixelFormat(hDeviceContext);
	}
	bool WINDOW::poll_events_platform() {
		if (!headless) {
			static MSG msg = {};
			while (PeekMessageA(&msg, 0, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_QUIT)
					return false;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
		return true;
	}
	void WINDOW::destroy_platform() {
		if (hwnd)
		{
			DestroyWindow(hwnd);
			hwnd = nullptr;
		}
	}
	void WINDOW::post_init_platform() {
		ShowWindow(hwnd, SW_NORMAL);
		UpdateWindow(hwnd);
		RECT rect;
		if (GetWindowRect(hwnd, &rect))
		{
			x = rect.left;
			y = rect.top;
		}
	}
	#elif defined(__linux__) && !defined(__ANDROID__)
	uint8_t get_window_type_platform() {
		return WINDOW_TYPE_X11;
	}

	void WINDOW::create_platform() {
		display = XOpenDisplay(nullptr);
		if (!display)
		{
			throw std::runtime_error("Failed to open Xlib display!");
		}

		screenNumber = DefaultScreen(display);
		Screen* scr = ScreenOfDisplay(display, screenNumber);
		root = RootWindow(display, screenNumber);
		int black = BlackPixel(display, screenNumber);
		int white = WhitePixel(display, screenNumber);

		window = XCreateSimpleWindow(display, root,
									x, y,
									static_cast<unsigned int>(*width),
									static_cast<unsigned int>(*height),
									1,
									black,
									white);
		if (borderless) {
			Atom mwmHintsProperty = XInternAtom(display, "_MOTIF_WM_HINTS", False);
			struct MotifWmHints
			{
				unsigned long flags;
				unsigned long functions;
				unsigned long decorations;
				long inputMode;
				unsigned long status;
			} hints = { 0 };

			hints.flags = 2;            // MWM_HINTS_DECORATIONS
			hints.decorations = 0;     // 0 = no decorations (borderless)

			XChangeProperty(display,
							window,
							mwmHintsProperty,
							mwmHintsProperty,
							32,
							PropModeReplace,
							reinterpret_cast<unsigned char*>(&hints),
							5);
		}

		XStoreName(display, window, title.c_str());

		Atom wmDelete = XInternAtom(display, "WM_DELETE_WINDOW", False);
		wm_delete_window = wmDelete;
		XSetWMProtocols(display, window, &wmDelete, 1);

		XSelectInput(display, window,
					ExposureMask |
					KeyPressMask | KeyReleaseMask |
					ButtonPressMask | ButtonReleaseMask |
					PointerMotionMask |
					StructureNotifyMask |
					FocusChangeMask |
					EnterWindowMask | LeaveWindowMask);

		XMapWindow(display, window);
		XFlush(display);

		initAtoms();
	}
	void WINDOW::initAtoms() {
		wm_protocols = XInternAtom(display, "WM_PROTOCOLS", False);
		wm_delete_window = XInternAtom(display, "WM_DELETE_WINDOW", False);
		atom_net_wm_state = XInternAtom(display, "_NET_WM_STATE", False);
		atom_net_wm_state_hidden = XInternAtom(display, "_NET_WM_STATE_HIDDEN", False);
		atom_net_wm_state_maximized_horz = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_HORZ", False);
		atom_net_wm_state_maximized_vert = XInternAtom(display, "_NET_WM_STATE_MAXIMIZED_VERT", False);
	}
	bool handle_x11_event(WINDOW& window, XEvent& event);
	bool WINDOW::poll_events_platform()
	{
		bool return_ = true;
		while (XPending(display))
		{
			XEvent event;
			XNextEvent(display, &event);
			if (!handle_x11_event(*this, event))
			{
				return_ = false;
				break;
			}
		}
		return return_;
	}
	void WINDOW::post_init_platform() {
	}
	void WINDOW::destroy_platform() {
		if (display && window) {
			XDestroyWindow(display, window);
			XCloseDisplay(display);
			display = nullptr;
			window = 0;
		}
	}
	#elif defined(__ANDROID__)
	uint8_t get_window_type_platform() {
		return WINDOW_TYPE_ANDROID;
	}
	void WINDOW::create_platform() {
	}
	bool handle_android_event(WINDOW& window);
	bool WINDOW::poll_events_platform() {
		return true;
	}
	void WINDOW::post_init_platform() {
	}
	void WINDOW::destroy_platform() {
    	android_window = nullptr;
	}
	#endif


	#ifdef _WIN32
	KEYCODES GetCorrectedKeycode(KEYCODES keycode, bool shift);
	LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		struct WINDOW* window_ptr = (WINDOW*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
		auto& window = *window_ptr;
		switch (msg)
		{
		case WM_CREATE:
			{
				CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
				window_ptr = (WINDOW*)createStruct->lpCreateParams;
				SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window_ptr);
				break;
			};
		case WM_CLOSE:
			DestroyWindow(hwnd);
			break;
		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
		case WM_MBUTTONDOWN:
		case WM_MBUTTONUP:
			{
				auto pressed = msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN;
				auto button = 0;
				switch (msg)
				{
				case WM_LBUTTONDOWN:
				case WM_LBUTTONUP:
					{
						button = 0;
						break;
					};
				case WM_RBUTTONDOWN:
				case WM_RBUTTONUP:
					{
						button = 1;
						break;
					};
				case WM_MBUTTONDOWN:
				case WM_MBUTTONUP:
					{
						button = 2;
						break;
					};
				}
				window.event_interface->cursor_press(button, pressed);
				break;
			};
		case WM_MOUSEWHEEL:
			{
				int zDelta = GET_WHEEL_DELTA_WPARAM(wParam); // This gives the scroll amount
				auto wheelButton = zDelta > 0 ? 3 : 4; // Wheel scrolled up or down
				window.event_interface->cursor_press(wheelButton, true);
				break;
			};
		case WM_XBUTTONDOWN:
		case WM_XBUTTONUP:
			{
				WORD button = GET_XBUTTON_WPARAM(wParam);
				auto xButton = (button == XBUTTON2 ? 5 : (button == XBUTTON1 ? 6 : -1));
				if (xButton == -1)
					throw std::runtime_error("Invalid XButton");
				auto pressed = msg == WM_XBUTTONDOWN;
				window.event_interface->cursor_press(xButton, pressed);
				break;
			};
		case WM_MOUSEMOVE:
			{
				POINT pt;
				GetCursorPos(&pt);
				ScreenToClient(hwnd, &pt);
				auto x = pt.x;
				auto y = pt.y;
				window.event_interface->cursor_move(x, y);
				break;
			};
		case WM_SYSKEYDOWN:
		case WM_SYSKEYUP:
			if (wParam == VK_MENU ||
				wParam == VK_LMENU ||
				wParam == VK_RMENU)
			{
				window.event_interface->key_press(KEYCODES::ALT, (msg == WM_SYSKEYDOWN));
			}
			else
			{
				return DefWindowProc(hwnd, msg, wParam, lParam);
			}
			break;
		case WM_KEYDOWN:
		case WM_KEYUP:
			{
				if (msg == WM_KEYDOWN)
				{
					// Check bit 30 of lParam: 1 if key was already down (i.e., autorepeat), 0 if it was previously up
					bool isAutoRepeat = (lParam & (1 << 30)) != 0;

					if (isAutoRepeat) {
						goto _default;
					}
				}
				auto mod = ((GetKeyState(VK_CONTROL) & 0x8000) >> 15) | ((GetKeyState(VK_SHIFT) & 0x8000) >> 14) |
					((GetKeyState(VK_MENU) & 0x8000) >> 13) | (((GetKeyState(VK_LWIN) | GetKeyState(VK_RWIN)) & 0x8000) >> 12);
				auto keycodeHiword = HIWORD(lParam) & 0x1ff;
				if (keycodeHiword < 0 || keycodeHiword > sizeof(WIN_MAP_KEYCODES) / sizeof(WIN_MAP_KEYCODES[0]))
				{
					break;
				}
				auto keycode = WIN_MAP_KEYCODES[keycodeHiword];
				auto keypress = !((lParam >> 31) & 1);
				auto shift = mod & 2;
				keycode = GetCorrectedKeycode(keycode, shift);
				*window.mod = mod;
				window.event_interface->key_press(keycode, keypress);
			}
			break;
		case WM_DESTROY:
			if (hwnd == dr_ptr->hwnd_root)
				PostQuitMessage(0);
			else
				return 0;
			break;
		case WM_MOVE:
			{
				window.x = (float)(short)LOWORD(lParam);
				window.y = (float)(short)HIWORD(lParam);
				break;
			}
		case WM_ENTERSIZEMOVE:
    		window.drag_in_progress = true;
    		break;
		case WM_EXITSIZEMOVE:
			window_cancel_drag(&window);
			break;
		case WM_SIZE:
			{
				int32_t width = LOWORD(lParam), height = HIWORD(lParam);
				if (width != 0 && width != *window.width && height != 0 && height != *window.height)
				{
					*window.width = width;
					*window.height = height;
				}
				break;
			};
		case WM_SETFOCUS:
			{
				window_set_focused(&window, true);
				break;
			};
		case WM_KILLFOCUS:
			{
				window_set_focused(&window, false);
				break;
			};
		default:
		_default:
			return DefWindowProc(hwnd, msg, wParam, lParam);
		}
		return 0;
	}
	KEYCODES GetCorrectedKeycode(KEYCODES keycode, bool shift) {
        switch (keycode)
        {
            case KEYCODES::ESCAPE:
            case KEYCODES::Delete:
            case KEYCODES::UP:
            case KEYCODES::DOWN:
            case KEYCODES::LEFT:
            case KEYCODES::RIGHT:
            case KEYCODES::HOME:
            case KEYCODES::END:
            case KEYCODES::PGUP:
            case KEYCODES::PGDOWN:
            case KEYCODES::INSERT:
            case KEYCODES::NUMLOCK:
            case KEYCODES::CAPSLOCK:
            case KEYCODES::CTRL:
            case KEYCODES::SHIFT:
            case KEYCODES::ALT:
            case KEYCODES::PAUSE:
            case KEYCODES::SUPER:
			case KEYCODES::BACKSPACE:
			case KEYCODES::TAB:
			case KEYCODES::ENTER:
			case KEYCODES::SPACE:
				return keycode;

            case KEYCODES::SINGLEQUOTE: return shift ? KEYCODES::DOUBLEQUOTE : KEYCODES::SINGLEQUOTE;
            case KEYCODES::COMMA: return shift ? KEYCODES::LESSTHAN : KEYCODES::COMMA;
            case KEYCODES::MINUS: return shift ? KEYCODES::UNDERSCORE : KEYCODES::MINUS;
            case KEYCODES::PERIOD: return shift ? KEYCODES::GREATERTHAN : KEYCODES::PERIOD;
            case KEYCODES::SLASH: return shift ? KEYCODES::QUESTIONMARK : KEYCODES::SLASH;
            case KEYCODES::SEMICOLON: return shift ? KEYCODES::COLON : KEYCODES::SEMICOLON;
            case KEYCODES::LEFTBRACKET: return shift ? KEYCODES::LEFTBRACE : KEYCODES::LEFTBRACKET;
            case KEYCODES::BACKSLASH: return shift ? KEYCODES::VERTICALBAR : KEYCODES::BACKSLASH;
            case KEYCODES::RIGHTBRACKET: return shift ? KEYCODES::RIGHTBRACE : KEYCODES::RIGHTBRACKET;
			
            case KEYCODES::GRAVEACCENT: return shift ? KEYCODES::TILDE : KEYCODES::GRAVEACCENT;
            case KEYCODES::EQUAL: return shift ? KEYCODES::PLUS : KEYCODES::EQUAL;

            case KEYCODES::_0: return shift ? KEYCODES::RIGHTPARENTHESIS : KEYCODES::_0;
            case KEYCODES::_1: return shift ? KEYCODES::EXCLAMATION : KEYCODES::_1;
            case KEYCODES::_2: return shift ? KEYCODES::ATSIGN : KEYCODES::_2;
            case KEYCODES::_3: return shift ? KEYCODES::HASHTAG : KEYCODES::_3;
            case KEYCODES::_4: return shift ? KEYCODES::DOLLAR : KEYCODES::_4;
            case KEYCODES::_5: return shift ? KEYCODES::PERCENTSIGN : KEYCODES::_5;
            case KEYCODES::_6: return shift ? KEYCODES::CARET : KEYCODES::_6;
            case KEYCODES::_7: return shift ? KEYCODES::AMPERSAND : KEYCODES::_7;
            case KEYCODES::_8: return shift ? KEYCODES::ASTERISK : KEYCODES::_8;
            case KEYCODES::_9: return shift ? KEYCODES::LEFTPARENTHESIS : KEYCODES::_9;

            case KEYCODES::A:	return shift ? KEYCODES::A : KEYCODES::a;
            case KEYCODES::C:	return shift ? KEYCODES::C : KEYCODES::c;
            case KEYCODES::B:	return shift ? KEYCODES::B : KEYCODES::b;
            case KEYCODES::D:	return shift ? KEYCODES::D : KEYCODES::d;
            case KEYCODES::E:	return shift ? KEYCODES::E : KEYCODES::e;
            case KEYCODES::F:	return shift ? KEYCODES::F : KEYCODES::f;
            case KEYCODES::G:	return shift ? KEYCODES::G : KEYCODES::g;
            case KEYCODES::H:	return shift ? KEYCODES::H : KEYCODES::h;
            case KEYCODES::I:	return shift ? KEYCODES::I : KEYCODES::i;
            case KEYCODES::J:	return shift ? KEYCODES::J : KEYCODES::j;
            case KEYCODES::K:	return shift ? KEYCODES::K : KEYCODES::k;
            case KEYCODES::L:	return shift ? KEYCODES::L : KEYCODES::l;
            case KEYCODES::M:	return shift ? KEYCODES::M : KEYCODES::m;
            case KEYCODES::N:	return shift ? KEYCODES::N : KEYCODES::n;
            case KEYCODES::O:	return shift ? KEYCODES::O : KEYCODES::o;
            case KEYCODES::P:	return shift ? KEYCODES::P : KEYCODES::p;
            case KEYCODES::Q:	return shift ? KEYCODES::Q : KEYCODES::q;
            case KEYCODES::R:	return shift ? KEYCODES::R : KEYCODES::r;
            case KEYCODES::S:	return shift ? KEYCODES::S : KEYCODES::s;
            case KEYCODES::T:	return shift ? KEYCODES::T : KEYCODES::t;
            case KEYCODES::U:	return shift ? KEYCODES::U : KEYCODES::u;
            case KEYCODES::V:	return shift ? KEYCODES::V : KEYCODES::v;
            case KEYCODES::W:	return shift ? KEYCODES::W : KEYCODES::w;
            case KEYCODES::X:	return shift ? KEYCODES::X : KEYCODES::x;
            case KEYCODES::Y:	return shift ? KEYCODES::Y : KEYCODES::y;
            case KEYCODES::Z:	return shift ? KEYCODES::Z : KEYCODES::z;

            case KEYCODES::a:	return shift ? KEYCODES::A : KEYCODES::a; 
            case KEYCODES::c:	return shift ? KEYCODES::C : KEYCODES::c; 
            case KEYCODES::b:	return shift ? KEYCODES::B : KEYCODES::b; 
            case KEYCODES::d:	return shift ? KEYCODES::D : KEYCODES::d; 
            case KEYCODES::e:	return shift ? KEYCODES::E : KEYCODES::e; 
            case KEYCODES::f:	return shift ? KEYCODES::F : KEYCODES::f; 
            case KEYCODES::g:	return shift ? KEYCODES::G : KEYCODES::g; 
            case KEYCODES::h:	return shift ? KEYCODES::H : KEYCODES::h; 
            case KEYCODES::i:	return shift ? KEYCODES::I : KEYCODES::i; 
            case KEYCODES::j:	return shift ? KEYCODES::J : KEYCODES::j; 
            case KEYCODES::k:	return shift ? KEYCODES::K : KEYCODES::k; 
            case KEYCODES::l:	return shift ? KEYCODES::L : KEYCODES::l; 
            case KEYCODES::m:	return shift ? KEYCODES::M : KEYCODES::m; 
            case KEYCODES::n:	return shift ? KEYCODES::N : KEYCODES::n; 
            case KEYCODES::o:	return shift ? KEYCODES::O : KEYCODES::o; 
            case KEYCODES::p:	return shift ? KEYCODES::P : KEYCODES::p; 
            case KEYCODES::q:	return shift ? KEYCODES::Q : KEYCODES::q; 
            case KEYCODES::r:	return shift ? KEYCODES::R : KEYCODES::r; 
            case KEYCODES::s:	return shift ? KEYCODES::S : KEYCODES::s; 
            case KEYCODES::t:	return shift ? KEYCODES::T : KEYCODES::t; 
            case KEYCODES::u:	return shift ? KEYCODES::U : KEYCODES::u; 
            case KEYCODES::v:	return shift ? KEYCODES::V : KEYCODES::v; 
            case KEYCODES::w:	return shift ? KEYCODES::W : KEYCODES::w; 
            case KEYCODES::x:	return shift ? KEYCODES::X : KEYCODES::x; 
            case KEYCODES::y:	return shift ? KEYCODES::Y : KEYCODES::y; 
            case KEYCODES::z:	return shift ? KEYCODES::Z : KEYCODES::z; 

            default:
                return keycode;
        }
	}
	#elif defined(__linux__) && !defined(__ANDROID__)
	bool handle_x11_event(WINDOW& window, XEvent& event) {
		switch (event.type)
		{
			case Expose:
			{
				break;
			}
			case KeyPress:
			case KeyRelease:
			{
				bool pressed = event.type == KeyPress;
				XKeyEvent& keyEvent = event.xkey;

				KeySym keysym = XkbKeycodeToKeysym(window.display, keyEvent.keycode, 0, (keyEvent.state & ShiftMask) ? 1 : 0);
				uint32_t keycode_utf = xkb_keysym_to_utf32(keysym);
				KEYCODES keycode = (KEYCODES)keycode_utf;

				int32_t mod = 0;
				if (keycode == KEYCODES::NUL)
				{
					switch (keysym) {
						case XK_Up: keycode = KEYCODES::UP; break;
						case XK_Down: keycode = KEYCODES::DOWN; break;
						case XK_Left: keycode = KEYCODES::LEFT; break;
						case XK_Right: keycode = KEYCODES::RIGHT; break;
						case XK_Home: keycode = KEYCODES::HOME; break;
						case XK_End: keycode = KEYCODES::END; break;
						case XK_Page_Up: keycode = KEYCODES::PGUP; break;
						case XK_Page_Down: keycode = KEYCODES::PGDOWN; break;
						case XK_Insert: keycode = KEYCODES::INSERT; break;
						case XK_Num_Lock: keycode = KEYCODES::NUMLOCK; break;
						case XK_Caps_Lock: keycode = KEYCODES::CAPSLOCK; break;
						case XK_Pause: keycode = KEYCODES::PAUSE; break;
						case XK_Super_L:
						case XK_Super_R: keycode = KEYCODES::SUPER; break;
						default: keycode = KEYCODES::NUL; break;
					}
				}
				*window.mod = mod;
				window.event_interface->key_press(keycode, pressed);
				break;
			}
			case ClientMessage:
			{
				if ((Atom)event.xclient.data.l[0] == window.wm_delete_window)
				{
					return false;
				}
				break;
			}
			case MotionNotify:
			{
				window.event_interface->cursor_move(event.xmotion.x, event.xmotion.y);
				break;
			}
			case ButtonPress:
			{
				int button = event.xbutton.button - 1;
				window.event_interface->cursor_press(button, true);
				break;
			}
			case ButtonRelease:
			{
				int button = event.xbutton.button - 1;
				if (button == 3 || button == 4) break;
				window.event_interface->cursor_press(button, false);
				if (window.drag_in_progress && button == 0)
				{
					window_cancel_drag(&window);
					// reset_event_mask(&window);
					XUngrabPointer(window.display, CurrentTime);
					XFlush(window.display);
				}
				break;
			}
			case FocusIn:
			{
				window_set_focused(&window, true);
				break;
			}
			case FocusOut:
			{
				window_set_focused(&window, false);
				break;
			}
			case DestroyNotify:
			{
				return false;
			}
			default:
			{
				break;
			}
		}
		return true;
	}
	#endif

	EventInterface* window_get_event_interface(WINDOW* window) {
		return window->event_interface;
	}

    void window_register_free_callback(WINDOW* window, float priority, const dz::function<void()>& callback) {
		window->event_interface->window_free_priorities[priority].push(callback);
	}

#if defined(_WIN32) || defined(__linux__)
    void window_set_capture(WINDOW* window_ptr, bool should_capture) {
		if (window_ptr->capture == should_capture)
			return;
#if defined(_WIN32)
		if (should_capture)
		{
			SetCapture(window_ptr->hwnd);
		}
		else
		{
			ReleaseCapture();
		}
#elif defined(__linux__) && !defined(ANDROID)
		if (should_capture)
		{
			XGrabPointer(window_ptr->display, window_ptr->window, True,
						ButtonPressMask | ButtonReleaseMask | PointerMotionMask,
						GrabModeAsync, GrabModeAsync, None, None, CurrentTime);
		}
		else
		{
			XUngrabPointer(window_ptr->display, CurrentTime);
		}
		XFlush(window_ptr->display);
#endif
		window_ptr->capture = should_capture;
	}
#endif

    void* window_get_native_handle(WINDOW* window_ptr) {
#if defined(_WIN32)
		return (void*)window_ptr->hwnd;
#elif defined(__linux__) && !defined(ANDROID)
		return (void*)(uintptr_t)(window_ptr->window);
#elif defined(ANDROID)
		return (void*)window_ptr->android_window;
#elif defined(__APPLE__)
		return (void*)window_ptr->nsWindow;
#endif
	}

	struct ImGui_ImplVulkan_FrameRenderBuffers
	{
		VkDeviceMemory      VertexBufferMemory;
		VkDeviceMemory      IndexBufferMemory;
		VkDeviceSize        VertexBufferSize;
		VkDeviceSize        IndexBufferSize;
		VkBuffer            VertexBuffer;
		VkBuffer            IndexBuffer;
	};

	struct ImGui_ImplVulkan_WindowRenderBuffers
	{
		uint32_t            Index;
		uint32_t            Count;
		ImVector<ImGui_ImplVulkan_FrameRenderBuffers> FrameRenderBuffers;
	};

	struct ImGui_ImplVulkan_ViewportData
	{
		ImGui_ImplVulkanH_Window                Window;
		ImGui_ImplVulkan_WindowRenderBuffers    RenderBuffers;
		bool                                    WindowOwned;
		bool                                    SwapChainNeedRebuild;
		bool                                    SwapChainSuboptimal;
	};

#if defined(_WIN32) || defined(__linux__)
	bool window_get_minimized(WINDOW* window_ptr) {
#if defined(_WIN32)
		return (window_ptr->minimized = IsIconic(window_ptr->hwnd));
#elif defined(__linux) && !defined(ANDROID)
		Atom wm_state_atom = XInternAtom(window_ptr->display, "WM_STATE", True);
		if (wm_state_atom == None)
			return (window_ptr->minimized = false);

		Atom actual_type;
		int actual_format;
		unsigned long nitems;
		unsigned long bytes_after;
		unsigned char* prop = nullptr;

		int status = XGetWindowProperty(
			window_ptr->display,
			window_ptr->window,
			wm_state_atom,
			0,
			2,
			False,
			AnyPropertyType,
			&actual_type,
			&actual_format,
			&nitems,
			&bytes_after,
			&prop
		);

		bool minimized = false;
		if (status == Success && prop != nullptr && nitems >= 2)
		{
			long state = ((long*)prop)[0];
			minimized = (state == IconicState); // 3 is IconicState
		}

		if (prop)
			XFree(prop);

		return (window_ptr->minimized = minimized);
#elif defined(ANDROID)
		return (window_ptr->minimized = false);
#endif
	}
	
	void window_set_focused(WINDOW* window_ptr) {
#if defined(_WIN32)
		if (!window_ptr || !window_ptr->hwnd)
			return;
		SetForegroundWindow(window_ptr->hwnd);
		SetFocus(window_ptr->hwnd);

#elif defined(__linux) && !defined(ANDROID)
		if (!window_ptr || !window_ptr->display || !window_ptr->window)
			return;

		XRaiseWindow(window_ptr->display, window_ptr->window);
		XSetInputFocus(window_ptr->display, window_ptr->window, RevertToParent, CurrentTime);
		XFlush(window_ptr->display);
#elif defined(ANDROID)
#endif
		*window_ptr->focused = true;
	}

	void window_set_size(WINDOW* window_ptr, float width, float height) {
		if (!window_ptr || *window_ptr->width == width && *window_ptr->height == height)
			return;
		*window_ptr->width = width;
		*window_ptr->height = height;
		defer_recreate_swap_chain(window_ptr->renderer);
		if (!window_ptr->headless) {
#if defined(_WIN32)
			if (!window_ptr->hwnd)
				return;
			RECT rect;
			GetWindowRect(window_ptr->hwnd, &rect);
			int x = rect.left;
			int y = rect.top;
			int w = static_cast<int>(width);
			int h = static_cast<int>(height);

			MoveWindow(window_ptr->hwnd, x, y, w, h, TRUE);
#elif defined(__linux) && !defined(ANDROID)
			if (!window_ptr->display || !window_ptr->window)
				return;

			XResizeWindow(window_ptr->display, window_ptr->window, static_cast<unsigned int>(width), static_cast<unsigned int>(height));
			XFlush(window_ptr->display);
#elif defined(ANDROID)
		// Android window sizing not controlled this way
#endif
		}
	}

	ImVec2 window_get_position(WINDOW* window_ptr) {
#if defined(_WIN32)
		if (!window_ptr || !window_ptr->hwnd)
			return ImVec2(window_ptr->x, window_ptr->y);

		RECT rect;
		GetWindowRect(window_ptr->hwnd, &rect);
		window_ptr->x = rect.left;
		window_ptr->y = rect.top;
		return ImVec2(window_ptr->x, window_ptr->y);

#elif defined(__linux) && !defined(ANDROID)
		if (!window_ptr || !window_ptr->display || !window_ptr->window)
			return ImVec2(window_ptr->x, window_ptr->y);

		Window root_return;
		int x = 0, y = 0;
		unsigned int width, height, border_width, depth;

		XGetGeometry(window_ptr->display, window_ptr->window, &root_return, &x, &y, &width, &height, &border_width, &depth);
		Window child;
		XTranslateCoordinates(window_ptr->display, window_ptr->window, root_return, 0, 0, &x, &y, &child);

		window_ptr->x = x;
		window_ptr->y = y;

		return ImVec2(window_ptr->x, window_ptr->y);

#elif defined(ANDROID)
		return ImVec2(window_ptr->x, window_ptr->y);
#endif
	}

	void window_set_position(WINDOW* window_ptr, float x, float y) {
#if defined(_WIN32)
		if (!window_ptr || !window_ptr->hwnd)
			return;

		RECT rect;
		GetWindowRect(window_ptr->hwnd, &rect);
		int w = rect.right - rect.left;
		int h = rect.bottom - rect.top;

		MoveWindow(window_ptr->hwnd, static_cast<int>(x), static_cast<int>(y), w, h, TRUE);

#elif defined(__linux) && !defined(ANDROID)
		if (!window_ptr || !window_ptr->display || !window_ptr->window)
			return;

		XMoveWindow(window_ptr->display, window_ptr->window, static_cast<int>(x), static_cast<int>(y));
		XFlush(window_ptr->display);

#elif defined(ANDROID)
#endif
		window_ptr->x = x;
		window_ptr->y = y;
	}


#endif

    void window_set_focused(WINDOW* window_ptr, bool focused) {
		*window_ptr->focused = focused;
		dr_ptr->imguiLayer.FocusWindow(window_ptr, focused);
	}
	
#if defined(_WIN32) || defined(__linux__)
    void window_request_drag(WINDOW* window_ptr) {
#if defined(_WIN32)
		ReleaseCapture();
		SendMessage(window_ptr->hwnd, WM_SYSCOMMAND, SC_MOVE | HTCAPTION, 0);
#elif defined(__linux__) && !defined(ANDROID)
		if (!window_ptr || !window_ptr->display || !window_ptr->window || !window_ptr->root)
			return;

		ImVec2 pos = ImGui::GetMousePos();

		Atom moveresize_atom = XInternAtom(window_ptr->display, "_NET_WM_MOVERESIZE", False);
		if (moveresize_atom == None)
			return;

		XClientMessageEvent ev{};
		ev.type = ClientMessage;
		ev.window = window_ptr->window;
		ev.message_type = moveresize_atom;
		ev.format = 32;
		ev.data.l[0] = static_cast<long>(pos.x);
		ev.data.l[1] = static_cast<long>(pos.y);
		ev.data.l[2] = 8;  // _NET_WM_MOVERESIZE_MOVE
		ev.data.l[3] = 1;  // left mouse button
		ev.data.l[4] = 0;

		XSendEvent(window_ptr->display,
				window_ptr->root,
				False,
				SubstructureNotifyMask | SubstructureRedirectMask,
				reinterpret_cast<XEvent*>(&ev));

		XFlush(window_ptr->display);

		window_ptr->drag_in_progress = true;
#elif defined(ANDROID)

#endif
	}
	void window_cancel_drag(WINDOW* window_ptr) {
		window_ptr->drag_in_progress = false;
		ImVec2 pos = ImGui::GetMousePos();
#if defined(_WIN32) || (defined(__linux__) && !defined(ANDROID))
		window_ptr->event_interface->cursor_press(0, false);
#endif
#if defined(__linux__) && !defined(ANDROID)
		if (!window_ptr || !window_ptr->display || !window_ptr->window || !window_ptr->root)
			return;

		Atom moveresize_atom = XInternAtom(window_ptr->display, "_NET_WM_MOVERESIZE", False);
		if (moveresize_atom == None)
			return;

		XClientMessageEvent ev{};
		ev.type = ClientMessage;
		ev.window = window_ptr->window;
		ev.message_type = moveresize_atom;
		ev.format = 32;
		ev.data.l[0] = static_cast<long>(pos.x);
		ev.data.l[1] = static_cast<long>(pos.y);
		ev.data.l[2] = 11;  // _NET_WM_MOVERESIZE_CANCEL
		ev.data.l[3] = 1;   // left mouse button
		ev.data.l[4] = 0;

		XSendEvent(window_ptr->display,
				window_ptr->root,
				False,
				SubstructureNotifyMask | SubstructureRedirectMask,
				reinterpret_cast<XEvent*>(&ev));

		XFlush(window_ptr->display);
#elif defined(ANDROID)
		window_ptr->event_interface->touch_event(
			AMOTION_EVENT_ACTION_POINTER_UP,
			0,
			0,
			pos.x, pos.y,
			1.0f,
			1.0f);
#endif
	}
#endif

#ifdef __ANDROID__
	void WINDOW::recreate_android(ANativeWindow* android_window, float width, float height) {
		this->android_window = android_window;
		*this->width = width;
		*this->height = height;
		create_surface(renderer);
		create_swap_chain(renderer);
		create_image_views(renderer);
		create_framebuffers(renderer);
	}
#endif

    void window_register_compute_dispatch(WINDOW* window_ptr, float priority, Shader* shader, const dz::function<int()>& dispatch_count_fn) {
		window_ptr->priority_shader_dispatches[priority][shader] = dispatch_count_fn;
	}

    void window_deregister_compute_dispatch(WINDOW* window_ptr, float priority, Shader* shader) {
		auto priority_it = window_ptr->priority_shader_dispatches.find(priority);
		if (priority_it == window_ptr->priority_shader_dispatches.end())
			return;
		auto& shader_dispatches = priority_it->second;
		auto it = shader_dispatches.find(shader);
		if (it == shader_dispatches.end())
			return;
		shader_dispatches.erase(it);
	}

	void window_set_clear_color(WINDOW* window_ptr, vec<float, 4> clearColor) {
		window_ptr->clearColor = clearColor;
	}

    VkDescriptorSet window_get_headless_ds(WINDOW* window_ptr) {
		return window_ptr->headless_ds;
	}
}
