#include "GLUtil.hpp"

namespace GLUtil {
    uint32_t LoadTexture(const TextureCreateData &data) {
        uint32_t texture = 0;
        glCreateTextures(GL_TEXTURE_2D, 1, &texture);

        glTextureParameteri(texture, GL_TEXTURE_WRAP_S, data.wrapping);
        glTextureParameteri(texture, GL_TEXTURE_WRAP_T, data.wrapping);
        glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, data.filter);
        glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, data.filter);

        int w = 0, h = 0, c = 0;
        uint8_t *pixels = stbi_load(data.path.c_str(), &w, &h, &c, 4);
        if (pixels) {
            glTextureStorage2D(texture, 1, GL_SRGB8_ALPHA8, w, h);
            glTextureSubImage2D(texture, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            glGenerateTextureMipmap(texture);
        } else {
            fprintf(stderr, "Failed to load texture at path: %s\n", data.path.c_str());
            exit(-1);
        }
        stbi_image_free(pixels);

        if (data.width) *data.width = w;
        if (data.height) *data.height = h;
        if (data.channels) *data.channels = c;

        return texture;
    }

    uint32_t LoadTextureArray(const TextureArrayCreateData &data) {
        uint32_t textureArray = 0;
        glCreateTextures(GL_TEXTURE_2D_ARRAY, 1, &textureArray);

        glTextureParameteri(textureArray, GL_TEXTURE_WRAP_S, data.wrapping);
        glTextureParameteri(textureArray, GL_TEXTURE_WRAP_T, data.wrapping);
        glTextureParameteri(textureArray, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_NEAREST);
        glTextureParameteri(textureArray, GL_TEXTURE_MAG_FILTER, data.filter);

        int width, height, channels;
        for (int i = 0; i < data.paths.size(); ++i) {
            int w, h, c;
            auto &path = data.paths[i];
            uint8_t *pixels = stbi_load(path.c_str(), &w, &h, &c, 4);
            if (i == 0) {
                glTextureStorage3D(textureArray, 1, data.gammaCorrection ? GL_SRGB8_ALPHA8 : GL_RGBA8, w, h, data.paths.size()); 

                width = h;
                height = h;
                channels = c;
            } else if (width != w || height != h) {
                fprintf(stderr, "Texture size does not match with the array\n");
                exit(-1);
            }

            if (pixels) {
                glTextureSubImage3D(textureArray, 0, 0, 0, i, w, h, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
                glGenerateTextureMipmap(textureArray);
            } else {
                fprintf(stderr, "Failed to load texture at path: %s\n", path.c_str());
                exit(-1);
            }

            stbi_image_free(pixels);
        }

        if (data.width) *data.width = width;
        if (data.height) *data.height = height;
        if (data.channels) *data.channels = channels;

        return textureArray;
    }

    void DestroyVertexArray(uint32_t &vao) {
        if (vao != 0 && glIsVertexArray(vao)) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
    }

    void DestroyVertexBuffer(uint32_t &vbo) {
        if (vbo != 0 && glIsBuffer(vbo)) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
    }

    void DestroyTexture(uint32_t &texture) {
        if (texture != 0) {
            glDeleteTextures(1, &texture);
            texture = 0;
        }
    }

    void DestroyFramebuffer(uint32_t &fbo) {
        if (fbo != 0 && glIsFramebuffer(fbo)) {
            glDeleteFramebuffers(1, &fbo);
            fbo = 0;
        }
    }

    void DestroyRenderbuffer(uint32_t &rbo) {
        if (rbo != 0 && glIsRenderbuffer(rbo)) {
            glDeleteRenderbuffers(1, &rbo);
            rbo = 0;
        }
    }
}
