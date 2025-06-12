inline static constexpr uint8_t WINDOW_TYPE_WIN32 = 1;
inline static constexpr uint8_t WINDOW_TYPE_MACOS = 2;
inline static constexpr uint8_t WINDOW_TYPE_X11 = 4;
inline static constexpr uint8_t WINDOW_TYPE_XCB = 8;
inline static constexpr uint8_t WINDOW_TYPE_WAYLAND = 16;
inline static constexpr uint8_t WINDOW_TYPE_ANDROID = 32;
inline static constexpr uint8_t WINDOW_TYPE_IOS = 64;
struct Window
{
    std::string title;
    float x;
    float y;
    float width;
    float height;
    bool borderless;
    bool vsync;
	std::chrono::system_clock::time_point lastFrame;
	std::shared_ptr<float> frametime;
	std::shared_ptr<int32_t> keys;
	std::shared_ptr<int32_t> buttons;
	std::shared_ptr<float> cursor;
	uint8_t mod = 0;
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
#elif defined(__APPLE__)
#endif
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
void window_create_platform(Window* window);
bool window_poll_events_platform(Window* window);
void window_post_init_platform(Window* window);
Window* window_create(const WindowCreateInfo& info)
{
    auto window = new Window{info.title, info.x, info.y, info.width, info.height, info.borderless, info.vsync};

	window->frametime = std::shared_ptr<float>(new float(0), [](float* fp) { delete fp; });
	window->keys = std::shared_ptr<int32_t>(new int32_t[256], [](int32_t* bp) { delete[] bp; });
	window->buttons = std::shared_ptr<int32_t>(new int32_t[8], [](int32_t* bp) { delete[] bp; });
	window->cursor = std::shared_ptr<float>(new float[2], [](float* fp) { delete[] fp; });

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

    window_create_platform(window);

    window->renderer = renderer_init(window);

    window_post_init_platform(window);

    return window;
}
void window_add_drawn_buffer_group(Window* window, IDrawListManager* mgr, BufferGroup* buffer_group)
{
	window->draw_list_managers[mgr].insert(buffer_group);
}
void window_remove_drawn_buffer_group(Window* window, IDrawListManager* mgr, BufferGroup* buffer_group)
{
	auto& vect = window->draw_list_managers[mgr];
	vect.erase(buffer_group);
	if (vect.empty())
		window->draw_list_managers.erase(mgr);
}
bool window_poll_events(Window* window)
{
	auto now = std::chrono::system_clock::now();
	if (window->lastFrame.time_since_epoch().count() == 0)
		window->lastFrame = now;
	else
	{
		auto diff = (now - window->lastFrame);
		auto count = (diff.count() / 1'000'000'0.0f);
		window->lastFrame = now;
		*window->frametime = count;
	}
    return window_poll_events_platform(window);
}
void window_render(Window* window)
{
    return renderer_render(window->renderer);
}
float& window_get_frametime_ref(Window* window)
{
	return *window->frametime;
}
void window_set_frametime_pointer(Window* window, float* pointer)
{
	window->frametime = std::shared_ptr<float>(pointer, [](auto p){});
}
void window_set_keys_pointer(Window* window, int32_t* pointer)
{
	window->keys = std::shared_ptr<int32_t>(pointer, [](auto p){});
}
void window_set_buttons_pointer(Window* window, int32_t* pointer)
{
	window->buttons = std::shared_ptr<int32_t>(pointer, [](auto p){});
}
void window_set_cursor_pointer(Window* window, float* pointer)
{
	window->cursor = std::shared_ptr<float>(pointer, [](auto p){});
}
void window_set_frametime_pointer(Window* window, const std::shared_ptr<float>& pointer)
{
	window->frametime = pointer;
}
void window_set_keys_pointer(Window* window, const std::shared_ptr<int32_t>& pointer)
{
	window->keys = pointer;
}
void window_set_buttons_pointer(Window* window, const std::shared_ptr<int32_t>& pointer)
{
	window->buttons = pointer;
}
void window_set_cursor_pointer(Window* window, const std::shared_ptr<float>& pointer)
{
	window->cursor = pointer;
}
int32_t& window_get_keypress_ref(Window* window, uint8_t keycode)
{
	return window->keys.get()[keycode];
}
std::shared_ptr<int32_t>& window_get_all_keypress_ref(Window* window, uint8_t keycode)
{
	return window->keys;
}
int32_t& window_get_buttonpress_ref(Window* window, uint8_t button)
{
	return window->buttons.get()[button];
}
std::shared_ptr<int32_t>& window_get_all_buttonpress_ref(Window* window, uint8_t button)
{
	return window->buttons;
}
float& window_get_width_ref(Window* window)
{
	return window->width;
}
float& window_get_height_ref(Window* window)
{
	return window->height;
}
void window_free(Window* window)
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
void window_create_platform(Window* window)
{
	if (!window->setDPIAware)
	{
		HRESULT hr = SetProcessDPIAware();
		if (FAILED(hr))
		{
			throw std::runtime_error("SetProcessDpiAwareness failed");
		}
		window->setDPIAware = true;
	}
	window->hInstance = GetModuleHandle(NULL);
	WNDCLASSEX wc = {0};
	// wc.cbSize = sizeof(WNDCLASS);
	wc.cbSize = sizeof(WNDCLASSEX);
	wc.style = CS_VREDRAW | CS_HREDRAW;
	wc.lpfnWndProc = wndproc;
	wc.hInstance = window->hInstance;
	wc.lpszClassName = window->title.c_str();
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	RegisterClassEx(&wc);
	window->dpiScale = 1.0f;
	HDC screen = GetDC(NULL);
	int32_t dpi = GetDeviceCaps(screen, LOGPIXELSX);
	ReleaseDC(NULL, screen);
	window->dpiScale = dpi / 96.0f;
	int adjustedWidth = window->width, adjustedHeight = window->height;
	auto wsStyle = WS_OVERLAPPEDWINDOW;
	RECT desiredRect = {0, 0, adjustedWidth, adjustedHeight};
	auto exStyle = WS_EX_APPWINDOW;
	AdjustWindowRectEx(&desiredRect, wsStyle, FALSE, exStyle);
	adjustedWidth = desiredRect.right - desiredRect.left;
	adjustedHeight = desiredRect.bottom - desiredRect.top;
	window->hwnd = CreateWindowEx(
        exStyle,
        window->title.c_str(),
        window->title.c_str(),
        wsStyle,
        window->x == -1 ? CW_USEDEFAULT : window->x,
        window->y == -1 ? CW_USEDEFAULT : window->y,
        adjustedWidth,
        adjustedHeight,
        0,
        NULL,
        window->hInstance,
        window
    );

	if (window->hwnd == NULL)
		throw std::runtime_error("Failed to create window");
	SetWindowLongPtr(window->hwnd, GWLP_USERDATA, (LONG_PTR)window);
	if (window->borderless)
	{
		SetWindowLong(
            window->hwnd,
            GWL_STYLE,
            (GetWindowLong(window->hwnd, GWL_STYLE) & ~WS_CAPTION & ~WS_THICKFRAME & ~WS_SYSMENU) | WS_MINIMIZEBOX |
            WS_MAXIMIZEBOX
        );
		SetWindowLong(window->hwnd, GWL_EXSTYLE, GetWindowLong(window->hwnd, GWL_EXSTYLE) & ~WS_EX_STATICEDGE);
		UINT flags = SWP_NOZORDER | SWP_FRAMECHANGED | SWP_SHOWWINDOW;
		if (window->x == -1 && window->y == -1)
			flags |= SWP_NOMOVE;
		SetWindowPos(
            window->hwnd,
            HWND_TOPMOST,
            (window->x == -1 ? 0 : window->x), // Use explicit or default X position
            (window->y == -1 ? 0 : window->y), window->width,
            window->height,
            flags
        );
	}
	window->hDeviceContext = GetDC(window->hwnd);
	SetupPixelFormat(window->hDeviceContext);
}
bool window_poll_events_platform(Window* window)
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
void window_post_init_platform(Window* window)
{
	ShowWindow(window->hwnd, SW_NORMAL);
	UpdateWindow(window->hwnd);
	RECT rect;
	if (GetWindowRect(window->hwnd, &rect))
	{
		window->x = rect.left;
		window->y = rect.top;
	}
}
#elif defined(__linux__)
uint8_t get_window_type_platform()
{
	return WINDOW_TYPE_XCB;
}
void window_create_platform(Window* window)
{
}
bool window_poll_events_platform(Window* window)
{
	return true;
}
void window_post_init_platform(Window* window)
{
}
#elif defined(__APPLE__)
uint8_t get_window_type_platform()
{
	return WINDOW_TYPE_MACOS;
}
void window_create_platform(Window* window)
{
}
bool window_poll_events_platform(Window* window)
{
	return true;
}
void window_post_init_platform(Window* window)
{
}
#endif


#ifdef _WIN32
LRESULT CALLBACK wndproc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	struct Window* window = (Window*)GetWindowLongPtr(hwnd, GWLP_USERDATA);
	switch (msg)
	{
	case WM_CREATE:
		{
			CREATESTRUCT* createStruct = (CREATESTRUCT*)lParam;
			window = (Window*)createStruct->lpCreateParams;
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
			window->mod = mod;
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
#endif