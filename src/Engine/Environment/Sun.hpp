#pragma once

#include <glad/glad.h>

#include <GL/GLUtil.hpp>
#include <Core/Shader.hpp>
#include <Core/Camera3D.hpp>

namespace Sun {
    inline uint32_t _vao = 0, _vbo = 0;

    void Init();
    void Destroy();

    void Draw(const Shader &shader, Camera3D &camera, const glm::mat4 &projection, const glm::vec3 &lightDirection);
};
