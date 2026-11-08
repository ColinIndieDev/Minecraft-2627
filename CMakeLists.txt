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

# GLAD
add_subdirectory(external/glad)

#### Linking ####

add_executable(${CMAKE_PROJECT_NAME}
    src/main.cpp

    src/Shader.cpp
    src/Cubemap.cpp
    src/Chunk.cpp
)

target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${stb_SOURCE_DIR})
target_include_directories(${CMAKE_PROJECT_NAME} PRIVATE ${fastnoiselite_SOURCE_DIR}/Cpp)

target_link_libraries(${CMAKE_PROJECT_NAME} PRIVATE
    glad
    glfw 
    OpenGL::GL
)
