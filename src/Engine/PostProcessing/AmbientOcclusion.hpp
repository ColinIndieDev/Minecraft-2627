#pragma once

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>

#include <glad/glad.h>
#include <glm/glm.hpp>

#include <GL/GLUtil.hpp>
#include <Core/Shader.hpp>

namespace AO {
    static inline uint32_t _gBuffer = 0;
    static inline uint32_t _gPosition = 0, _gNormal = 0, _gAlbedo = 0;
    static inline uint32_t _rboDepth = 0;

    static inline uint32_t _ssaoFbo = 0, _ssaoFboBlur = 0;
    static inline uint32_t _ssaoColorBuffer = 0, _ssaoColorBufferBlur = 0;

    static inline glm::vec3 _ssaoKernel[64];
    static inline glm::vec3 _ssaoNoise[16];
    static inline uint32_t _ssaoNoiseTexture = 0;

    static inline uint32_t _vao = 0, _vbo = 0;

    static inline void CreateGBuffer(uint32_t screenWidth, uint32_t screenHeight) {
        glCreateFramebuffers(1, &_gBuffer);

        glCreateTextures(GL_TEXTURE_2D, 1, &_gPosition);
        glTextureStorage2D(_gPosition, 1, GL_RGBA16F, screenWidth, screenHeight);
        glTextureParameteri(_gPosition, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(_gPosition, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(_gPosition, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(_gPosition, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glNamedFramebufferTexture(_gBuffer, GL_COLOR_ATTACHMENT0, _gPosition, 0);

        glCreateTextures(GL_TEXTURE_2D, 1, &_gNormal);
        glTextureStorage2D(_gNormal, 1, GL_RGBA16F, screenWidth, screenHeight);
        glTextureParameteri(_gNormal, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(_gNormal, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glNamedFramebufferTexture(_gBuffer, GL_COLOR_ATTACHMENT1, _gNormal, 0);

        glCreateTextures(GL_TEXTURE_2D, 1, &_gAlbedo);
        glTextureStorage2D(_gAlbedo, 1, GL_RGBA8, screenWidth, screenHeight);
        glTextureParameteri(_gAlbedo, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(_gAlbedo, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glNamedFramebufferTexture(_gBuffer, GL_COLOR_ATTACHMENT2, _gAlbedo, 0);

        glCreateRenderbuffers(1, &_rboDepth);
        glNamedRenderbufferStorage(_rboDepth, GL_DEPTH_COMPONENT24, screenWidth, screenHeight);
        glNamedFramebufferRenderbuffer(_gBuffer, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _rboDepth);

        glBindFramebuffer(GL_FRAMEBUFFER, _gBuffer);
        glDrawBuffers(3, (uint32_t[]){ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 });
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        if (glCheckNamedFramebufferStatus(_gBuffer, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "G Buffer incomplete\n");
            exit(-1);
        }
    }

    static inline void CreateFramebuffer(uint32_t screenWidth, uint32_t screenHeight) {
        glCreateFramebuffers(1, &_ssaoFbo);

        glCreateTextures(GL_TEXTURE_2D, 1, &_ssaoColorBuffer);
        glTextureStorage2D(_ssaoColorBuffer, 1, GL_R16F, screenWidth, screenHeight);
        glTextureParameteri(_ssaoColorBuffer, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(_ssaoColorBuffer, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glNamedFramebufferTexture(_ssaoFbo, GL_COLOR_ATTACHMENT0, _ssaoColorBuffer, 0);

        if (glCheckNamedFramebufferStatus(_ssaoFbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "SSAO Framebuffer incomplete\n");
            exit(-1);
        }

        glCreateFramebuffers(2, &_ssaoFboBlur);

        glCreateTextures(GL_TEXTURE_2D, 1, &_ssaoColorBufferBlur);
        glTextureStorage2D(_ssaoColorBufferBlur, 1, GL_R16F, screenWidth, screenHeight);
        glTextureParameteri(_ssaoColorBufferBlur, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(_ssaoColorBufferBlur, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glNamedFramebufferTexture(_ssaoFboBlur, GL_COLOR_ATTACHMENT0, _ssaoColorBufferBlur, 0);

        if (glCheckNamedFramebufferStatus(_ssaoFboBlur, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "SSAO Blur Framebuffer incomplete\n");
            exit(-1);
        }
    }

    static inline void CreateKernelNoise() {
        std::uniform_real_distribution<float> randFloat(0.0f, 1.0f);
        std::default_random_engine gen;

        for (int i = 0; i < 64; ++i) {
            glm::vec3 sample(randFloat(gen) * 2.0f - 1.0f, randFloat(gen) * 2.0f - 1.0f, randFloat(gen));
            sample = glm::normalize(sample);
            sample *= randFloat(gen);
            float scale = (float)i / 64.0f;

            scale = 0.1f + scale * scale * (1.0f - 0.1f); 
            sample *= scale;
            _ssaoKernel[i] = sample;
        }

        for (int i = 0; i < 16; ++i) {
            glm::vec3 noise(randFloat(gen) * 2.0f - 1.0f, randFloat(gen) * 2.0f - 1.0f, 0.0f);
            _ssaoNoise[i] = noise;
        }

        glCreateTextures(GL_TEXTURE_2D, 1, &_ssaoNoiseTexture);
        glTextureStorage2D(_ssaoNoiseTexture, 1, GL_RGBA32F, 4, 4);
        glTextureSubImage2D(_ssaoNoiseTexture, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, _ssaoNoise);
        glTextureParameteri(_ssaoNoiseTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(_ssaoNoiseTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(_ssaoNoiseTexture, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTextureParameteri(_ssaoNoiseTexture, GL_TEXTURE_WRAP_T, GL_REPEAT);
    }

    static inline void Init(uint32_t screenWidth, uint32_t screenHeight) {
        CreateGBuffer(screenWidth, screenHeight);
        CreateFramebuffer(screenWidth, screenHeight);
        CreateKernelNoise();

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
        GLUtil::DestroyFramebuffer(_gBuffer);
        GLUtil::DestroyTexture(_gPosition);
        GLUtil::DestroyTexture(_gNormal);
        GLUtil::DestroyTexture(_gAlbedo);
        GLUtil::DestroyFramebuffer(_ssaoFbo);
        GLUtil::DestroyTexture(_ssaoColorBuffer);
        GLUtil::DestroyFramebuffer(_ssaoFboBlur);
        GLUtil::DestroyTexture(_ssaoColorBufferBlur);

        CreateGBuffer(screenWidth, screenHeight);
        CreateFramebuffer(screenWidth, screenHeight);
    }

    static inline void BindFramebuffer() {
        glBindFramebuffer(GL_FRAMEBUFFER, _gBuffer);
    }

    static inline void UnbindFramebuffer() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    static inline void GenTexture(const Shader &shader, const glm::mat4 &projection) {
        glBindFramebuffer(GL_FRAMEBUFFER, _ssaoFbo);
        glClear(GL_COLOR_BUFFER_BIT);
    
        shader.Use();
        for (int i = 0; i < 64; ++i) {
            shader.SetVector3("samples[" + std::to_string(i) + "]", _ssaoKernel[i]);
        }
        shader.SetMatrix4("projection", projection);

        glBindTextureUnit(0, _gPosition);
        glBindTextureUnit(1, _gNormal);
        glBindTextureUnit(2, _ssaoNoiseTexture);

        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    static inline void RenderWithLighting() {
        glBindTextureUnit(0, _gPosition);
        glBindTextureUnit(1, _gNormal);
        glBindTextureUnit(2, _gAlbedo);
        glBindTextureUnit(3, _ssaoColorBufferBlur);

        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }

    static inline void BlurColorBuffer(const Shader &shader) {
        glBindFramebuffer(GL_FRAMEBUFFER, _ssaoFboBlur);
        glClear(GL_COLOR_BUFFER_BIT);

        shader.Use();
        glBindTextureUnit(0, _ssaoColorBuffer);
        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
