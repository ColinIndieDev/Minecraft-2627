#include "Model.hpp"

uint32_t LoadTextureFromFile(const std::string &path, const std::string &directory) {
    std::string filename = std::string(path);
    filename = directory + '/' + filename;

    uint32_t id = 0;
    glCreateTextures(GL_TEXTURE_2D, 1, &id);
    glTextureParameteri(id, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTextureParameteri(id, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTextureParameteri(id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTextureParameteri(id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    int width = 0, height = 0, channels = 0;
    uint8_t *data = stbi_load(filename.c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (data) {
        glTextureStorage2D(id, 1, GL_SRGB8_ALPHA8, width, height);
        glTextureSubImage2D(id, 0, 0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateTextureMipmap(id);
    } else {
        fprintf(stderr, "Failed to load texture at path: %s\n", path.c_str());
        exit(-1);
    }
    stbi_image_free(data);

    return id;
}

MeshVertex::MeshVertex(const glm::vec3 &position, const glm::vec3 &normal, const glm::vec2 &uv) : position(position), normal(normal), uv(uv) {}

MeshTexture::MeshTexture(uint32_t id, const std::string &type, const std::string &path) : id(id), type(type), path(path) {}

Mesh::Mesh(std::vector<MeshVertex> &vertices, std::vector<uint32_t> &indices, std::vector<MeshTexture> &textures) : vertices(vertices), indices(indices), textures(textures) {
    glCreateVertexArrays(1, &vao);
    glCreateBuffers     (1, &vbo);
    glCreateBuffers     (1, &ebo);

    glNamedBufferData        (vbo, vertices.size() * sizeof(MeshVertex), vertices.data(), GL_STATIC_DRAW);
    glVertexArrayVertexBuffer(vao, 0, vbo, 0, sizeof(MeshVertex));

    glNamedBufferData         (ebo, indices.size() * sizeof(uint32_t), indices.data(), GL_STATIC_DRAW);
    glVertexArrayElementBuffer(vao, ebo);

    glVertexArrayAttribFormat (vao, 0, 3, GL_FLOAT, GL_FALSE, 0);
    glEnableVertexArrayAttrib (vao, 0);
    glVertexArrayAttribFormat (vao, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float));
    glEnableVertexArrayAttrib (vao, 1);
    glVertexArrayAttribFormat (vao, 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(float));
    glEnableVertexArrayAttrib (vao, 2);
    glVertexArrayAttribBinding(vao, 0, 0);
    glVertexArrayAttribBinding(vao, 1, 0);
    glVertexArrayAttribBinding(vao, 2, 0);
}

void Mesh::Draw(const Shader &shader) {
    uint32_t diffuseNum = 1;
    uint32_t specularNum = 1;
    for (int i = 0; i < textures.size(); ++i) {
        std::string number = "";
        std::string name = textures[i].type;

        if (name == "texture_diffuse")       number = std::to_string(diffuseNum++);
        else if (name == "texture_specular") number = std::to_string(specularNum++);

        shader.SetInt("material." + name + number, i);
        glBindTextureUnit(i, textures[i].id);
    }
    glBindVertexArray(vao);
    glDrawElements   (GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);
}

std::vector<MeshTexture> Model::LoadMaterialTextures(aiMaterial *mat, aiTextureType type, std::string typeName) {
    std::vector<MeshTexture> textures;
    textures.reserve(mat->GetTextureCount(type));
    for (int i = 0; i < mat->GetTextureCount(type); ++i) {
        aiString str;
        mat->GetTexture(type, i, &str);
        bool skip = false;
        for (int j = 0; j < texturesLoaded.size(); ++j) {
            if (strcmp(texturesLoaded[j].path.data(), str.C_Str()) == 0) {
                textures.emplace_back(texturesLoaded[j]);
                skip = true; 
                break;
            }
        }
        if (!skip) {
            MeshTexture texture(LoadTextureFromFile(str.C_Str(), directory), typeName, str.C_Str());
            textures.emplace_back(texture);
            texturesLoaded.emplace_back(texture);
        }
    }
    return textures;
}

Mesh Model::ProcessMesh(aiMesh *mesh, const aiScene *scene) {
    std::vector<MeshVertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<MeshTexture> textures;

    vertices.reserve(mesh->mNumVertices);
    indices.reserve(mesh->mNumFaces);

    for (int i = 0; i < mesh->mNumVertices; ++i) {
        glm::vec2 uv = glm::vec2(0.0f);
        if (mesh->mTextureCoords[0]) {
            uv = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
        }
        vertices.emplace_back(glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z), 
                              glm::vec3(mesh->mNormals[i].x,  mesh->mNormals[i].y,  mesh->mNormals[i].z),
                              uv);
    }
    for (int i = 0; i < mesh->mNumFaces; ++i) {
        aiFace face = mesh->mFaces[i];
        for (int j = 0; j < face.mNumIndices; ++j) {
            indices.emplace_back(face.mIndices[j]);
        }
    }
    if (mesh->mMaterialIndex >= 0) {
        aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];

        std::vector<MeshTexture> diffuseMaps = LoadMaterialTextures(material, aiTextureType_DIFFUSE, "texture_diffuse");
        textures.insert(textures.end(), diffuseMaps.begin(), diffuseMaps.end());

        std::vector<MeshTexture> specularMaps = LoadMaterialTextures(material, aiTextureType_SPECULAR, "texture_specular");
        textures.insert(textures.end(), specularMaps.begin(), specularMaps.end());
    }
    return Mesh(vertices, indices, textures);
}    

void Model::ProcessNode(aiNode *node, const aiScene *scene) {
    for (int i = 0; i < node->mNumMeshes; ++i) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        meshes.emplace_back(ProcessMesh(mesh, scene));
    }
    for (int i = 0; i < node->mNumChildren; ++i) {
        ProcessNode(node->mChildren[i], scene);
    }
}

Model::Model(const std::string &path) {
    Assimp::Importer import;
    const aiScene *scene = import.ReadFile(path, aiProcess_Triangulate | aiProcess_FlipUVs);

    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        fprintf(stderr, "Assimp: %s\n", import.GetErrorString());
        exit(-1);
    }
    directory = path.substr(0, path.find_last_of('/'));

    ProcessNode(scene->mRootNode, scene);
}

void Model::Draw(const Shader &shader) {
    for (auto &mesh : meshes) {
        mesh.Draw(shader);
    }
}
