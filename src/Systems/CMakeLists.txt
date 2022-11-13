add_subdirectory(RenderSystem)

add_library(Systems "Systems.cpp")

target_include_directories(Systems PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Systems RenderSystem)