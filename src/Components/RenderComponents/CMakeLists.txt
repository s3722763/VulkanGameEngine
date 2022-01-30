add_library(RenderComponents "VulkanPipeline.cpp" "Material.cpp" "Camera.cpp" "LightComponent.hpp" "Framebuffer.hpp")

target_include_directories(RenderComponents PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})