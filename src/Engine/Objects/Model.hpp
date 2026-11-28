#pragma once

#include <string>
#include <vector>

#include <glm/glm.hpp>
#include <glad/glad.h>
#include <stb_image.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <Core/Shader.hpp>

struct MeshVertex {
    glm::vec3 position = glm::vec3(0.0f);
    glm::vec3 normal = glm::vec3(0.0f);
    glm::vec2 uv = glm::vec3(0.0f);
    MeshVertex(const glm::vec3 &position, const glm::vec3 &normal, const glm::vec2 &uv);
};

struct MeshTexture {
    uint32_t id = 0;
    std::string type = "";
    std::string path = "";
    MeshTexture(uint32_t id, const std::string &type, const std::string &path);
};

struct Mesh {
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<MeshTexture> textures;

    uint32_t vao = 0, vbo = 0, ebo = 0;

    Mesh(std::vector<MeshVertex> &vertices, std::vector<uint32_t> &indices, std::vector<MeshTexture> &textures);

    void Draw(const Shader &shader);
};

struct Model {
    std::vector<MeshTexture> texturesLoaded;
    std::vector<Mesh> meshes;
    std::string directory = "";

    Model() = default;
    Model(const std::string &path);

    std::vector<MeshTexture> LoadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName);
    Mesh ProcessMesh(aiMesh *mesh, const aiScene *scene);
    void ProcessNode(aiNode *node, const aiScene *scene);

    void Draw(const Shader &shader);
};
