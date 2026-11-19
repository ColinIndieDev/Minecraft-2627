#pragma once

#include <cstdint>

#include <glad/glad.h>

#include "GLUtil.hpp"

namespace HDR {
    static inline uint32_t _hdrFbo = 0, _rboDepth = 0; 
    static inline uint32_t _colorBuffers[2] = {0};
    static inline uint32_t _msaaHdrFbo = 0, _msaaRboDepth = 0; 
    static inline uint32_t _msaaColorBuffers[2] = {0};
    static inline uint32_t _msaaSamples = 4;
    static inline uint32_t _vao = 0, _vbo = 0;

    static inline void CreateFramebuffer(uint32_t screenWidth, uint32_t screenHeight) {
        glCreateFramebuffers(1, &_hdrFbo);

        glCreateTextures(GL_TEXTURE_2D, 2, _colorBuffers);
        for (int i = 0; i < 2; ++i) {
            glTextureStorage2D(_colorBuffers[i], 1, GL_RGBA16F, screenWidth, screenHeight);
            glTextureParameteri(_colorBuffers[i], GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTextureParameteri(_colorBuffers[i], GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glNamedFramebufferTexture(_hdrFbo, GL_COLOR_ATTACHMENT0 + i, _colorBuffers[i], 0);
        }

        glCreateRenderbuffers(1, &_rboDepth);
        glNamedRenderbufferStorage(_rboDepth, GL_DEPTH_COMPONENT24, screenWidth, screenHeight);

        glNamedFramebufferRenderbuffer(_hdrFbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _rboDepth);

        if (glCheckNamedFramebufferStatus(_hdrFbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "HDR Framebuffer incomplete\n");
            exit(-1);
        }
    }

    static inline void CreateMSAAFramebuffer(uint32_t screenWidth, uint32_t screenHeight) {
        glCreateFramebuffers(1, &_msaaHdrFbo);

        glCreateTextures(GL_TEXTURE_2D_MULTISAMPLE, 2, _msaaColorBuffers);
        for (int i = 0; i < 2; ++i) {
            glTextureStorage2DMultisample(_msaaColorBuffers[i], _msaaSamples, GL_RGBA16F, screenWidth, screenHeight, GL_TRUE);

            glNamedFramebufferTexture(_msaaHdrFbo, GL_COLOR_ATTACHMENT0 + i, _msaaColorBuffers[i], 0);
        }

        glCreateRenderbuffers(1, &_msaaRboDepth);
        glNamedRenderbufferStorageMultisample(_msaaRboDepth, _msaaSamples, GL_DEPTH_COMPONENT24, screenWidth, screenHeight);

        glNamedFramebufferRenderbuffer(_msaaHdrFbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _msaaRboDepth);

        if (glCheckNamedFramebufferStatus(_msaaHdrFbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "MSAA HDR Framebuffer incomplete\n");
            exit(-1);
        }
    }

    static inline void Init(uint32_t screenWidth, uint32_t screenHeight) {
        CreateFramebuffer(screenWidth, screenHeight);
        CreateMSAAFramebuffer(screenWidth, screenHeight);

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
        GLUtil::DestroyFramebuffer(_hdrFbo);
        GLUtil::DestroyTexture(_colorBuffers[0]);
        GLUtil::DestroyTexture(_colorBuffers[1]);
        GLUtil::DestroyRenderbuffer(_rboDepth);
        GLUtil::DestroyFramebuffer(_msaaHdrFbo);
        GLUtil::DestroyTexture(_msaaColorBuffers[0]);
        GLUtil::DestroyTexture(_msaaColorBuffers[1]);
        GLUtil::DestroyRenderbuffer(_msaaRboDepth);

        CreateFramebuffer(screenWidth, screenHeight);
        CreateMSAAFramebuffer(screenWidth, screenHeight);
    }

    static inline void BindFramebuffer(bool msaa) {
        glBindFramebuffer(GL_FRAMEBUFFER, msaa ? _msaaHdrFbo : _hdrFbo);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    static inline void ResolveFramebuffer(uint32_t screenWidth, uint32_t screenHeight) {
        // Ofc assuming screen size = framebuffer size
        glBlitNamedFramebuffer(_msaaHdrFbo,_hdrFbo, 0, 0, screenWidth, screenHeight, 0, 0, screenWidth, screenHeight, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    }

    static inline void UnbindFramebuffer() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    static inline void DrawScreenQuad() {
        glBindVertexArray(_vao);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);
    }
}
