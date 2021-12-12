add_subdirectory(Systems)
add_subdirectory(Managers)
add_subdirectory(Components)
add_subdirectory(Utility)

add_library(src "World.cpp" "Entities.cpp")

target_include_directories(src PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(src Systems Components Managers)