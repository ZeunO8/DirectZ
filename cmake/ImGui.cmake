FetchContent_Declare(imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG docking
  GIT_SHALLOW TRUE)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
  FetchContent_Populate(imgui)
endif()

add_library(imgui STATIC
  ${imgui_SOURCE_DIR}/imgui.cpp
  ${imgui_SOURCE_DIR}/misc/cpp/imgui_stdlib.cpp
  
  ${imgui_SOURCE_DIR}/imgui_demo.cpp
  ${imgui_SOURCE_DIR}/imgui_draw.cpp
  ${imgui_SOURCE_DIR}/backends/imgui_impl_vulkan.cpp
  ${imgui_SOURCE_DIR}/imgui_tables.cpp
  ${imgui_SOURCE_DIR}/imgui_widgets.cpp
)

target_include_directories(imgui PRIVATE
  ${imgui_SOURCE_DIR}
  ${imgui_SOURCE_DIR}/backends
	${Vulkan_INCLUDE_DIRS}
)

target_compile_features(imgui PRIVATE cxx_std_20)
set_target_properties(imgui PROPERTIES DEBUG_POSTFIX "d")

FetchContent_Declare(imguizmo
  GIT_REPOSITORY https://github.com/CedricGuillemet/ImGuizmo.git
  GIT_TAG master
  GIT_SHALLOW TRUE)
FetchContent_GetProperties(imguizmo)
if(NOT imguizmo_POPULATED)
  FetchContent_Populate(imguizmo)
endif()

add_library(imguizmo STATIC
  ${imguizmo_SOURCE_DIR}/GraphEditor.cpp
  ${imguizmo_SOURCE_DIR}/ImCurveEdit.cpp
  ${imguizmo_SOURCE_DIR}/ImGradient.cpp
  ${imguizmo_SOURCE_DIR}/ImGuizmo.cpp
  ${imguizmo_SOURCE_DIR}/ImSequencer.cpp)

target_include_directories(imguizmo PRIVATE
  ${imguizmo_SOURCE_DIR}
  ${imgui_SOURCE_DIR})

target_compile_features(imguizmo PRIVATE cxx_std_20)
set_target_properties(imguizmo PROPERTIES DEBUG_POSTFIX "d")