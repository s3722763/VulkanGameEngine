add_library(imgui "imgui.cpp" "imgui_demo.cpp" "imgui_draw.cpp" "imgui_impl_sdl.cpp" "imgui_impl_vulkan.cpp" "imgui_widgets.cpp" "imgui_tables.cpp")

target_include_directories(imgui PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})