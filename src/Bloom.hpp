#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <glad/glad.h>

#include "GLUtil.hpp"
#include "Shader.hpp"

namespace Bloom {
    static inline uint32_t _pingPongFbo[2] = {0};
    static inline uint32_t _pingPongColorBuffers[2] = {0};
    static inline uint32_t _vao = 0, _vbo = 0;

    static inline void CreateFramebuffers(uint32_t screenWidth, uint32_t screenHeight) {
        glCreateFramebuffers(2, _pingPongFbo);
        glCreateTextures(GL_TEXTURE_2D, 2, _pingPongColorBuffers);
        for (int i = 0; i < 2; ++i) {
            glTextureStorage2D(_pingPongColorBuffers[i], 1, GL_RGBA16F, screenWidth, screenHeight);
            glTextureParameteri(_pingPongColorBuffers[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(_pingPongColorBuffers[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(_pingPongColorBuffers[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(_pingPongColorBuffers[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glNamedFramebufferTexture(_pingPongFbo[i], GL_COLOR_ATTACHMENT0, _pingPongColorBuffers[i], 0);

            if (glCheckNamedFramebufferStatus(_pingPongFbo[i], GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                fprintf(stderr, "Ping Pong Framebuffer incomplete\n");
                exit(-1);
            }
        }
    }

    static inline void Init(uint32_t screenWidth, uint32_t screenHeight) {
        CreateFramebuffers(screenWidth, screenHeight);

        float vertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };

        glCreateVertexArrays(1, &_vao);
        glCreateBuffers(1, &_vbo);
        glNamedBufferData(_vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexArrayVertexBuffer(_vao, 0, _vbo, 0, 5 * sizeof(float));
        glVertexArrayAttribFormat(_vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib(_vao, 0);
        glVertexArrayAttribFormat(_vao, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
        glEnableVertexArrayAttrib(_vao, 1);
        glVertexArrayAttribBinding(_vao, 0, 0);
        glVertexArrayAttribBinding(_vao, 1, 0);
    }

    static inline void Resize(uint32_t screenWidth, uint32_t screenHeight) {
        GLUtil::DestroyFramebuffer(_pingPongFbo[0]);
        GLUtil::DestroyFramebuffer(_pingPongFbo[1]);
        GLUtil::DestroyTexture(_pingPongColorBuffers[0]);
        GLUtil::DestroyTexture(_pingPongColorBuffers[1]);

        CreateFramebuffers(screenWidth, screenHeight);
    }

    static inline bool BlurColorBuffer(const Shader &shader, uint32_t colorBuffer) {
        bool horizontal = true;
        bool firstIteration = true;

        shader.Use();

        int totalBlurAmount = 10;
        for (int i = 0; i < totalBlurAmount; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, _pingPongFbo[horizontal]);
            shader.SetInt("horizontal", horizontal);
            glBindTextureUnit(0, firstIteration ? colorBuffer : _pingPongColorBuffers[!horizontal]);
            if (firstIteration) glBindVertexArray(_vao);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            horizontal = !horizontal;
            if (firstIteration) firstIteration = false;
        }

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return horizontal;
    }
}
