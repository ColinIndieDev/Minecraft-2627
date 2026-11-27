#pragma once

#include <cstdint>

#include <glad/glad.h>

namespace Godrays {
    inline uint32_t _occlusionFbo = 0, _occlusionColorBuffer = 0, _occlusionRboDepth = 0;

    void CreateFramebuffer(uint32_t screenWidth, uint32_t screenHeight);

    void Init(uint32_t screenWidth, uint32_t screenHeight);
    void Resize(uint32_t screenWidth, uint32_t screenHeight);

    void BindOcclusionFramebuffer();
    void UnbindOcclusionFramebuffer();
}
