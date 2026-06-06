cmake_minimum_required(VERSION 3.22)

project(crushline_ui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(crushline
    src/main.cpp
    src/app/App.cpp
)

target_include_directories(crushline PRIVATE src)

if (MSVC)
    target_compile_options(crushline PRIVATE /W4)
else()
    target_compile_options(crushline PRIVATE -Wall -Wextra -Wpedantic)
endif()
