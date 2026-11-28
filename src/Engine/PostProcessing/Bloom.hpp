#pragma once

#include <cstdint>

#include <glad/glad.h>

#include <Core/Shader.hpp>

namespace Bloom {
    inline uint32_t PingPongFbo[2] = {0};
    inline uint32_t PingPongColorBuffers[2] = {0};
    inline uint32_t Vao = 0, Vbo = 0;

    void CreateFramebuffers(uint32_t screenWidth, uint32_t screenHeight);

    void Init   (uint32_t screenWidth, uint32_t screenHeight);
    void Resize (uint32_t screenWidth, uint32_t screenHeight);
    void Destroy();

    bool BlurColorBuffer(const Shader &shader, uint32_t colorBuffer, int totalBlurAmount);
}
