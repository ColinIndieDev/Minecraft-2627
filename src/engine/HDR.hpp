#pragma once

#include <cstdint>

#include <glad/glad.h>

namespace HDR {
    inline uint32_t _hdrFbo = 0, _rboDepth = 0; 
    inline uint32_t _colorBuffers[2] = {0};
    inline uint32_t _msaaHdrFbo = 0, _msaaRboDepth = 0; 
    inline uint32_t _msaaColorBuffers[2] = {0};
    inline uint32_t _msaaSamples = 4;
    inline uint32_t _vao = 0, _vbo = 0;

    void CreateFramebuffer(uint32_t screenWidth, uint32_t screenHeight);
    void CreateMSAAFramebuffer(uint32_t screenWidth, uint32_t screenHeight);

    void Init(uint32_t screenWidth, uint32_t screenHeight);
    void Resize(uint32_t screenWidth, uint32_t screenHeight);
    void Destroy();

    void BindFramebuffer(bool msaa);
    void ResolveFramebuffer(uint32_t screenWidth, uint32_t screenHeight);
    void UnbindFramebuffer();

    void DrawScreenQuad();
}
