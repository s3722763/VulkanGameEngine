add_library(Managers "ResourceManager.cpp" "VulkanResourceManager.hpp" "VulkanResourceManager.cpp"  )

target_include_directories(Managers PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})