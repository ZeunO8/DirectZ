inline static constexpr uint8_t WINDOW_TYPE_WIN32 = 1;
inline static constexpr uint8_t WINDOW_TYPE_MACOS = 2;
inline static constexpr uint8_t WINDOW_TYPE_X11 = 4;
inline static constexpr uint8_t WINDOW_TYPE_XCB = 8;
inline static constexpr uint8_t WINDOW_TYPE_WAYLAND = 16;
inline static constexpr uint8_t WINDOW_TYPE_ANDROID = 32;
inline static constexpr uint8_t WINDOW_TYPE_IOS = 64;
struct WINDOW
{
    std::string title;
    float x;
    float y;
    float width;
    float height;
    bool borderless;
    bool vsync;
	std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> lastFrame;
	std::shared_ptr<float> float_frametime;
	std::shared_ptr<double> double_frametime;
	std::shared_ptr<int32_t> keys;
	std::shared_ptr<int32_t> buttons;
	std::shared_ptr<float> cursor;
	std::shared_ptr<int32_t> mod;
	std::shared_ptr<int32_t> focused;
	VkViewport viewport = {};
	VkRect2D scissor = {};
    Renderer* renderer = 0;
    std::shared_ptr<DirectRegistry> registry;
	std::map<IDrawListManager*, std::set<BufferGroup*>> draw_list_managers;
#ifdef _WIN32
    HINSTANCE hInstance;
    HWND hwnd;
    HDC hDeviceContext;
    HGLRC hRenderingContext;
    bool setDPIAware = false;
    float dpiScale;
#elif defined(__linux__)
	xcb_connection_t *connection = 0;
	const xcb_setup_t *setup = 0;
	xcb_screen_t *screen = 0;
	xcb_window_t window = 0;
	xcb_window_t root = 0;
	xcb_atom_t wm_protocols;
	xcb_atom_t wm_delete_window;
	xcb_atom_t atom_net_wm_state;
	xcb_atom_t atom_net_wm_state_hidden;
	xcb_atom_t atom_net_wm_state_maximized_horz;
	xcb_atom_t atom_net_wm_state_maximized_vert;
	xcb_key_symbols_t *keysyms;
	Display *display = 0;
	int32_t screenNumber = 0;
	uint32_t originalDelay = 0;
	uint32_t originalInterval = 0;
	void initAtoms();
#elif defined(MACOS)
	void *nsWindow = 0;
	void *nsView;
	void *nsImage = 0;
	void *nsImageView = 0;
#endif
	void create_platform();
	void post_init_platform();
	bool poll_events_platform();
};
static const uint32_t KEYCODES[] = {
	0,	27, 49, 50, 51, 52, 53, 54,					 55, 56, 57, 48, 45, 61, 8,	 9,	 81, 87, 69, 82, 84, 89, 85, 73,
	79, 80, 91, 93, 10, KEYCODE_CTRL,	65, 83,					 68, 70, 71, 72, 74, 75, 76, 59, 39, 96, KEYCODE_SHIFT,	 92, 90, 88, 67, 86,
	66, 78, 77, 44, 46, 47, 0,	0,					 0,	 32, 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 KEYCODE_HOME,
	0,	0,	0,	0,	0,	0,	0,	KEYCODE_END, 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	0,	0,	0,	0,	0,	0,	0,	0,					 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	0,	0,	0,	0,	0,	0,	0,	0,					 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	0,	0,	0,	0,	0,	0,	0,	0,					 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	0,	0,	0,	0,	0,	0,	0,	0,					 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	0,	0,	0,	0,	0,	0,	0,	0,					 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	0,	0,	0,	0,	0,	0,	0,	0,					 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	0,	0,	0,	0,	0,	0,	0,	0,					 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	0,	0,	0,	0,	0,	0,	0,	0,					 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	0,	0,	0,	0,	0,	0,	0,	0,					 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,	 0,
	0,	0,	0,	0,	0,	0,	0,	0,					 0,	 0,	 0,	 0,	 0,	 0,	 0,	 2,	 17, 3,	 0,	 20, 0,	 19, 0,	 5,
	18, 4,	26, 127};
