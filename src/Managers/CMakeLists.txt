add_library(Managers "ResourceManager.cpp" "VulkanResourceManager.hpp" "VulkanResourceManager.cpp"  )

target_include_directories(Managers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Managers unofficial::vulkan-memory-allocator::vulkan-memory-allocator)