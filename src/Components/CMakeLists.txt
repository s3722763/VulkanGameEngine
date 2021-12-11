add_subdirectory(RenderComponents)

add_library(Components "Components.cpp")

target_include_directories(Components PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(Components RenderComponents)