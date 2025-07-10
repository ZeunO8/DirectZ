namespace dz {
	#include "WindowImpl.hpp"
	WINDOW* window_create(const WindowCreateInfo& info)
	{
		auto window = new WINDOW{info.title, info.x, info.y,  info.borderless, info.vsync};

		window->registry = get_direct_registry();

		window->event_interface = new EventInterface(window);

	#ifdef __ANDROID__
		window->android_window = info.android_window;
		if (!window->registry->android_asset_manager)
			window->registry->android_asset_manager = info.android_asset_manager;
		if (!window->registry->android_config)
			AConfiguration_fromAssetManager(window->registry->android_config, info.android_asset_manager);
	#endif

		window->width = std::shared_ptr<float>(zmalloc<float>(1, info.width), [](float* fp) { zfree(fp, 1); });
		window->height = std::shared_ptr<float>(zmalloc<float>(1, info.height), [](float* fp) { zfree(fp, 1); });
		window->float_frametime = std::shared_ptr<float>(zmalloc<float>(1, 0), [](float* fp) { zfree(fp, 1); });
		window->double_frametime = std::shared_ptr<double>(zmalloc<double>(1, 0), [](double* dp) { zfree(dp, 1); });
		window->keys = std::shared_ptr<int32_t>(zmalloc<int32_t>(256, 0), [](int32_t* bp) { zfree(bp, 256); });
		window->buttons = std::shared_ptr<int32_t>(zmalloc<int32_t>(8, 0), [](int32_t* bp) { zfree(bp, 8); });
		window->cursor = std::shared_ptr<float>(zmalloc<float>(2, 0), [](float* fp) { zfree(fp, 2); });
		window->mod = std::shared_ptr<int32_t>(zmalloc<int32_t>(1, 0), [](int32_t* up) { zfree(up, 1); });
		window->focused = std::shared_ptr<int32_t>(zmalloc<int32_t>(1, 0), [](int32_t* ip) { zfree(ip, 1); });

		auto& viewport = window->viewport;
		viewport.x = 0.0f;
		viewport.y = 0.0f;
		viewport.width = info.width;
		viewport.height = info.height;
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		auto& scissor = window->scissor;
		scissor.offset = {0, 0};
		scissor.extent = {uint32_t(info.width), uint32_t(info.height)};

		window->registry->window_count++;

		window->create_platform();

		window->renderer = renderer_init(window);

		window->post_init_platform();

		return window;
	}
	
	ImGuiLayer& window_get_ImGuiLayer(WINDOW* window) {
		return window->imguiLayer;
	}

	void window_add_drawn_buffer_group(WINDOW* window, IDrawListManager* mgr, BufferGroup* buffer_group)
	{
		window->draw_list_managers[mgr].insert(buffer_group);
	}
	void window_remove_drawn_buffer_group(WINDOW* window, IDrawListManager* mgr, BufferGroup* buffer_group)
	{
		auto& vect = window->draw_list_managers[mgr];
		vect.erase(buffer_group);
		if (vect.empty())
			window->draw_list_managers.erase(mgr);
	}
	void window_free(WINDOW* window)
	{
		renderer_free(window->renderer);
		delete window;
	}
	bool window_poll_events(WINDOW* window)
	{
		auto poll_continue = window->poll_events_platform();
		auto now = std::chrono::time_point_cast<std::chrono::nanoseconds>(
			std::chrono::system_clock::now()
		);
		if (window->lastFrame.time_since_epoch().count() == 0)
			window->lastFrame = now;
		else
		{
			std::chrono::nanoseconds steady_duration_ns = now - window->lastFrame;
			std::chrono::duration<double> steady_duration_seconds = steady_duration_ns;
			*window->float_frametime = *window->double_frametime = steady_duration_seconds.count();
			window->lastFrame = now;
		}
		if (!poll_continue)
		{
			window->registry->window_count--;
			auto f_d_r = (window->registry->window_count == 0);
			window_free(window);
			if (f_d_r)
			{
				free_direct_registry();
			}
		}
		return poll_continue;
	}
	void window_render(WINDOW* window)
	{
		renderer_render(window->renderer);
	}
	float& window_get_float_frametime_ref(WINDOW* window)
	{
		return *window->float_frametime;
	}
	double& window_get_double_frametime_ref(WINDOW* window)
	{
		return *window->double_frametime;
	}
	void window_set_float_frametime_pointer(WINDOW* window, float* pointer)
	{
		window_set_float_frametime_pointer(window, std::shared_ptr<float>(pointer, [](auto p){}));
	}
	void window_set_double_frametime_pointer(WINDOW* window, double* pointer)
	{
		window_set_double_frametime_pointer(window, std::shared_ptr<double>(pointer, [](auto p){}));
	}
	void window_set_keys_pointer(WINDOW* window, int32_t* pointer)
	{
		window_set_keys_pointer(window, std::shared_ptr<int32_t>(pointer, [](auto p){}));
	}
	void window_set_buttons_pointer(WINDOW* window, int32_t* pointer)
	{
		window_set_buttons_pointer(window, std::shared_ptr<int32_t>(pointer, [](auto p){}));
	}
	void window_set_cursor_pointer(WINDOW* window, float* pointer)
	{
		window_set_cursor_pointer(window, std::shared_ptr<float>(pointer, [](auto p){}));
	}
	void window_set_mod_pointer(WINDOW* window, int32_t* pointer)
	{
		window_set_mod_pointer(window, std::shared_ptr<int32_t>(pointer, [](auto p){}));
	}
	void window_set_focused_pointer(WINDOW* window, int32_t* pointer)
	{
		window_set_focused_pointer(window, std::shared_ptr<int32_t>(pointer, [](auto p){}));
	}
	void window_set_width_pointer(WINDOW* window, float* pointer)
	{
		window_set_width_pointer(window, std::shared_ptr<float>(pointer, [](auto p){}));
	}
	void window_set_height_pointer(WINDOW* window, float* pointer)
	{
		window_set_height_pointer(window, std::shared_ptr<float>(pointer, [](auto p){}));
	}
	void window_set_float_frametime_pointer(WINDOW* window, const std::shared_ptr<float>& pointer)
	{
		auto old_float_frametime = *window->float_frametime;
		window->float_frametime = pointer;
		*window->float_frametime = old_float_frametime;
	}
	void window_set_double_frametime_pointer(WINDOW* window, const std::shared_ptr<double>& pointer)
	{
		auto old_double_frametime = *window->double_frametime;
		window->double_frametime = pointer;
		*window->double_frametime = old_double_frametime;
	}
	void window_set_keys_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer)
	{
		auto old_keys = window->keys;
		window->keys = pointer;
		memcpy(window->keys.get(), old_keys.get(), 256 * sizeof(int32_t));
	}
	void window_set_buttons_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer)
	{
		auto old_buttons = window->buttons;
		window->buttons = pointer;
		memcpy(window->buttons.get(), old_buttons.get(), 8 * sizeof(int32_t));
	}
	void window_set_cursor_pointer(WINDOW* window, const std::shared_ptr<float>& pointer)
	{
		auto old_cursor = window->cursor;
		window->cursor = pointer;
		memcpy(window->cursor.get(), old_cursor.get(), 2 * sizeof(float));
	}
	void window_set_mod_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer)
	{
		auto old_mod = *window->mod;
		window->mod = pointer;
		*window->mod = old_mod;
	}
	void window_set_focused_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer)
	{
		auto old_focused = *window->focused;
		window->focused = pointer;
		*window->focused = old_focused;
	}
	void window_set_width_pointer(WINDOW* window, const std::shared_ptr<float>& pointer)
	{
		auto old_width = *window->width;
		window->width = pointer;
		*window->width = old_width;
	}
	void window_set_height_pointer(WINDOW* window, const std::shared_ptr<float>& pointer)
	{
		auto old_height = *window->height;
		window->height = pointer;
		*window->height = old_height;
	}

	int32_t& window_get_keypress_ref(WINDOW* window, uint8_t keycode) {
		return window->keys.get()[keycode];
	}

    int32_t& window_get_keypress_ref(WINDOW* window, KEYCODES keycode) {
		return window_get_keypress_ref(window, (uint8_t)keycode);
	}

	std::shared_ptr<int32_t>& window_get_all_keypress_ref(WINDOW* window, uint8_t keycode)
	{
		return window->keys;
	}
	int32_t& window_get_buttonpress_ref(WINDOW* window, uint8_t button)
	{
		return window->buttons.get()[button];
	}
	std::shared_ptr<int32_t>& window_get_all_buttonpress_ref(WINDOW* window, uint8_t button)
	{
		return window->buttons;
	}
	std::shared_ptr<float>& window_get_width_ref(WINDOW* window)
	{
		return window->width;
	}
	std::shared_ptr<float>& window_get_height_ref(WINDOW* window)
	{
		return window->height;
	}

	#ifdef _WIN32
	LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
	void SetupPixelFormat(HDC hDeviceContext)
	{
		int pixelFormat;
		PIXELFORMATDESCRIPTOR pfd = {sizeof(PIXELFORMATDESCRIPTOR), 1};
		pfd.nVersion = 1;
		pfd.dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
		pfd.iPixelType = PFD_TYPE_RGBA;
		pfd.cColorBits = 32;
		pfd.cDepthBits = 32;
		pfd.iLayerType = PFD_MAIN_PLANE;
		pixelFormat = ChoosePixelFormat(hDeviceContext, &pfd);
		if (pixelFormat == 0)
		{
			throw std::runtime_error("pixelFormat is null!");
		}
		BOOL result = SetPixelFormat(hDeviceContext, pixelFormat, &pfd);
		if (!result)
		{
			throw std::runtime_error("failed to setPixelFormat!");
		}
	}
	uint8_t get_window_type_platform()
	{
		return WINDOW_TYPE_WIN32;
	}
	void WINDOW::create_platform()
	{
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
		WNDCLASSEX wc = {0};
		// wc.cbSize = sizeof(WNDCLASS);
		wc.cbSize = sizeof(WNDCLASSEX);
		wc.style = CS_VREDRAW | CS_HREDRAW;
		wc.lpfnWndProc = wndproc;
		wc.hInstance = hInstance;
		wc.lpszClassName = title.c_str();
		wc.hCursor = LoadCursor(NULL, IDC_ARROW);
		RegisterClassEx(&wc);
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
		hwnd = CreateWindowEx(
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
		hDeviceContext = GetDC(hwnd);
		SetupPixelFormat(hDeviceContext);
	}
	bool WINDOW::poll_events_platform()
	{
		MSG msg;
		while (PeekMessage(&msg, 0, 0, 0, PM_REMOVE))
		{
			if (msg.message == WM_QUIT)
				return false;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		return true;
	}
	void WINDOW::destroy_platform()
	{

	}
	void WINDOW::post_init_platform()
	{
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
	uint8_t get_window_type_platform()
	{
		return WINDOW_TYPE_XCB;
	}
	void WINDOW::create_platform()
	{
		connection = xcb_connect(nullptr, nullptr);
		if (xcb_connection_has_error(connection))
		{
			throw std::runtime_error("Failed to connect to X server!");
		}
		setup = xcb_get_setup(connection);
		xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
		screen = iter.data;
		root = screen->root;
		window = xcb_generate_id(connection);
		uint32_t value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
		uint32_t value_list[] = {screen->white_pixel,
														XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE |
															XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_BUTTON_PRESS |
															XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_FOCUS_CHANGE};
		xcb_create_window(connection,
											XCB_COPY_FROM_PARENT, // depth
											window,
											screen->root, // parent
											x, y, // X, Y position
											*width, *height, // Width, Height
											1, // Border width
											XCB_WINDOW_CLASS_INPUT_OUTPUT, screen->root_visual, value_mask, value_list);
		auto titleLength = title.size();
		xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, XCB_ATOM_WM_NAME, XCB_ATOM_STRING,
												8, // Format (8-bit string)
												titleLength, // Length of the title
												title.c_str());
		xcb_intern_atom_cookie_t wm_protocols_cookie = xcb_intern_atom(connection, 1, 12, "WM_PROTOCOLS");
		xcb_intern_atom_cookie_t wm_delete_cookie = xcb_intern_atom(connection, 0, 16, "WM_DELETE_WINDOW");
		xcb_intern_atom_reply_t* wm_protocols_reply = xcb_intern_atom_reply(connection, wm_protocols_cookie, nullptr);
		xcb_intern_atom_reply_t* wm_delete_reply = xcb_intern_atom_reply(connection, wm_delete_cookie, nullptr);
		if (wm_protocols_reply && wm_delete_reply)
		{
			wm_delete_window = wm_delete_reply->atom;
			xcb_change_property(connection, XCB_PROP_MODE_REPLACE, window, wm_protocols_reply->atom, XCB_ATOM_ATOM, 32, 1,
													&wm_delete_window);
		}
		free(wm_protocols_reply);
		free(wm_delete_reply);
		xcb_map_window(connection, window);
		xcb_flush(connection);
		keysyms = xcb_key_symbols_alloc(connection);
		if (!keysyms)
		{
			throw std::runtime_error("Failed to allocate key symbols!");
		}
		display = XOpenDisplay(0);
		if (!display)
		{
			throw std::runtime_error("Failed to openXlib display!");
		}
		XSync(display, False);
		screenNumber = DefaultScreen(display);
		initAtoms();
	}
	void WINDOW::initAtoms()
	{
		const char* atomNames[] =
		{
			"WM_PROTOCOLS",
			"WM_DELETE_WINDOW",
			"_NET_WM_STATE",
			"_NET_WM_STATE_HIDDEN",
			"_NET_WM_STATE_MAXIMIZED_HORZ",
			"_NET_WM_STATE_MAXIMIZED_VERT"
		};

		xcb_intern_atom_cookie_t cookies[6];
		xcb_intern_atom_reply_t* replies[6];

		for (int i = 0; i < 6; i++)
		{
			cookies[i] = xcb_intern_atom(connection, 0, strlen(atomNames[i]), atomNames[i]);
		}

		for (int i = 0; i < 6; i++)
		{
			replies[i] = xcb_intern_atom_reply(connection, cookies[i], nullptr);
		}

		wm_protocols = replies[0] ? replies[0]->atom : XCB_ATOM_NONE;
		wm_delete_window = replies[1] ? replies[1]->atom : XCB_ATOM_NONE;
		atom_net_wm_state = replies[2] ? replies[2]->atom : XCB_ATOM_NONE;
		atom_net_wm_state_hidden = replies[3] ? replies[3]->atom : XCB_ATOM_NONE;
		atom_net_wm_state_maximized_horz = replies[4] ? replies[4]->atom : XCB_ATOM_NONE;
		atom_net_wm_state_maximized_vert = replies[5] ? replies[5]->atom : XCB_ATOM_NONE;

		for (int i = 0; i < 6; i++)
		{
			free(replies[i]);
		}
	}
	bool handle_xcb_event(WINDOW& window, int eventType, xcb_generic_event_t* event);
	bool WINDOW::poll_events_platform()
	{
		bool return_ = true;
		xcb_generic_event_t* event;
		while ((event = xcb_poll_for_event(connection)))
		{
			auto eventType = event->response_type & ~0x80;
			if (!handle_xcb_event(*this, eventType, event))
			{
				return_ = false;
				goto _free;
			}
	_free:
			free(event);
			if (!return_)
				break;
		}
		return return_;
	}
	void WINDOW::post_init_platform()
	{
	}
	void WINDOW::destroy_platform()
	{
		XCloseDisplay(display);
		xcb_destroy_window(connection, window);
		xcb_disconnect(connection);
	}
	#elif defined(__ANDROID__)
	uint8_t get_window_type_platform()
	{
		return WINDOW_TYPE_ANDROID;
	}
	void WINDOW::create_platform()
	{
	}
	bool handle_android_event(WINDOW& window);
	bool WINDOW::poll_events_platform()
	{
		return true;
	}
	void WINDOW::post_init_platform()
	{
	}
	void WINDOW::destroy_platform()
	{
	}
	#endif


	#ifdef _WIN32
	KEYCODES GetCorrectedKeycode(KEYCODES keycode, bool shift);
	LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
	{
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

					if (isAutoRepeat)
					{
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
			PostQuitMessage(0);
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
				*window.focused = true;
				break;
			};
		case WM_KILLFOCUS:
			{
				*window.focused = false;
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
	bool handle_xcb_event(WINDOW& window, int eventType, xcb_generic_event_t* event)
	{
		switch (eventType)
		{
		case XCB_EXPOSE:
			break;
		case XCB_KEY_PRESS:
		case XCB_KEY_RELEASE:
			{
				auto pressed = eventType == XCB_KEY_PRESS;
				xcb_key_press_event_t* keyEvent = (xcb_key_press_event_t*)event;
				bool shiftPressed = keyEvent->state & (XCB_MOD_MASK_SHIFT);
				xcb_keysym_t keysym = xcb_key_symbols_get_keysym(window.keysyms, keyEvent->detail, shiftPressed ? 1 : 0);
				uint32_t keycode_utf = xkb_keysym_to_utf32(keysym);
				KEYCODES keycode = (KEYCODES)keycode_utf;
				int32_t mod = 0;
				if (keycode == KEYCODES::NUL)
				{
					switch (keysym)
					{
					case XK_Up:
						keycode = KEYCODES::UP;
						break;
					case XK_Down:
						keycode = KEYCODES::DOWN;
						break;
					case XK_Left:
						keycode = KEYCODES::LEFT;
						break;
					case XK_Right:
						keycode = KEYCODES::RIGHT;
						break;
					case XK_Home:
						keycode = KEYCODES::HOME;
						break;
					case XK_End:
						keycode = KEYCODES::END;
						break;
					case XK_Page_Up:
						keycode = KEYCODES::PGUP;
						break;
					case XK_Page_Down:
						keycode = KEYCODES::PGDOWN;
						break;
					case XK_Insert:
						keycode = KEYCODES::INSERT;
						break;
					case XK_Num_Lock:
						keycode = KEYCODES::NUMLOCK;
						break;
					case XK_Caps_Lock:
						keycode = KEYCODES::CAPSLOCK;
						break;
					case XK_Pause:
						keycode = KEYCODES::PAUSE;
						break;
					case XK_Super_L:
						keycode = KEYCODES::SUPER;
						break;
					case XK_Super_R:
						keycode = KEYCODES::SUPER;
						break;
					default:
						keycode = KEYCODES::NUL;
						break;
					}
				}
				*window.mod = mod;
				window.event_interface->key_press(keycode, pressed);
				break;
			}
		case XCB_CLIENT_MESSAGE:
			{
				xcb_client_message_event_t* cm = (xcb_client_message_event_t*)event;
				if (cm->data.data32[0] == window.wm_delete_window)
				{
					return false;
				}
				break;
			}
		case XCB_MOTION_NOTIFY:
			{
				xcb_motion_notify_event_t* motion = (xcb_motion_notify_event_t*)event;
				window.event_interface->cursor_move(motion->event_x, motion->event_y);
				break;
			}
		case XCB_BUTTON_PRESS:
			{
				xcb_button_press_event_t* buttonPress = (xcb_button_press_event_t*)event;
				auto button = buttonPress->detail - 1;
				window.event_interface->cursor_press(button, true);
				break;
			}
		case XCB_BUTTON_RELEASE:
			{
				xcb_button_release_event_t* buttonRelease = (xcb_button_release_event_t*)event;
				auto button = buttonRelease->detail - 1;
				if (button == 3 || button == 4)
					break;
				window.event_interface->cursor_press(button, false);
				break;
			}
		case XCB_FOCUS_IN:
			*window.focused = true;
			break;

		case XCB_FOCUS_OUT:
			*window.focused = false;
			break;
		case XCB_DESTROY_NOTIFY:
			// free(event);
			return false;
		default: break;
		}
		return true;
	}
	#endif

	EventInterface* window_get_event_interface(WINDOW* window)
	{
		return window->event_interface;
	}

	#ifdef __ANDROID__
	void WINDOW::recreate_android(ANativeWindow* android_window, float width, float height)
	{
		this->android_window = android_window;
		*this->width = width;
		*this->height = height;
		create_surface(renderer);
		create_swap_chain(renderer);
		create_image_views(renderer);
		create_framebuffers(renderer);
	}
	#endif
}