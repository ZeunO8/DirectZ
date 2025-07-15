#pragma once
#include <dz/GlobalUID.hpp>
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
	bool close_requested = false;
	VkViewport viewport = {};
	VkRect2D scissor = {};
    Renderer* renderer = 0;
	std::map<IDrawListManager*, std::set<BufferGroup*>> draw_list_managers;
	EventInterface* event_interface = 0;
	bool capture = false;
	bool drag_in_progress = false;
	ImGuiViewport* imguiViewport = 0;
#ifdef _WIN32
    HINSTANCE hInstance;
    HWND hwnd;
    // HDC hDeviceContext;
    HGLRC hRenderingContext;
    bool setDPIAware = false;
    float dpiScale;
#elif defined(__linux__) && !defined(__ANDROID__)
	Display* display = nullptr;
	Window window = 0;
	Window root = 0;
	Atom wm_protocols = 0;
	Atom wm_delete_window = 0;
	Atom atom_net_wm_state = 0;
	Atom atom_net_wm_state_hidden = 0;
	Atom atom_net_wm_state_maximized_horz = 0;
	Atom atom_net_wm_state_maximized_vert = 0;
	int screenNumber = 0;
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
	size_t id = GlobalUID::GetNew("Window");
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
static const KEYCODES WIN_MAP_KEYCODES[] = {
	KEYCODES::NUL,
	KEYCODES::ESCAPE,
	KEYCODES::_1, KEYCODES::_2, KEYCODES::_3,
	KEYCODES::_4, KEYCODES::_5, KEYCODES::_6,
	KEYCODES::_7, KEYCODES::_8, KEYCODES::_9,
	KEYCODES::_0,
	KEYCODES::MINUS, KEYCODES::EQUAL, KEYCODES::BACKSPACE,
	KEYCODES::TAB,
	KEYCODES::Q, KEYCODES::W, KEYCODES::E, KEYCODES::R, KEYCODES::T, KEYCODES::Y, KEYCODES::U, KEYCODES::I, KEYCODES::O, KEYCODES::P,
	KEYCODES::LEFTBRACKET, KEYCODES::RIGHTBRACKET, KEYCODES::ENTER, KEYCODES::CTRL,
	KEYCODES::A, KEYCODES::S, KEYCODES::D, KEYCODES::F, KEYCODES::G, KEYCODES::H, KEYCODES::J, KEYCODES::K, KEYCODES::L,
	KEYCODES::SEMICOLON, KEYCODES::SINGLEQUOTE, KEYCODES::GRAVEACCENT, KEYCODES::SHIFT,	KEYCODES::BACKSLASH,
	KEYCODES::Z, KEYCODES::X, KEYCODES::C, KEYCODES::V, KEYCODES::B, KEYCODES::N, KEYCODES::M, KEYCODES::COMMA, KEYCODES::PERIOD, KEYCODES::SLASH,
	KEYCODES::NUL,	KEYCODES::NUL,  KEYCODES::NUL,	KEYCODES::SPACE, KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::HOME,
	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::END, KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,
	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,					 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,
	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,					 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,
	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,					 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,
	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,					 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,
	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,					 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,
	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,					 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,
	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,					 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,
	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,					 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,
	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,					 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,
	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,	KEYCODES::NUL,					 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::UP, KEYCODES::NUL,	 KEYCODES::NUL,	 KEYCODES::LEFT, KEYCODES::NUL,	 KEYCODES::RIGHT, KEYCODES::NUL, KEYCODES::NUL,
	KEYCODES::DOWN, KEYCODES::NUL,	KEYCODES::CTRL, KEYCODES::Delete};

struct WindowMetaReflectable : Reflectable {

private:
	WINDOW* window_ptr;
	int uid;
	std::string name;
	inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
		{"title", {0, 0}}
	};
	inline static std::unordered_map<int, std::string> prop_index_names = {
		{0, "title"}
	};
	inline static std::vector<std::string> prop_names = {
		"title"
	};
	inline static const std::vector<const std::type_info*> typeinfos = {
		&typeid(std::string)
	};

public:
	WindowMetaReflectable(WINDOW* window_ptr);
	int GetID() override;
	std::string& GetName() override;
	DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
	DEF_GET_PROPERTY_NAMES(prop_names);
	void* GetVoidPropertyByIndex(int prop_index) override;
	DEF_GET_VOID_PROPERTY_BY_NAME;
	DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
	void NotifyChange(int prop_index) override;
};

struct WindowViewportReflectable : Reflectable {

private:
	WINDOW* window_ptr;
	int uid;
	std::string name;
	inline static std::unordered_map<std::string, std::pair<int, int>> prop_name_indexes = {
		{"x", {0, 0}},
		{"y", {1, 0}},
		{"width", {2, 0}},
		{"height", {3, 0}}
	};
	inline static std::unordered_map<int, std::string> prop_index_names = {
		{0, "x"},
		{1, "y"},
		{2, "width"},
		{3, "height"}
	};
	inline static std::vector<std::string> prop_names = {
		"x",
		"y",
		"width",
		"height"
	};
	inline static const std::vector<const std::type_info*> typeinfos = {
		&typeid(float),
		&typeid(float),
		&typeid(float),
		&typeid(float)
	};
	inline static const std::vector<bool>& disabled_properties = {
		true,
		true,
		true,
		true
	};

public:
	WindowViewportReflectable(WINDOW* window_ptr);
	int GetID() override;
	std::string& GetName() override;
	DEF_GET_PROPERTY_INDEX_BY_NAME(prop_name_indexes);
	DEF_GET_PROPERTY_NAMES(prop_names);
	void* GetVoidPropertyByIndex(int prop_index) override;
	DEF_GET_VOID_PROPERTY_BY_NAME;
	DEF_GET_PROPERTY_TYPEINFOS(typeinfos);
	DEF_GET_DISABLED_PROPERTIES(disabled_properties);
};