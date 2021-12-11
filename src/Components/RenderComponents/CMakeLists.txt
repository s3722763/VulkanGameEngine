add_library(RenderComponents "VulkanPipeline.cpp" "Material.cpp" "Camera.cpp")

target_include_directories(RenderComponents PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})