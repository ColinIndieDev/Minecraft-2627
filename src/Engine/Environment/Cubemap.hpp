#pragma once 

#include <cstdint>
#include <string>

#include <Core/Camera3D.hpp>
#include <Core/Shader.hpp>

struct Cubemap {
    uint32_t vao = 0, vbo = 0, texture = 0;

    Cubemap() = default;
    Cubemap(std::string paths[6]);
    void Draw(glm::vec3 rotation, glm::vec4 color, Camera3D &camera, Shader &shader, const glm::mat4 &projection);
};
