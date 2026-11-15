#pragma once

#include <glad/glad.h>

#include "GLUtil.hpp"
#include "Shader.hpp"
#include "Camera3D.hpp"

struct Sun {
    uint32_t vao = 0, vbo = 0;

    Sun() {
        float vertices[] = {
            -0.5f, -0.5f,
             0.5f, -0.5f,
            -0.5f,  0.5f,
             0.5f,  0.5f 
        };
        glCreateVertexArrays(1, &vao);
        glCreateBuffers(1, &vbo);

        glNamedBufferData(vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);

        glVertexArrayVertexBuffer(vao, 0, vbo, 0, 2 * sizeof(float));

        glVertexArrayAttribFormat(vao, 0, 2, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(vao, 0);
        glVertexArrayAttribBinding(vao, 0, 0);
    }

    ~Sun() {
        GLUtil::DestroyVertexArray(vao); 
        GLUtil::DestroyVertexBuffer(vbo); 
    }

    void Draw(const Shader &shader, Camera3D &camera, const glm::mat4 &projection, const glm::vec3 &lightDirection) {
        glDepthFunc(GL_LEQUAL);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE);

        shader.Use();
        shader.SetVector3("cameraPos", camera.pos);
        shader.SetVector3("lightDirection", lightDirection);
        shader.SetMatrix4("view", camera.GetViewMatrix());
        shader.SetMatrix4("projection", projection);

        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glDepthMask(GL_TRUE);
        glDepthFunc(GL_LESS);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
};
