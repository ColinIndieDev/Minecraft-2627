#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace DirectionalShadow {
    inline uint32_t _depthMapFbo = 0, _depthMaps = 0;
    inline uint32_t _matricesUBO = 0;
    inline uint32_t _resolution = 0;

    void CreateFramebuffer(uint32_t resolution, uint32_t totalCascades);

    void Init   (uint32_t resolution, uint32_t totalCascades);
    void Resize (uint32_t resolution, uint32_t totalCascades);
    void Destroy();

    void GetFrustumCornersWorldSpace   (glm::vec4 corners[8], const glm::mat4 &projection, const glm::mat4 &view);
    glm::mat4 GetLightSpaceMatrix      (const glm::mat4 &projection, const glm::mat4 &view, const glm::vec3 &lightDir, float near, float far, float aspect, float fov);
    void GetLightSpaceMatrices         (std::vector<glm::mat4> &matrices, const std::vector<float> &shadowCascadeLevels, 
                                        const glm::mat4 &projection, const glm::mat4 &view, const glm::vec3 &lightDir, float near, float far, float aspect, float fov);
    void LoadLightSpaceMatricesToBuffer(const std::vector<float> &shadowCascadeLevels, const glm::mat4 &projection, const glm::mat4 &view, const glm::vec3 &lightDir,
                                        float near, float far, float aspect, float fov);

    void BindDepthMap  ();
    void UnbindDepthMap();
};
