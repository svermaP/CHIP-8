cmake_minimum_required(VERSION 3.16)
project(my_project LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL3 REQUIRED sdl3)

add_executable(my_project
    src/main.cpp
)

target_include_directories(my_project PRIVATE
    ${SDL3_INCLUDE_DIRS}
)

target_compile_options(my_project PRIVATE
    -Wall -Wextra -Wpedantic
    ${SDL3_CFLAGS_OTHER}
)

target_link_libraries(my_project PRIVATE
    ${SDL3_LIBRARIES}
)
