cmake_minimum_required(VERSION 3.31)
project(Minecraft)

# C++ 17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

#### Debug ####

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0 -fno-omit-frame-pointer)
    add_compile_options(
        -fsanitize=address,undefined
        -fno-omit-frame-pointer
        -g
        -O0
    )
    add_link_options(
        -fsanitize=address,undefined
    )
endif()

#### Dev ####

if (CMAKE_BUILD_TYPE STREQUAL "Dev")
    add_compile_options(-g -O2 -DNDEBUG)
endif()

#### Release ####

if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_compile_options(-g -O3 -DNDEBUG)
endif()

include(FetchContent)

# GLFW & OpenGL
find_package(glfw3 3.3 REQUIRED)
find_package(OpenGL REQUIRED)

# GLM
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG master
)
FetchContent_MakeAvailable(glm)

# STB
FetchContent_Declare(
        stb
        GIT_REPOSITORY https://github.com/nothings/stb.git
        GIT_TAG master
)
FetchContent_MakeAvailable(stb)

# FastNoiseLite
FetchContent_Declare(
        fastnoiselite
        GIT_REPOSITORY https://github.com/Auburn/FastNoiseLite.git
        GIT_TAG master
)
FetchContent_MakeAvailable(fastnoiselite)

# ImGUI
FetchContent_Declare(
    imgui
    GIT_REPOSITORY https://github.com/ocornut/imgui.git
    GIT_TAG v1.91.8
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(imgui)

# Assimp
FetchContent_Declare(
    assimp
    GIT_REPOSITORY https://github.com/assimp/assimp.git
    GIT_TAG master
)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)

# Disable Minizip
set(ASSIMP_BUILD_MINIZIP OFF CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(assimp)

# GLAD
add_subdirectory(external/glad)

#### Linking ####

add_executable(${CMAKE_PROJECT_NAME}
    src/main.cpp

    src/Engine/Core/Camera3D.cpp
    src/Engine/Core/Shader.cpp
    src/Engine/Core/Window.cpp
    src/Engine/Environment/Cubemap.cpp
    src/Engine/Environment/Sun.cpp
    src/Engine/GL/GLUtil.cpp
    src/Engine/Lighting/Shadow.cpp
    src/Engine/Objects/Model.cpp
    src/Engine/Performance/Frustum.cpp
    src/Engine/Performance/HeapTracker.cpp
    src/Engine/Performance/StackTracker.cpp
    src/Engine/PostProcessing/Bloom.cpp
    src/Engine/PostProcessing/Godrays.cpp
    src/Engine/PostProcessing/HDR.cpp

    src/Chunk.cpp
    src/Minecraft.cpp
    src/Config.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/Engine)

add_library(imgui STATIC
    ${imgui_SOURCE_DIR}/imgui.cpp
    ${imgui_SOURCE_DIR}/imgui_draw.cpp
    ${imgui_SOURCE_DIR}/imgui_tables.cpp
    ${imgui_SOURCE_DIR}/imgui_widgets.cpp
    ${imgui_SOURCE_DIR}/imgui_demo.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
    ${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${imgui_SOURCE_DIR}
    ${imgui_SOURCE_DIR}/backends
)

target_link_libraries(imgui PUBLIC glfw OpenGL::GL)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${stb_SOURCE_DIR})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${fastnoiselite_SOURCE_DIR}/Cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    glad
    glfw 
    OpenGL::GL
    imgui
    assimp
)
