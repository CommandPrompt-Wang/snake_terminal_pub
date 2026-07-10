cmake_minimum_required(VERSION 3.16)
project(snake_terminal LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Homebrew raylib on Apple Silicon / Intel Mac
list(APPEND CMAKE_PREFIX_PATH
    "/opt/homebrew"
    "/usr/local"
)

find_package(raylib 6.0 REQUIRED)

add_executable(snake
    src/main.cpp
)

target_include_directories(snake PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(snake PRIVATE raylib)

if(APPLE)
    target_link_libraries(snake PRIVATE
        "-framework IOKit"
        "-framework Cocoa"
        "-framework OpenGL"
    )
endif()

# Symlink compile_commands.json to project root for clangd / VS Code IntelliSense
if(CMAKE_EXPORT_COMPILE_COMMANDS)
    add_custom_target(compile_commands_link ALL
        COMMAND ${CMAKE_COMMAND} -E create_symlink
            ${CMAKE_BINARY_DIR}/compile_commands.json
            ${CMAKE_SOURCE_DIR}/compile_commands.json
        COMMENT "Linking compile_commands.json for IDE"
    )
endif()
