#include "Chunk.hpp"
#include "BlockType.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glad/glad.h>
#include <FastNoiseLite.h>

#include "GLUtil.hpp"
#include "BlockTexture.hpp"

// Only in order for the shader to understand which face we want to draw / vertex pull
enum class FaceDirection : uint8_t {
    Front = 0,
    Back,
    Top,
    Bottom,
    Right,
    Left
};

void AddVertex(std::vector<int> &list, int x, int y, int z, FaceDirection face, BlockTexture blockTex) {
    int compressedVertex = (z << 20) | (y << 10) | x;
    list.push_back(compressedVertex);
    int compressedData = ((uint8_t)face << 8) | (uint8_t)blockTex;
    list.push_back(compressedData);
}

void AddFrontFace(std::vector<int> &v, int x, int y, int z, BlockTexture blockTex) {
    AddVertex(v, x,   y,   z+1, FaceDirection::Front, blockTex);
}

void AddBackFace(std::vector<int> &v, int x, int y, int z, BlockTexture blockTex) {
    AddVertex(v, x+1, y,   z,   FaceDirection::Back, blockTex);
}

void AddTopFace(std::vector<int> &v, int x, int y, int z, BlockTexture blockTex) {
    AddVertex(v, x,   y+1, z,   FaceDirection::Top, blockTex);
}

void AddBottomFace(std::vector<int> &v, int x, int y, int z, BlockTexture blockTex) {
    AddVertex(v, x,   y, z,     FaceDirection::Bottom, blockTex);
}

void AddRightFace(std::vector<int> &v, int x, int y, int z, BlockTexture blockTex) {
    AddVertex(v, x+1, y,   z+1, FaceDirection::Right, blockTex);
}

void AddLeftFace(std::vector<int> &v, int x, int y, int z, BlockTexture blockTex) {
    AddVertex(v, x,   y,   z,   FaceDirection::Left, blockTex);
}

void Chunk::GenerateTree(int localX, int baseY, int localZ, int height, int &maxHeight) {
    int trunkHeight = height;
    int leavesStart = baseY + trunkHeight - 2;
    int leavesTop = baseY + trunkHeight + 2;

    if (leavesTop > maxHeight) maxHeight = leavesTop;

    for (int y = baseY; y < baseY + trunkHeight; ++y) {
        if (y < MAX_CHUNK_HEIGHT) {
            SetBlock(glm::ivec3(localX, y, localZ), (uint8_t)BlockType::Oak_Log);
        }
    }

    for (int y = leavesStart; y <= leavesTop; ++y) {
        if (y < 0 || y >= MAX_CHUNK_HEIGHT) continue;

        int radius = 2;
        if (y == leavesTop || y == leavesStart) {
            radius = 1;
        }

        for (int x = -radius; x <= radius; ++x) {
            for (int z = -radius; z <= radius; ++z) {
                if (radius == 2 && abs(x) == 2 && abs(z) == 2 && (y == leavesTop - 1 || y == leavesStart + 1)) {
                    continue;
                }

                int targetX = localX + x;
                int targetZ = localZ + z;

                if (targetX >= 0 && targetX < MAX_CHUNK_WIDTH &&
                    targetZ >= 0 && targetZ < MAX_CHUNK_WIDTH) {

                    if (blocks[targetX][y][targetZ] == (uint8_t)BlockType::Air ||
                        blocks[targetX][y][targetZ] == 0) {
                        SetBlock(glm::ivec3(targetX, y, targetZ), (uint8_t)BlockType::Oak_Leaves);
                    }
                }
            }
        }
    }
}

