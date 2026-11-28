#pragma once

#include <cstdint>
#include <vector>

#include <imgui.h>

#include "Minecraft.hpp"

struct ShadowConfig {
    uint32_t lastShadowResolution = 4096;
    uint32_t shadowResolution = lastShadowResolution;
    bool drawShadows = false;
    std::vector<float> shadowCascadeLevels { 
        ZFAR / 100.0f, ZFAR / 70.0f, ZFAR / 50.0f, ZFAR / 25.0f 
    };
};

struct FogConfig {
    int lastFogStartChunk = DEFAULT_VIEW_DISTANCE - 2;
    int lastFogEndChunk = DEFAULT_VIEW_DISTANCE;
    int fogStartChunk = lastFogStartChunk;
    int fogEndChunk = lastFogEndChunk;
    bool fog = false;
};

struct HdrConfig {
    bool hdr = false;
    float exposure = 1.0f;
};

struct BloomConfig {
    bool bloom = false;
    int totalBlurAmount = 10;
    float bloomIntensity = 1.0f;
};

struct GodRayConfig {
    bool godRays = false;
    int samples = 100;
    float decay = 0.96f;
    float density = 0.9f;
    float weight = 0.5f;
    float godRayExposure = 0.3f;
};

struct EnvironmentConfig {
    glm::vec3 dirLightDirection = glm::vec3(-0.2f, -0.3f, -0.5f);
    glm::vec3 dirLightAmbient = glm::vec3(0.1f);
    glm::vec3 dirLightDiffuse = glm::vec3(0.85f);
    glm::vec3 dirLightSpecular = glm::vec3(0.1f);

    glm::vec3 sunColor = glm::vec3(1.0f, 1.0f, 0.3f);
    glm::vec3 skyboxTint = glm::vec3(1.0f, 1.0f, 1.0f);

    glm::vec4 grassColor  = glm::vec4(67 / 255.0f, 255 / 255.0f, 32  / 255.0f, 255 / 255.0f);
    glm::vec4 leavesColor = glm::vec4(20 / 255.0f, 156 / 255.0f, 45  / 255.0f, 255 / 255.0f);
    glm::vec4 waterColor  = glm::vec4(12 / 255.0f, 155 / 255.0f, 220 / 255.0f, 200 / 255.0f);
};

struct NormalMappingConfig {
    float normalStrength = 1.0f;
    bool normalMapping = false;
};

struct MsaaConfig {
    bool prevMsaa = false;
    bool msaa = false;
};

namespace Config {
    inline ShadowConfig Shadow;
    inline FogConfig Fog;
    inline HdrConfig Hdr;
    inline BloomConfig Bloom;
    inline GodRayConfig GodRay;
    inline EnvironmentConfig Environment;
    inline NormalMappingConfig NormalMapping;
    inline MsaaConfig Msaa;

    inline ImFont* MenuFont = NULL;
    inline ImFont* MenuFontBig = NULL;
    inline bool ShouldRenderMenu = false;

    void InitMenu();
    void DisplayMenu();
    void RenderMenu();
    void DestroyMenu();
}
