add_library(RenderSystem "RenderSystem.cpp" "VulkanRenderer.cpp" "VkBootstrap.cpp" "../../Components/RenderComponents/VulkanPipeline.cpp" "VulkanUtility.cpp" "../../Managers/ModelManager.cpp" "VulkanTypes.cpp" "RenderLibraryImplementations.cpp" )

target_include_directories(RenderSystem PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})