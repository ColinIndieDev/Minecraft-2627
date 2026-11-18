#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "GLUtil.hpp"

struct DirectionalShadow {
    uint32_t depthMapFbo = 0, depthMaps = 0;
    uint32_t matricesUBO = 0;
    uint32_t resolution = 0;

    DirectionalShadow(uint32_t resolution, uint32_t totalCascades) : resolution(resolution) {
        if (resolution <= 0) this->resolution = 16;

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &depthMaps);
        glTextureStorage3D(depthMaps, 1, GL_DEPTH_COMPONENT32F, resolution, resolution, totalCascades);

        glTextureParameteri(depthMaps, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(depthMaps, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(depthMaps, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(depthMaps, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTextureParameterfv(depthMaps, GL_TEXTURE_BORDER_COLOR, (float[4]){1.0f, 1.0f, 1.0f, 1.0f});
        
        glCreateFramebuffers(1, &depthMapFbo);
        glNamedFramebufferTexture(depthMapFbo, GL_DEPTH_ATTACHMENT, depthMaps, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFbo);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        glCreateBuffers(1, &matricesUBO);
        glNamedBufferData(matricesUBO, sizeof(glm::mat4) * 16, (void *)0, GL_DYNAMIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, matricesUBO);
    }

    static void GetFrustumCornersWorldSpace(glm::vec4 corners[8], const glm::mat4 &projection, const glm::mat4 &view) {
        glm::mat4 inv = glm::inverse(projection * view);

        int idx = 0;
        for (int x = 0; x < 2; ++x) {
            for (int y = 0; y < 2; ++y) {
                for (int z = 0; z < 2; ++z) {
                    glm::vec4 pt = inv * glm::vec4(2.0f * x - 1.0f, 2.0f * y - 1.0f, 2.0f * z - 1.0f, 1.0f);
                    corners[idx++] = pt / pt.w;
                }
            }
        }
    }

    static glm::mat4 GetLightSpaceMatrix(const glm::mat4 &projection, const glm::mat4 &view, const glm::vec3 &lightDir, float near, float far, float aspect, float fov) {
        glm::mat4 cascadeProjection = glm::perspective(glm::radians(fov), aspect, near, far);

        glm::vec4 corners[8];
        GetFrustumCornersWorldSpace(corners, cascadeProjection, view);
        
        glm::vec3 center(0);

        for (const auto &v : corners) {
            center += glm::vec3(v);
        }
        center /= 8;

        glm::mat4 lightView = glm::lookAt(center + lightDir, center, glm::vec3(0.0f, 1.0f, 0.0f));

        float minX = std::numeric_limits<float>::max();
        float maxX = std::numeric_limits<float>::lowest();
        float minY = std::numeric_limits<float>::max();
        float maxY = std::numeric_limits<float>::lowest();
        float minZ = std::numeric_limits<float>::max();
        float maxZ = std::numeric_limits<float>::lowest();
        for (const auto& v : corners) {
            const auto trf = lightView * v;
            minX = std::min(minX, trf.x);
            maxX = std::max(maxX, trf.x);
            minY = std::min(minY, trf.y);
            maxY = std::max(maxY, trf.y);
            minZ = std::min(minZ, trf.z);
            maxZ = std::max(maxZ, trf.z);
        }

        constexpr float zPadding = 50.0f;
        minZ -= zPadding;
        maxZ += zPadding;

        glm::mat4 lightProjection = glm::ortho(minX, maxX, minY, maxY, minZ, maxZ);
        return lightProjection * lightView;
    }

    static void GetLightSpaceMatrices(std::vector<glm::mat4> &matrices, const std::vector<float> &shadowCascadeLevels, 
                                      const glm::mat4 &projection, const glm::mat4 &view, const glm::vec3 &lightDir, float near, float far, float aspect, float fov) {
        float previousSplit = near;

        for (size_t i = 0; i < shadowCascadeLevels.size() + 1; ++i) {
            float split = (i < shadowCascadeLevels.size()) ? shadowCascadeLevels[i] : far;
            matrices.push_back(GetLightSpaceMatrix(projection, view, lightDir, previousSplit, split, aspect, fov));
            previousSplit = split;
        }
    }

    void LoadLightSpaceMatricesToBuffer(const std::vector<float> &shadowCascadeLevels, const glm::mat4 &projection, const glm::mat4 &view, const glm::vec3 &lightDir,
                                        float near, float far, float aspect, float fov) {
        std::vector<glm::mat4> matrices;
        GetLightSpaceMatrices(matrices, shadowCascadeLevels, projection, view, lightDir, near, far, aspect, fov);
        glBindBuffer(GL_UNIFORM_BUFFER, matricesUBO);
        for (int i = 0; i < matrices.size(); ++i) {
            glBufferSubData(GL_UNIFORM_BUFFER, i * sizeof(glm::mat4), sizeof(glm::mat4), &matrices[i]);
        }
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    ~DirectionalShadow() {
        GLUtil::DestroyTexture(depthMaps);
        GLUtil::DestroyFramebuffer(depthMapFbo);
    }

    void Resize(uint32_t resolution, uint32_t totalCascades) {
        this->resolution = resolution;
        if (resolution <= 0) this->resolution = 16;

        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &depthMaps);
        glTextureStorage3D(depthMaps, 1, GL_DEPTH_COMPONENT32F, resolution, resolution, totalCascades);

        glTextureParameteri(depthMaps, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTextureParameteri(depthMaps, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTextureParameteri(depthMaps, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTextureParameteri(depthMaps, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTextureParameterfv(depthMaps, GL_TEXTURE_BORDER_COLOR, (float[4]){1.0f, 1.0f, 1.0f, 1.0f});
        
        glCreateFramebuffers(1, &depthMapFbo);
        glNamedFramebufferTexture(depthMapFbo, GL_DEPTH_ATTACHMENT, depthMaps, 0);

        glBindFramebuffer(GL_FRAMEBUFFER, depthMapFbo);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
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
