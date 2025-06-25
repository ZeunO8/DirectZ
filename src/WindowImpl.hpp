
struct WINDOW
{
    std::string title;
    float x;
    float y;
    bool borderless;
    bool vsync;
    std::shared_ptr<float> width;
    std::shared_ptr<float> height;
	std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds> lastFrame;
	std::shared_ptr<float> float_frametime;
	std::shared_ptr<double> double_frametime;
	std::shared_ptr<int32_t> keys;
	std::shared_ptr<int32_t> buttons;
	std::shared_ptr<float> cursor;
	std::shared_ptr<int32_t> mod;
	std::shared_ptr<int32_t> focused;
	bool closed = false;
	VkViewport viewport = {};
	VkRect2D scissor = {};
    Renderer* renderer = 0;
    DirectRegistry* registry;
	std::map<IDrawListManager*, std::set<BufferGroup*>> draw_list_managers;
	EventInterface* event_interface = 0;
#ifdef _WIN32
    HINSTANCE hInstance;
    HWND hwnd;
    HDC hDeviceContext;
    HGLRC hRenderingContext;
    bool setDPIAware = false;
    float dpiScale;
#elif defined(__linux__) && !defined(__ANDROID__)
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
#elif defined(__ANDROID__)
	ANativeWindow* android_window = 0;
#elif defined(MACOS)
	void *nsWindow = 0;
	void *nsView;
	void *nsImage = 0;
	void *nsImageView = 0;
	void *metalView = 0;
#endif
	void create_platform();
	void post_init_platform();
	bool poll_events_platform();
	void destroy_platform();
#ifdef __ANDROID__
	void recreate_android(ANativeWindow* android_window, float width, float height);
#endif
};
inline static constexpr uint8_t WINDOW_TYPE_WIN32 = 1;
inline static constexpr uint8_t WINDOW_TYPE_MACOS = 2;
inline static constexpr uint8_t WINDOW_TYPE_X11 = 4;
inline static constexpr uint8_t WINDOW_TYPE_XCB = 8;
inline static constexpr uint8_t WINDOW_TYPE_WAYLAND = 16;
inline static constexpr uint8_t WINDOW_TYPE_ANDROID = 32;
inline static constexpr uint8_t WINDOW_TYPE_IOS = 64;
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