cmake_minimum_required(VERSION 3.16)
project(chip8 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)

add_executable(chip8
    src/main.cpp
)

target_include_directories(chip8 PRIVATE
    ${SDL3_INCLUDE_DIRS}
)

target_compile_options(chip8 PRIVATE
    -Wall -Wextra -Wpedantic
    ${SDL3_CFLAGS_OTHER}
)

target_link_libraries(chip8 PRIVATE
    ${SDL3_LIBRARIES}
)
