# CMakeList.txt : CMake project for GameEngine, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)
set(CMAKE_CXX_STANDARD 20)
set(_DEBUG 0)

set(VULKAN_SDK "C:/VulkanSDK/1.3.204.0")

project ("GameEngine")

include_directories(${VULKAN_SDK}/Include)
link_directories(${VULKAN_SDK}/Lib)

find_package(imgui CONFIG REQUIRED)
find_package(Bullet CONFIG REQUIRED)
find_package(assimp CONFIG REQUIRED)
find_package(unofficial-vulkan-memory-allocator CONFIG REQUIRED)
find_package(glm CONFIG REQUIRED)
find_package(SDL2 CONFIG REQUIRED)

# Enable hot loading on MSVC 2022+
if (MSVC AND WIN32 AND NOT MSVC_VERSION VERSION_LESS 142)

    # option 1: put the following command before the creation of the first
    #           compiled target you want to apply the option to;
    #           this applies to targets created in this dir and subdirectories
    add_link_options($<$<CONFIG:Debug>:/INCREMENTAL>)
    add_compile_options($<$<CONFIG:Debug>:/Zi>)
endif()

add_subdirectory("src")

# Add source to this project's executable.
add_executable (GameEngine "GameEngine.cpp")

target_link_libraries(GameEngine src)

add_custom_command(TARGET GameEngine POST_BUILD
				   COMMAND ${CMAKE_COMMAND} -E copy_directory
						   ${CMAKE_SOURCE_DIR}/resources/shaders
						   ${CMAKE_CURRENT_BINARY_DIR}/resources/shaders)

add_custom_command(TARGET GameEngine POST_BUILD
				   COMMAND ${CMAKE_COMMAND} -E copy_directory
						   ${CMAKE_SOURCE_DIR}/resources/models
						   ${CMAKE_CURRENT_BINARY_DIR}/resources/models)
