cmake_minimum_required(VERSION 3.22)

project(crushline_ui LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)
include(CTest)

set(SDL_SHARED OFF CACHE BOOL "" FORCE)
set(SDL_STATIC ON CACHE BOOL "" FORCE)
set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)

set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

FetchContent_Declare(
    SDL3
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.0
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

FetchContent_Declare(
    freetype
    GIT_REPOSITORY https://github.com/freetype/freetype.git
    GIT_TAG VER-2-13-3
    GIT_SHALLOW TRUE
)

FetchContent_MakeAvailable(SDL3 nlohmann_json freetype)

find_package(OpenGL REQUIRED)

add_executable(crushline
    src/main.cpp
    src/app/App.cpp
    src/app/DashboardLayout.cpp
    src/editor/GraphView.cpp
    src/graph/GraphDocument.cpp
    src/graph/GraphEvaluator.cpp
    src/graph/GraphSerializer.cpp
    src/graph/SampleGraph.cpp
    tests/GraphSerializerRoundTrip.cpp
    tests/GraphEvaluatorTest.cpp
    tests/GraphConnectionTest.cpp
    src/platform/Window.cpp
    src/platform/Time.cpp
    src/renderer/FontAtlas.cpp
    src/renderer/Renderer2D.cpp
    src/renderer/Texture.cpp
    src/ui/Theme.cpp
)

target_include_directories(crushline PRIVATE src)

target_link_libraries(crushline PRIVATE
    SDL3::SDL3
    OpenGL::GL
    nlohmann_json::nlohmann_json
    freetype
)

if (MSVC)
    target_compile_options(crushline PRIVATE /W4)
else()
    target_compile_options(crushline PRIVATE -Wall -Wextra -Wpedantic)
endif()

if (BUILD_TESTING)
    add_test(NAME crushline_smoke COMMAND crushline --smoke-test)
    add_test(NAME graph_serializer_roundtrip COMMAND crushline --serializer-roundtrip-test)
    add_test(NAME graph_evaluator COMMAND crushline --graph-evaluator-test)
    add_test(NAME graph_connection COMMAND crushline --graph-connection-test)
endif()
