#include "Cubemap.hpp"
#include "Camera3D.hpp"

#include <glad/glad.h>
#include <glm/glm.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

void FlipImageHorizontal(uint8_t *pixels, int width, int height, int channels) {
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width / 2; x++) {
            for (int c = 0; c < channels; c++) {
                int idx1 = (y * width + x) * channels + c;
                int idx2 = (y * width + (width - 1 - x)) * channels + c;
                uint8_t temp = pixels[idx1];
                pixels[idx1] = pixels[idx2];
                pixels[idx2] = temp;
            }
        }
    }
}

Cubemap::Cubemap(std::string paths[6]) {
    float vertices[108] = {
        -1.0f,  1.0f, -1.0f, 
        -1.0f, -1.0f, -1.0f, 
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, 
         1.0f,  1.0f, -1.0f, 
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,  
        -1.0f, -1.0f, -1.0f, 
        -1.0f,  1.0f, -1.0f,
        -1.0f,  1.0f, -1.0f, 
        -1.0f,  1.0f,  1.0f,  
        -1.0f, -1.0f,  1.0f,

         1.0f, -1.0f, -1.0f, 
         1.0f, -1.0f,  1.0f,  
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  
         1.0f,  1.0f, -1.0f, 
         1.0f, -1.0f, -1.0f,

        -1.0f, -1.0f,  1.0f,  
        -1.0f,  1.0f,  1.0f,  
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  
         1.0f, -1.0f,  1.0f,  
        -1.0f, -1.0f,  1.0f,

        -1.0f,  1.0f, -1.0f, 
         1.0f,  1.0f, -1.0f, 
         1.0f,  1.0f,  1.0f,
         1.0f,  1.0f,  1.0f,  
        -1.0f,  1.0f,  1.0f,  
        -1.0f,  1.0f, -1.0f,

        -1.0f, -1.0f, -1.0f, 
        -1.0f, -1.0f,  1.0f,  
         1.0f, -1.0f, -1.0f,
         1.0f, -1.0f, -1.0f, 
        -1.0f, -1.0f,  1.0f,  
         1.0f, -1.0f,  1.0f
    };

    glCreateVertexArrays(1, &vao);
    glCreateBuffers(1, &vbo);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, 3 * sizeof(float));
    glNamedBufferStorage(vbo, sizeof(vertices), &vertices, 0);
    glEnableVertexArrayAttrib(vao, 0);
    glVertexArrayAttribFormat(vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glVertexArrayAttribBinding(vao, 0, 0);

    glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &texture);

    int width, height, channels;

    {
        uint8_t *data = stbi_load(paths[0].c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data) {
            fprintf(stderr, "Failed to load texture at path: %s", paths[0].c_str());
            stbi_image_free(data);
            exit(-1);
        }
        glTextureStorage2D(texture, 1, GL_SRGB8_ALPHA8, width, height);
        FlipImageHorizontal(data, width, height, channels);
        glTextureSubImage3D(texture, 0, 0, 0, 0, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }

    for (int i = 1; i < 6; ++i) {
        uint8_t *data = stbi_load(paths[i].c_str(), &width, &height, &channels, STBI_rgb_alpha);
        if (!data) {
            fprintf(stderr, "Failed to load texture at path: %s", paths[i].c_str());
            stbi_image_free(data);
            exit(-1);
        }
        FlipImageHorizontal(data, width, height, channels);
        glTextureSubImage3D(texture, 0, 0, 0, i, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, data);
        stbi_image_free(data);
    }
    glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTextureParameteri(texture, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
}

void Cubemap::Draw(glm::vec3 rotation, glm::vec4 color, Camera3D &camera, Shader &shader, const glm::mat4 &projection) {
    glDepthFunc(GL_LEQUAL);

    shader.Use();

    glm::mat4 view = camera.GetViewMatrix();
    view = glm::mat4(glm::mat3(view));

    glm::mat4 rotationMat = glm::mat4(1);
    rotationMat = glm::rotate(rotationMat, glm::radians(rotation.x), glm::vec3(1, 0, 0));
    rotationMat = glm::rotate(rotationMat, glm::radians(rotation.y), glm::vec3(0, 1, 0));
    rotationMat = glm::rotate(rotationMat, glm::radians(rotation.z), glm::vec3(0, 0, 1));

    shader.SetMatrix4("projection", projection);
    shader.SetMatrix4("view", view * rotationMat);
    shader.SetColor("inputColor", color);

    glBindVertexArray(vao);
    glBindTextureUnit(0, texture);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
}
