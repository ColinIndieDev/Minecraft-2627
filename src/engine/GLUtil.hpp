#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include <glad/glad.h>
#include <stb_image.h>

namespace GLUtil {
    struct TextureCreateData {
        const std::string &path = "";
        int *width = NULL;
        int *height = NULL;
        int *channels = NULL;
        int filter = GL_NEAREST;
        int wrapping = GL_CLAMP_TO_EDGE;
    };

    struct TextureArrayCreateData {
        const std::vector<std::string> &paths;
        int *width = NULL;
        int *height = NULL;
        int *channels = NULL;
        int filter = GL_NEAREST;
        int wrapping = GL_CLAMP_TO_EDGE;
        bool gammaCorrection = true;
    };

    uint32_t LoadTexture(const TextureCreateData &data);
    uint32_t LoadTextureArray(const TextureArrayCreateData &data);

    void DestroyVertexArray(uint32_t &vao);
    void DestroyVertexBuffer(uint32_t &vbo);
    void DestroyTexture(uint32_t &texture);
    void DestroyFramebuffer(uint32_t &fbo);
    void DestroyRenderbuffer(uint32_t &rbo);
}