WINDOW* window_create(const WindowCreateInfo& info)
{
    auto window = new WINDOW{info.title, info.x, info.y, info.width, info.height, info.borderless, info.vsync};

	window->float_frametime = std::shared_ptr<float>(new float(0), [](float* fp) { delete fp; });
	window->double_frametime = std::shared_ptr<double>(new double(0), [](double* dp) { delete dp; });
	window->keys = std::shared_ptr<int32_t>(new int32_t[256], [](int32_t* bp) { delete[] bp; });
	window->buttons = std::shared_ptr<int32_t>(new int32_t[8], [](int32_t* bp) { delete[] bp; });
	window->cursor = std::shared_ptr<float>(new float[2], [](float* fp) { delete[] fp; });
	window->mod = std::shared_ptr<int32_t>(new int32_t(0), [](int32_t* up) { delete up; });
	window->focused = std::shared_ptr<int32_t>(new int32_t(0), [](int32_t* ip) { delete ip; });

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

	window->registry = DZ_RGY;

    window->create_platform();

    window->renderer = renderer_init(window);

    window->post_init_platform();

    return window;
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
bool window_poll_events(WINDOW* window)
{
	auto poll_exit = window->poll_events_platform();
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
    return poll_exit;
}
void window_render(WINDOW* window)
{
    return renderer_render(window->renderer);
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
	window->float_frametime = std::shared_ptr<float>(pointer, [](auto p){});
}
void window_set_double_frametime_pointer(WINDOW* window, double* pointer)
{
	window->double_frametime = std::shared_ptr<double>(pointer, [](auto p){});
}
void window_set_keys_pointer(WINDOW* window, int32_t* pointer)
{
	window->keys = std::shared_ptr<int32_t>(pointer, [](auto p){});
}
void window_set_buttons_pointer(WINDOW* window, int32_t* pointer)
{
	window->buttons = std::shared_ptr<int32_t>(pointer, [](auto p){});
}
void window_set_cursor_pointer(WINDOW* window, float* pointer)
{
	window->cursor = std::shared_ptr<float>(pointer, [](auto p){});
}
void window_set_mod_pointer(WINDOW* window, int32_t* pointer)
{
	window->mod = std::shared_ptr<int32_t>(pointer, [](auto p){});
}
void window_set_focused_pointer(WINDOW* window, int32_t* pointer)
{
	window->focused = std::shared_ptr<int32_t>(pointer, [](auto p){});
}
void window_set_float_frametime_pointer(WINDOW* window, const std::shared_ptr<float>& pointer)
{
	window->float_frametime = pointer;
}
void window_set_double_frametime_pointer(WINDOW* window, const std::shared_ptr<double>& pointer)
{
	window->double_frametime = pointer;
}
void window_set_keys_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer)
{
	window->keys = pointer;
}
void window_set_buttons_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer)
{
	window->buttons = pointer;
}
void window_set_cursor_pointer(WINDOW* window, const std::shared_ptr<float>& pointer)
{
	window->cursor = pointer;
}
void window_set_mod_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer)
{
	window->mod = pointer;
}
void window_set_focused_pointer(WINDOW* window, const std::shared_ptr<int32_t>& pointer)
{
	window->focused = pointer;
}
int32_t& window_get_keypress_ref(WINDOW* window, uint8_t keycode)
{
	return window->keys.get()[keycode];
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
float& window_get_width_ref(WINDOW* window)
{
	return window->width;
}
float& window_get_height_ref(WINDOW* window)
{
	return window->height;
}
void window_free(WINDOW* window)
{
    renderer_free(window->renderer);
    delete window;
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
	int adjustedWidth = width, adjustedHeight = height;
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
            (y == -1 ? 0 : y), width,
            height,
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
#elif defined(__linux__)
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
										width, height, // Width, Height
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
#elif defined(__APPLE__)
uint8_t get_window_type_platform()
{
	return WINDOW_TYPE_MACOS;
}
void WINDOW::create_platform()
{
}
bool WINDOW::poll_events_platform()
{
	return true;
}
void WINDOW::post_init_platform()
{
}
#endif


#ifdef _WIN32
LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct WINDOW* window = (WINDOW*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_CREATE:
		{
			CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
			window = (WINDOW*)createStruct->lpCreateParams;
			SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)window);
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
			window->buttons.get()[button] = pressed;
			break;
		};
	// case WM_MOUSEWHEEL:
	// 	{
	// 		int zDelta = GET_WHEEL_DELTA_WPARAM(wParam); // This gives the scroll amount
	// 		auto wheelButton = zDelta > 0 ? 3 : 4; // Wheel scrolled up or down
	// 		glWindow->queueEvent(EVENT_MOUSE_PRESS, true, wheelButton);
	// 		break;
	// 	};
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		{
			WORD button = GET_XBUTTON_WPARAM(wParam);
			auto xButton = (button == XBUTTON2 ? 5 : (button == XBUTTON1 ? 6 : -1));
			if (xButton == -1)
				throw std::runtime_error("Invalid XButton");
			auto pressed = msg == WM_XBUTTONDOWN;
			window->buttons.get()[xButton] = pressed;
			break;
		};
	case WM_MOUSEMOVE:
		{
			POINT pt;
			GetCursorPos(&pt);
			ScreenToClient(hwnd, &pt);
			auto x = pt.x;
			auto y = pt.y;
			auto cursor = window->cursor.get();
			cursor[0] = x;
			cursor[1] = y;
			break;
		};
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
		if (wParam == VK_MENU ||
			wParam == VK_LMENU ||
			wParam == VK_RMENU)
		{
			window->keys.get()[KEYCODE_ALT] = (msg == WM_SYSKEYDOWN);
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
			if (keycodeHiword < 0 || keycodeHiword > sizeof(KEYCODES) / sizeof(KEYCODES[0]))
			{
				break;
			}
			auto keycode = KEYCODES[keycodeHiword];
			auto keypress = !((lParam >> 31) & 1);
			BYTE keyboardState[256];
			GetKeyboardState(keyboardState);
			wchar_t translatedChar[2] = {};
			int result = ToUnicode(keycode, keycodeHiword, keyboardState, translatedChar, 2, 0);
			if (result > 0)
			{
				keycode = translatedChar[0];
			}
			*window->mod = mod;
			window->keys.get()[keycode] = keypress;
		}
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	case WM_SIZE:
		{
			int32_t width = LOWORD(lParam), height = HIWORD(lParam);
			if (width != 0 && width != window->width && height != 0 && height != window->height)
			{
				window->width = width;
				window->height = height;
			}
			break;
		};
	// case WM_SETFOCUS:
	// 	{
	// 		glWindow->queueFocusEvent(true);
	// 		break;
	// 	};
	// case WM_KILLFOCUS:
	// 	{
	// 		glWindow->queueFocusEvent(false);
	// 		break;
	// 	};
	default:
	_default:
		return DefWindowProc(hwnd, msg, wParam, lParam);
	}
	return 0;
}
#elif defined(__linux__)
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
			uint32_t keycode = xkb_keysym_to_utf32(keysym);
			int32_t mod = 0;
			if (keycode == 0)
			{
				switch (keysym)
				{
				case XK_Up:
					keycode = KEYCODE_UP;
					break;
				case XK_Down:
					keycode = KEYCODE_DOWN;
					break;
				case XK_Left:
					keycode = KEYCODE_LEFT;
					break;
				case XK_Right:
					keycode = KEYCODE_RIGHT;
					break;
				case XK_Home:
					keycode = KEYCODE_HOME;
					break;
				case XK_End:
					keycode = KEYCODE_END;
					break;
				case XK_Page_Up:
					keycode = KEYCODE_PGUP;
					break;
				case XK_Page_Down:
					keycode = KEYCODE_PGDOWN;
					break;
				case XK_Insert:
					keycode = KEYCODE_INSERT;
					break;
				case XK_Num_Lock:
					keycode = KEYCODE_NUMLOCK;
					break;
				case XK_Caps_Lock:
					keycode = KEYCODE_CAPSLOCK;
					break;
				case XK_Pause:
					keycode = KEYCODE_PAUSE;
					break;
				case XK_Super_L:
					keycode = KEYCODE_SUPER;
					break;
				case XK_Super_R:
					keycode = KEYCODE_SUPER;
					break;
				default:
					keycode = 0;
					break;
				}
			}
			*window.mod = mod;
			window.keys.get()[keycode] = pressed;
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
			window.cursor.get()[0] = motion->event_x;
			window.cursor.get()[1] = motion->event_y;
			break;
		}
	case XCB_BUTTON_PRESS:
		{
			xcb_button_press_event_t* buttonPress = (xcb_button_press_event_t*)event;
			window.buttons.get()[buttonPress->detail - 1] = true;
			break;
		}
	case XCB_BUTTON_RELEASE:
		{
			xcb_button_release_event_t* buttonRelease = (xcb_button_release_event_t*)event;
			auto button = buttonRelease->detail - 1;
			if (button == 3 || button == 4)
				break;
			window.buttons.get()[button] = false;
			break;
		}
	// case XCB_FOCUS_IN:
	// 	window.queueFocusEvent(true);
	// 	break;

	// case XCB_FOCUS_OUT:
	// 	window.queueFocusEvent(false);
	// 	break;
	case XCB_DESTROY_NOTIFY:
		// free(event);
		return false;
	default: break;
	}
	return true;
}
#endif