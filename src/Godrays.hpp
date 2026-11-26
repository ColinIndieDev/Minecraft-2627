#pragma once

#include <cstdint>

#include <glad/glad.h>

#include "GLUtil.hpp"

namespace Godrays {
    static inline uint32_t _occlusionFbo = 0, _occlusionColorBuffer = 0, _occlusionRboDepth = 0;

    static inline void CreateFramebuffer(uint32_t screenWidth, uint32_t screenHeight) {
        glCreateFramebuffers(1, &_occlusionFbo);

        glCreateTextures(GL_TEXTURE_2D, 1, &_occlusionColorBuffer);
        glTextureStorage2D(_occlusionColorBuffer, 1, GL_RGBA8, screenWidth, screenHeight);
        glTextureParameteri(_occlusionColorBuffer, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(_occlusionColorBuffer, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glCreateRenderbuffers(1, &_occlusionRboDepth);
        glNamedRenderbufferStorage(_occlusionRboDepth, GL_DEPTH_COMPONENT24, screenWidth, screenHeight);

        glNamedFramebufferRenderbuffer(_occlusionFbo, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, _occlusionRboDepth);

        glNamedFramebufferTexture(_occlusionFbo, GL_COLOR_ATTACHMENT0, _occlusionColorBuffer, 0);

        if (glCheckNamedFramebufferStatus(_occlusionFbo, GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "Occlusion Framebuffer incomplete\n");
            exit(-1);
        }
    }

    static inline void Init(uint32_t screenWidth, uint32_t screenHeight) {
        CreateFramebuffer(screenWidth, screenHeight); 
    }

    static inline void Resize(uint32_t screenWidth, uint32_t screenHeight) {
        GLUtil::DestroyFramebuffer(_occlusionFbo);
        GLUtil::DestroyRenderbuffer(_occlusionRboDepth);
        GLUtil::DestroyTexture(_occlusionColorBuffer);

        CreateFramebuffer(screenWidth, screenHeight); 
    }

    static inline void BindOcclusionFramebuffer() {
        glBindFramebuffer(GL_FRAMEBUFFER, _occlusionFbo);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    static inline void UnbindOcclusionFramebuffer() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
}
