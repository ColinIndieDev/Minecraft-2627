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

    inline uint32_t LoadTexture(const TextureCreateData &data) {
        uint32_t texture = 0;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, data.wrapping);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, data.wrapping);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, data.filter);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, data.filter);

        int w, h, c;
        uint8_t *pixels = stbi_load(data.path.c_str(), &w, &h, &c, 4);
        if (pixels) {
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glGenerateMipmap(GL_TEXTURE_2D);
        } else {
            fprintf(stderr, "Failed to load texture at path: %s\n", data.path.c_str());
            exit(-1);
        }
        stbi_image_free(pixels);

        if (data.width) *data.width = w;
        if (data.height) *data.height = h;
        if (data.channels) *data.channels = c;

        glBindTexture(GL_TEXTURE_2D, 0);

        return texture;
    }

    struct TextureArrayCreateData {
        const std::vector<std::string> &paths;
        int *width = NULL;
        int *height = NULL;
        int *channels = NULL;
        int filter = GL_NEAREST;
        int wrapping = GL_CLAMP_TO_EDGE;
    };

    inline uint32_t LoadTextureArray(const TextureArrayCreateData &data) {
        uint32_t textureArray = 0;
        glGenTextures(1, &textureArray);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_S, data.wrapping);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_WRAP_T, data.wrapping);
        glGenerateMipmap(GL_TEXTURE_2D_ARRAY);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTexParameteri(GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, data.filter);

        int width, height, channels;
        for (int i = 0; i < data.paths.size(); ++i) {
            int w, h, c;
            auto &path = data.paths[i];
            uint8_t *pixels = stbi_load(path.c_str(), &w, &h, &c, 4);
            if (i == 0) {
                glTexStorage3D(GL_TEXTURE_2D_ARRAY, 1, GL_RGBA8, w, h, data.paths.size()); 
                width = h;
                height = h;
                channels = c;
            } else if (width != w || height != h) {
                fprintf(stderr, "Texture size does not match with the array\n");
                exit(-1);
            }
            if (pixels) {
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, i, w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            } else {
                fprintf(stderr, "Failed to load texture at path: %s\n", path.c_str());
                exit(-1);
            }
            stbi_image_free(pixels);
        }


        if (data.width) *data.width = width;
        if (data.height) *data.height = height;
        if (data.channels) *data.channels = channels;

        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        return textureArray;
    }

    inline void DestroyVertexArray(uint32_t &vao) {
        if (vao != 0 && glIsVertexArray(vao)) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
    }

    inline void DestroyVertexBuffer(uint32_t &vbo) {
        if (vbo != 0 && glIsBuffer(vbo)) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
    }

    inline void DestroyTexture(uint32_t &texture) {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
            texture = 0;
        }
    }
}
