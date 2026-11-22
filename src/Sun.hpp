#pragma once

#include <glad/glad.h>

#include "GLUtil.hpp"
#include "Shader.hpp"
#include "Camera3D.hpp"

namespace Sun {
    static inline uint32_t _vao = 0, _vbo = 0;

    static inline void Init() {
        float vertices[] = {
            -0.5f, -0.5f,
             0.5f, -0.5f,
            -0.5f,  0.5f,
             0.5f,  0.5f 
        };
        glCreateVertexArrays(1, &_vao);
        glCreateBuffers(1, &_vbo);
        glNamedBufferData(_vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexArrayVertexBuffer(_vao, 0, _vbo, 0, 2 * sizeof(float));
        glVertexArrayAttribFormat(_vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(_vao, 0);
        glVertexArrayAttribBinding(_vao, 0, 0);
    }

    static inline void Destroy() {
        GLUtil::DestroyVertexArray(_vao); 
        GLUtil::DestroyVertexBuffer(_vbo); 
    }

    static inline void Draw(const Shader &shader, Camera3D &camera, const glm::mat4 &projection, const glm::vec3 &lightDirection) {
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        shader.Use();
        shader.SetVector3("cameraPos", camera.pos);
        shader.SetVector3("lightDirection", lightDirection);
        shader.SetMatrix4("view", camera.GetViewMatrix());
        shader.SetMatrix4("projection", projection);

        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
    }
};