Chunk::Chunk(const glm::ivec2 &position) : position(position) {
    FastNoiseLite noise;
    noise.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    noise.SetFrequency(0.02f);
    noise.SetFractalType(FastNoiseLite::FractalType_FBm);
    noise.SetFractalOctaves(8);
    for (int x = 0; x < MAX_CHUNK_WIDTH; ++x) {
        for (int z = 0; z < MAX_CHUNK_WIDTH; ++z) {
            float height = MIN_PLAINS_HEIGHT;
            float nx = position.x * MAX_CHUNK_WIDTH + x;
            float nz = position.y * MAX_CHUNK_WIDTH + z;
            float offset = 10'000.0f;
            float n = noise.GetNoise(nx + offset, nz + offset);
            if (n > 0) {
                n = powf(n, 2.5f);
            }
            height += (n + 1) * 0.5f * (MAX_PLAINS_HEIGHT - MIN_PLAINS_HEIGHT);

            if ((int)height > maxHeight) {
                maxHeight = (int)height;
            }
            for (int y = 0; y < (int)height; ++y) {
                if (y > height - 13 && y <= SEA_LEVEL) {
                    SetBlock(glm::ivec3(x, y, z), (uint8_t)BlockType::Sand);
                } else if (y >= height - 2) {
                    SetBlock(glm::ivec3(x, y, z), (uint8_t)BlockType::Grass_Block);
                } else if (y > height - 6) {
                    SetBlock(glm::ivec3(x, y, z), (uint8_t)BlockType::Dirt);
                } else {
                    SetBlock(glm::ivec3(x, y, z), (uint8_t)BlockType::Stone);
                }
            }
            for (int waterY = SEA_LEVEL; waterY >= height - 1; waterY--) {
                if (GetBlock(glm::ivec3(x, waterY, z)) == 0) {
                    SetBlock(glm::ivec3(x, waterY, z), (uint8_t)BlockType::Water);
                    if (waterY > maxHeight) {
                        maxHeight = waterY;
                    }
                }
            }
            if (rand() % 50 == 0 && height > SEA_LEVEL) {
                GenerateTree(x, height, z, 3 + rand() % 4, maxHeight);
            }
        }
    }
}

Chunk::~Chunk() {
    GLUtil::DestroyVertexArray(lvao);
    GLUtil::DestroyVertexBuffer(lvbo);
    GLUtil::DestroyVertexArray(vao);
    GLUtil::DestroyVertexBuffer(ssbo);
    GLUtil::DestroyVertexArray(tvao);
    GLUtil::DestroyVertexBuffer(tssbo);
}

void Chunk::SetBlock(const glm::ivec3 &position, uint8_t type) {
    if (position.x >= 0 && position.x < MAX_CHUNK_WIDTH &&
        position.y >= 0 && position.y < MAX_CHUNK_HEIGHT &&
        position.z >= 0 && position.z < MAX_CHUNK_WIDTH) {
        blocks[position.x][position.y][position.z] = type;
    }
}

uint8_t Chunk::GetBlock(const glm::ivec3 &position) {
    if (position.x >= 0 && position.x < MAX_CHUNK_WIDTH &&
        position.y >= 0 && position.y < MAX_CHUNK_HEIGHT &&
        position.z >= 0 && position.z < MAX_CHUNK_WIDTH) {
        return blocks[position.x][position.y][position.z];
    }
    return 0;
}

