#pragma once

#include "GLUtil.hpp"
#include <glad/glad.h>

struct DirectionalShadow {
    uint32_t depthMapFbo = 0, depthMap = 0;
    uint32_t resolution = 0;

    DirectionalShadow(uint32_t resolution) : resolution(resolution) {
        if (resolution <= 0) this->resolution = 16;

        glCreateTextures(GL_TEXTURE_2D, 1, &depthMap);
        glTextureStorage2D(depthMap, 1, GL_DEPTH_COMPONENT32F, resolution, resolution);
        glTextureSubImage2D(depthMap, 0, 0, 0, resolution, resolution, GL_DEPTH_COMPONENT, GL_FLOAT, (void *)0);
        glTextureParameteri(depthMap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(depthMap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(depthMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(depthMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTextureParameterfv(depthMap, GL_TEXTURE_BORDER_COLOR, (float[4]){1.0f, 1.0f, 1.0f, 1.0f});
        
        glCreateFramebuffers(1, &depthMapFbo);
        glNamedFramebufferTexture(depthMapFbo, GL_DEPTH_ATTACHMENT, depthMap, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFbo);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    ~DirectionalShadow() {
        GLUtil::DestroyTexture(depthMap);
        GLUtil::DestroyFramebuffer(depthMapFbo);
    }

    void Resize(uint32_t resolution) {
        this->resolution = resolution;
        if (resolution <= 0) this->resolution = 16;

        GLUtil::DestroyTexture(depthMap);
        glCreateTextures(GL_TEXTURE_2D, 1, &depthMap);
        glTextureStorage2D(depthMap, 1, GL_DEPTH_COMPONENT32F, resolution, resolution);
        glTextureSubImage2D(depthMap, 0, 0, 0, resolution, resolution, GL_DEPTH_COMPONENT, GL_FLOAT, (void *)0);
        glTextureParameteri(depthMap, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(depthMap, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(depthMap, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(depthMap, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTextureParameterfv(depthMap, GL_TEXTURE_BORDER_COLOR, (float[4]){1.0f, 1.0f, 1.0f, 1.0f});
        
        glNamedFramebufferTexture(depthMapFbo, GL_DEPTH_ATTACHMENT, depthMap, 0);
    }

    void BindDepthMap() {
        glViewport(0, 0, resolution, resolution);
        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFbo);
        glClear(GL_DEPTH_BUFFER_BIT);
        glEnable(GL_DEPTH_TEST);
        // No front culling to prevent peter panning because if using this, the shadows are offset wrong
        // and we do not want this
        // glCullFace(GL_FRONT);
    }

    void UnbindDepthMap() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        // glCullFace(GL_BACK);
    }
};
