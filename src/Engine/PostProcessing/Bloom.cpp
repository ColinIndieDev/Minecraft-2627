#include "Bloom.hpp"

#include <GL/GLUtil.hpp>

namespace Bloom {
    void CreateFramebuffers(uint32_t screenWidth, uint32_t screenHeight) {
        glCreateFramebuffers(2, PingPongFbo);
        glCreateTextures(GL_TEXTURE_2D, 2, PingPongColorBuffers);
        for (int i = 0; i < 2; ++i) {
            glTextureStorage2D (PingPongColorBuffers[i], 1, GL_RGBA16F, screenWidth, screenHeight);
            glTextureParameteri(PingPongColorBuffers[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(PingPongColorBuffers[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTextureParameteri(PingPongColorBuffers[i], GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTextureParameteri(PingPongColorBuffers[i], GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glNamedFramebufferTexture(PingPongFbo[i], GL_COLOR_ATTACHMENT0, PingPongColorBuffers[i], 0);

            if (glCheckNamedFramebufferStatus(PingPongFbo[i], GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
                fprintf(stderr, "Ping Pong Framebuffer incomplete\n");
                exit(-1);
            }
        }
    }

    void Init(uint32_t screenWidth, uint32_t screenHeight) {
        CreateFramebuffers(screenWidth, screenHeight);

        float vertices[] = {
            -1.0f,  1.0f, 0.0f, 0.0f, 1.0f,
            -1.0f, -1.0f, 0.0f, 0.0f, 0.0f,
             1.0f,  1.0f, 0.0f, 1.0f, 1.0f,
             1.0f, -1.0f, 0.0f, 1.0f, 0.0f,
        };

        glCreateVertexArrays      (1, &Vao);
        glCreateBuffers           (1, &Vbo);

        glNamedBufferData         (Vbo, sizeof(vertices), vertices, GL_STATIC_DRAW);
        glVertexArrayVertexBuffer (Vao, 0, Vbo, 0, 5 * sizeof(float));

        glVertexArrayAttribFormat (Vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glEnableVertexArrayAttrib (Vao, 0);
        glVertexArrayAttribFormat (Vao, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
        glEnableVertexArrayAttrib (Vao, 1);
        glVertexArrayAttribBinding(Vao, 0, 0);
        glVertexArrayAttribBinding(Vao, 1, 0);
    }

    void Resize(uint32_t screenWidth, uint32_t screenHeight) {
        GLUtil::DestroyFramebuffer(PingPongFbo[0]);
        GLUtil::DestroyFramebuffer(PingPongFbo[1]);
        GLUtil::DestroyTexture    (PingPongColorBuffers[0]);
        GLUtil::DestroyTexture    (PingPongColorBuffers[1]);

        CreateFramebuffers(screenWidth, screenHeight);
    }

    void Destroy() {
        GLUtil::DestroyFramebuffer(PingPongFbo[0]);
        GLUtil::DestroyFramebuffer(PingPongFbo[1]);
        GLUtil::DestroyTexture    (PingPongColorBuffers[0]);
        GLUtil::DestroyTexture    (PingPongColorBuffers[1]);

        GLUtil::DestroyVertexArray (Vao);
        GLUtil::DestroyVertexBuffer(Vbo);
    }

    bool BlurColorBuffer(const Shader &shader, uint32_t colorBuffer, int totalBlurAmount) {
        bool horizontal = true;
        bool firstIteration = true;

        shader.Use();
        for (int i = 0; i < totalBlurAmount; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, PingPongFbo[horizontal]);

            shader.SetInt("horizontal", horizontal);

            glBindTextureUnit(0, firstIteration ? colorBuffer : PingPongColorBuffers[!horizontal]);
            if (firstIteration) {
                glBindVertexArray(Vao);
            }
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            horizontal = !horizontal;
            if (firstIteration) {
                firstIteration = false;
            }
        }

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return horizontal;
    }
}