void Chunk::RebuildMesh(std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>> &chunks) {
    std::vector<int> vertices;
    vertices.reserve(MAX_CHUNK_WIDTH * MAX_CHUNK_WIDTH * maxHeight * 5 * 6);
    std::vector<int> verticesTransparent;
    verticesTransparent.reserve(MAX_CHUNK_WIDTH * MAX_CHUNK_WIDTH * maxHeight * 5 * 6);

    int worldStartX = position.x * MAX_CHUNK_WIDTH;
    int worldStartZ = position.y * MAX_CHUNK_WIDTH;

    for (int x = 0; x < MAX_CHUNK_WIDTH; x++) {
        for (int y = 0; y < MAX_CHUNK_HEIGHT; y++) {
            for (int z = 0; z < MAX_CHUNK_WIDTH; z++) {
                if (blocks[x][y][z] == 0) continue;

                int wx = worldStartX + x;
                int wz = worldStartZ + z;

                auto floorDiv = [](int value, int divisor) {
                    return (int)std::floor((double)value / divisor);
                };

                auto getBlockGlobal = [&](int worldX, int y, int worldZ) -> int {
                    if (y < 0 || y >= MAX_CHUNK_HEIGHT) {
                        return 0;
                    }

                    int chunkX = floorDiv(worldX, MAX_CHUNK_WIDTH);
                    int chunkZ = floorDiv(worldZ, MAX_CHUNK_WIDTH);

                    auto it = chunks.find(glm::ivec2(chunkX, chunkZ));
                    if (it == chunks.end()) {
                        return 0;
                    }

                    int localX = worldX - chunkX * MAX_CHUNK_WIDTH;
                    int localZ = worldZ - chunkZ * MAX_CHUNK_WIDTH;

                    return !BlockIsTransparent((BlockType)it->second->GetBlock(glm::ivec3(localX, y, localZ))) && it->second->GetBlock(glm::ivec3(localX, y, localZ)) != 0;
                };

                auto getBlockGlobalTransparent = [&](int worldX, int y, int worldZ) -> int {
                    if (y < 0 || y >= MAX_CHUNK_HEIGHT) {
                        return 0;
                    }

                    int chunkX = floorDiv(worldX, MAX_CHUNK_WIDTH);
                    int chunkZ = floorDiv(worldZ, MAX_CHUNK_WIDTH);

                    auto it = chunks.find(glm::ivec2(chunkX, chunkZ));
                    if (it == chunks.end()) {
                        return 0;
                    }

                    int localX = worldX - chunkX * MAX_CHUNK_WIDTH;
                    int localZ = worldZ - chunkZ * MAX_CHUNK_WIDTH;

                    return it->second->GetBlock(glm::ivec3(localX, y, localZ));
                };

                if (BlockIsTransparent((BlockType)blocks[x][y][z])) {
                    if (!getBlockGlobalTransparent(wx, y + 1, wz)) AddTopFace   (verticesTransparent, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].top);
                    if (!getBlockGlobalTransparent(wx, y - 1, wz)) AddBottomFace(verticesTransparent, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].bottom);
                    if (!getBlockGlobalTransparent(wx, y, wz + 1)) AddFrontFace (verticesTransparent, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].front);
                    if (!getBlockGlobalTransparent(wx, y, wz - 1)) AddBackFace  (verticesTransparent, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].back);
                    if (!getBlockGlobalTransparent(wx + 1, y, wz)) AddRightFace (verticesTransparent, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].right);
                    if (!getBlockGlobalTransparent(wx - 1, y, wz)) AddLeftFace  (verticesTransparent, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].left);
                } else {
                    if (!getBlockGlobal(wx, y + 1, wz)) AddTopFace   (vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].top);
                    if (!getBlockGlobal(wx, y - 1, wz)) AddBottomFace(vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].bottom);
                    if (!getBlockGlobal(wx, y, wz + 1)) AddFrontFace (vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].front);
                    if (!getBlockGlobal(wx, y, wz - 1)) AddBackFace  (vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].back);
                    if (!getBlockGlobal(wx + 1, y, wz)) AddRightFace (vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].right);
                    if (!getBlockGlobal(wx - 1, y, wz)) AddLeftFace  (vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].left);
                }
            }
        }
    }

    vertexCount = vertices.size() / 2;
    if (vertexCount > 0) {
        vertexBuffer.reserve(vertices.size());
        for (auto v : vertices) {
            vertexBuffer.push_back(v);
        }
    }

    vertexCountTransparent = verticesTransparent.size() / 2;
    if (vertexCountTransparent == 0) return;

    vertexBufferTransparent.reserve(verticesTransparent.size());
    for (auto v : verticesTransparent) {
        vertexBufferTransparent.push_back(v);
    }
}

