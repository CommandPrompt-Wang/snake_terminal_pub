cmake_minimum_required(VERSION 3.16)
project(snake_terminal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0)
    add_definitions(-DDEBUG)
endif()

add_subdirectory(third_party/raylib)

set(SNAKE_SOURCES
    src/app.cpp
    src/scene/scene_manager.cpp
    src/scene/game_scene.cpp
    src/scene/menu_scene.cpp
    src/scene/end_scene.cpp
    src/scene/config_scene.cpp
    src/game/snake.cpp
    src/config/config_loader.cpp
    src/audio/backend/audioloader.cpp
    src/audio/audiomanager.cpp
    src/utils/log.cpp
)

add_executable(snake ${SNAKE_SOURCES})

target_include_directories(snake PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/dr_libs
    ${CMAKE_SOURCE_DIR}/third_party/miniaudio
)
target_link_libraries(snake PRIVATE raylib)

if(WIN32)
    target_link_libraries(snake PRIVATE winmm ole32 imm32)
endif()

if(MINGW)
    set_target_properties(snake PROPERTIES
        LINK_FLAGS "-static"
    )
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(compile_commands_link ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
        COMMENT "Linking compile_commands.json for IDE"
    )
endif()

enable_testing()

# Unit tests (no raylib)
add_executable(snake_tests
    tests/snake_collide_test.cpp
    src/game/snake.cpp
    src/utils/log.cpp
    src/audio/audiomanager.cpp
    src/audio/backend/audioloader.cpp
)
target_include_directories(snake_tests PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/third_party/dr_libs
    ${CMAKE_SOURCE_DIR}/third_party/miniaudio
)
if(WIN32)
    target_link_libraries(snake_tests PRIVATE winmm ole32)
endif()
if(MINGW)
    set_target_properties(snake_tests PROPERTIES
        LINK_FLAGS "-static"
    )
endif()
add_test(NAME snake_collide COMMAND snake_tests)
