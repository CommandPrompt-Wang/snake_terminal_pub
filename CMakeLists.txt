cmake_minimum_required(VERSION 3.16)
project(snake_terminal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Default to Debug build if not specified
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

# Debug-specific flags
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
    add_definitions(-DDEBUG)
endif()

# raylib as a git submodule – built inline, no system install needed
add_subdirectory(third_party/raylib)

add_executable(snake
    src/app.cpp
    src/scene/scene_manager.cpp
    src/scene/game_scene.cpp
    src/scene/menu_scene.cpp
    src/scene/end_scene.cpp
    src/scene/config_scene.cpp
    src/scene/config_scene.h
    src/game/snake.cpp
    src/utility.h
    src/game/snake_anim.h
)

target_include_directories(snake PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(snake PRIVATE raylib)

# Symlink compile_commands.json to project root for clangd / VS Code IntelliSense
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(compile_commands_link ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
        COMMENT "Linking compile_commands.json for IDE"
    )
endif()