void Chunk::UploadData() {
    GLUtil::DestroyVertexArray(lvao);
    GLUtil::DestroyVertexBuffer(lvbo);
    GLUtil::DestroyVertexArray(vao);
    GLUtil::DestroyVertexBuffer(ssbo);
    GLUtil::DestroyVertexArray(tvao);
    GLUtil::DestroyVertexBuffer(tssbo);

    // LOD Chunk
    glGenVertexArrays(1, &lvao);
    glBindVertexArray(lvao);

    float h = maxHeight;
    float w = MAX_CHUNK_WIDTH * 0.5f;
    // LOD Chunk is always the color of the grass block
    float vertices[] = {
         // Front
        -w, -h,  w,  0, 0, 1,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w, -h,  w,  0, 0, 1,  1.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w,  h,  w,  0, 0, 1,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w, -h,  w,  0, 0, 1,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w,  h,  w,  0, 0, 1,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w,  h,  w,  0, 0, 1,  0.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,

         // Back
         w, -h, -w,  0, 0,-1,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w, -h, -w,  0, 0,-1,  1.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w,  h, -w,  0, 0,-1,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w, -h, -w,  0, 0,-1,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w,  h, -w,  0, 0,-1,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w,  h, -w,  0, 0,-1,  0.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,

         // Top
        -w,  h,  w,  0, 1, 0,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w,  h,  w,  0, 1, 0,  1.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w,  h, -w,  0, 1, 0,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w,  h,  w,  0, 1, 0,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w,  h, -w,  0, 1, 0,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w,  h, -w,  0, 1, 0,  0.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         
         // Bottom
        -w, -h, -w,  0,-1, 0,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w, -h, -w,  0,-1, 0,  1.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w, -h,  w,  0,-1, 0,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w, -h, -w,  0,-1, 0,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w, -h,  w,  0,-1, 0,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w, -h,  w,  0,-1, 0,  0.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,

         // Right
         w, -h,  w,  1, 0, 0,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w, -h, -w,  1, 0, 0,  1.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w,  h, -w,  1, 0, 0,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w, -h,  w,  1, 0, 0,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w,  h, -w,  1, 0, 0,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
         w,  h,  w,  1, 0, 0,  0.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,

         // Left
        -w, -h, -w, -1, 0, 0,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w, -h,  w, -1, 0, 0,  1.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w,  h,  w, -1, 0, 0,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w, -h, -w, -1, 0, 0,  0.0f, 0.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w,  h,  w, -1, 0, 0,  1.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
        -w,  h, -w, -1, 0, 0,  0.0f, 1.0f, (float)(uint8_t)BlockTexture::Grass_Top,
    };

    glGenBuffers(1, &lvbo);
    glBindBuffer(GL_ARRAY_BUFFER, lvbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, NULL);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (void *)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    // Vertex pulling (normal)
    glCreateBuffers(1, &ssbo);
    glCreateVertexArrays(1, &vao);
    glNamedBufferData(ssbo, vertexCount * sizeof(int) * 2, vertexBuffer.data(), GL_DYNAMIC_COPY);
    vertexBuffer.clear();
    std::vector<int>().swap(vertexBuffer);

    // Vertex pulling (transparent)
    glCreateBuffers(1, &tssbo);
    glCreateVertexArrays(1, &tvao);
    glNamedBufferData(tssbo, vertexCountTransparent * sizeof(int) * 2, vertexBufferTransparent.data(), GL_DYNAMIC_COPY);
    vertexBufferTransparent.clear();
    std::vector<int>().swap(vertexBufferTransparent);
}

void Chunk::Render(const Shader &shader, const glm::vec3 &playerPos, float distForLOD) {
    if (vertexCount == 0) return;

    glm::mat4 transform(1);
    transform = glm::translate(transform, glm::vec3(position.x * MAX_CHUNK_WIDTH, 0, position.y * MAX_CHUNK_WIDTH));
    shader.SetMatrix4("transform", transform);

    glm::vec3 chunkPos = glm::vec3((position.x * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f),
                                    MAX_CHUNK_HEIGHT * 0.5f,
                                   (position.y * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f)
                                  );
    if (glm::distance(chunkPos, playerPos) <= distForLOD) {
        glBindVertexArray(vao);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount * 6);
    } else {
        /*
        shader.SetInt("lod", 1);
        glBindVertexArray(lvao);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        shader.SetInt("lod", 0);
        */
    }
    glBindVertexArray(0);

}

void Chunk::RenderTransparent(const Shader &shader, const glm::vec3 &playerPos, float distForLOD) {
    if (vertexCountTransparent == 0) return;

    glm::mat4 transform(1);
    transform = glm::translate(transform, glm::vec3(position.x * MAX_CHUNK_WIDTH, 0, position.y * MAX_CHUNK_WIDTH));
    shader.SetMatrix4("transform", transform);

    glm::vec3 chunkPos = glm::vec3((position.x * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f),
                                    MAX_CHUNK_HEIGHT * 0.5f,
                                   (position.y * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f)
                                  );
    if (glm::distance(glm::vec2(chunkPos.x, chunkPos.z), glm::vec2(playerPos.x, playerPos.z)) <= distForLOD) {
        glBindVertexArray(tvao);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, tssbo);
        glDrawArrays(GL_TRIANGLES, 0, vertexCountTransparent * 6);
    }     

    glBindVertexArray(0);
}
