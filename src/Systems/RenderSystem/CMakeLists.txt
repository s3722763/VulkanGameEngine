find_path(STB_INCLUDE_DIRS "stb_c_lexer.h")

find_package(assimp CONFIG REQUIRED)

add_library(RenderSystem "RenderSystem.cpp" "VulkanRenderer.cpp" "VkBootstrap.cpp" "../../Components/RenderComponents/VulkanPipeline.cpp" "VulkanUtility.cpp" "../../Managers/ModelManager.cpp" "VulkanTypes.cpp" "RenderLibraryImplementations.cpp"  "LightingSystem.hpp" "LightingSystem.cpp")

target_include_directories(RenderSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(RenderSystem PUBLIC ${STB_INCLUDE_DIRS})

target_link_directories(RenderSystem PUBLIC ${VULKAN_SDK}/Lib)

target_link_libraries(RenderSystem PUBLIC $<TARGET_NAME_IF_EXISTS:SDL2::SDL2main> $<IF:$<TARGET_EXISTS:SDL2::SDL2>,SDL2::SDL2,SDL2::SDL2-static> vulkan-1 unofficial::vulkan-memory-allocator::vulkan-memory-allocator assimp::assimp)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_link_libraries(RenderSystem PUBLIC shaderc_combinedd)
else()
	target_link_libraries(RenderSystem PUBLIC shaderc_combined)
endif()
