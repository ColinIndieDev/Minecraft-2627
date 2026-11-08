#include "Chunk.hpp"
#include "BlockType.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>
#include <glad/glad.h>
#include <FastNoiseLite.h>

#include "GLUtil.hpp"
#include "BlockTexture.hpp"

void AddVertex(std::vector<float> &list, float x, float y, float z, float nx, float ny, float nz, float u, float v, BlockTexture blockTex) {
    list.push_back(x);
    list.push_back(y);
    list.push_back(z);
    list.push_back(nx);
    list.push_back(ny);
    list.push_back(nz);
    list.push_back(u);
    list.push_back(v);
    // This should work (hopefully)
    list.push_back((float)(uint8_t)blockTex);
}

#pragma region Faces Helper Block
void AddFrontFace(std::vector<float> &v, float x, float y, float z, BlockTexture blockTex) {
    AddVertex(v, x,   y,   z+1, 0, 0, 1, 0, 1, blockTex);
    AddVertex(v, x+1, y,   z+1, 0, 0, 1, 1, 1, blockTex);
    AddVertex(v, x+1, y+1, z+1, 0, 0, 1, 1, 0, blockTex);

    AddVertex(v, x,   y,   z+1, 0, 0, 1, 0, 1, blockTex);
    AddVertex(v, x+1, y+1, z+1, 0, 0, 1, 1, 0, blockTex);
    AddVertex(v, x,   y+1, z+1, 0, 0, 1, 0, 0, blockTex);
}

void AddBackFace(std::vector<float> &v, float x, float y, float z, BlockTexture blockTex) {
    AddVertex(v, x+1, y,   z,   0, 0, -1, 0, 1, blockTex);
    AddVertex(v, x,   y,   z,   0, 0, -1, 1, 1, blockTex);
    AddVertex(v, x,   y+1, z,   0, 0, -1, 1, 0, blockTex);

    AddVertex(v, x+1, y,   z,   0, 0, -1, 0, 1, blockTex);
    AddVertex(v, x,   y+1, z,   0, 0, -1, 1, 0, blockTex);
    AddVertex(v, x+1, y+1, z,   0, 0, -1, 0, 0, blockTex);
}

void AddTopFace(std::vector<float> &v, float x, float y, float z, BlockTexture blockTex) {
    AddVertex(v, x,   y+1, z,   0, 1, 0, 0, 0, blockTex);
    AddVertex(v, x,   y+1, z+1, 0, 1, 0, 0, 1, blockTex);
    AddVertex(v, x+1, y+1, z+1, 0, 1, 0, 1, 1, blockTex);

    AddVertex(v, x,   y+1, z,   0, 1, 0, 0, 0, blockTex);
    AddVertex(v, x+1, y+1, z+1, 0, 1, 0, 1, 1, blockTex);
    AddVertex(v, x+1, y+1, z,   0, 1, 0, 1, 0, blockTex);
}

void AddBottomFace(std::vector<float> &v, float x, float y, float z, BlockTexture blockTex) {
    AddVertex(v, x,   y, z,     0, -1, 0, 0, 0, blockTex);
    AddVertex(v, x+1, y, z+1,   0, -1, 0, 1, 1, blockTex);
    AddVertex(v, x,   y, z+1,   0, -1, 0, 0, 1, blockTex);

    AddVertex(v, x,   y, z,     0, -1, 0, 0, 0, blockTex);
    AddVertex(v, x+1, y, z,     0, -1, 0, 1, 0, blockTex);
    AddVertex(v, x+1, y, z+1,   0, -1, 0, 1, 1, blockTex);
}

void AddRightFace(std::vector<float> &v, float x, float y, float z, BlockTexture blockTex) {
    AddVertex(v, x+1, y,   z+1, 1, 0, 0, 0, 1, blockTex);
    AddVertex(v, x+1, y,   z,   1, 0, 0, 1, 1, blockTex);
    AddVertex(v, x+1, y+1, z,   1, 0, 0, 1, 0, blockTex);

    AddVertex(v, x+1, y,   z+1, 1, 0, 0, 0, 1, blockTex);
    AddVertex(v, x+1, y+1, z,   1, 0, 0, 1, 0, blockTex);
    AddVertex(v, x+1, y+1, z+1, 1, 0, 0, 0, 0, blockTex);
}

void AddLeftFace(std::vector<float> &v, float x, float y, float z, BlockTexture blockTex) {
    AddVertex(v, x,   y,   z,   -1, 0, 0, 0, 1, blockTex);
    AddVertex(v, x,   y,   z+1, -1, 0, 0, 1, 1, blockTex);
    AddVertex(v, x,   y+1, z+1, -1, 0, 0, 1, 0, blockTex);

    AddVertex(v, x,   y,   z,   -1, 0, 0, 0, 1, blockTex);
    AddVertex(v, x,   y+1, z+1, -1, 0, 0, 1, 0, blockTex);
    AddVertex(v, x,   y+1, z,   -1, 0, 0, 0, 0, blockTex);
}
#pragma endregion

/*
#pragma region Faces Helper LOD Chunk
void AddFrontFaceLOD(std::vector<float> &vl, int maxHeight) {
    float h = maxHeight;
    float w = MAX_CHUNK_WIDTH * 0.5f;
    AddVertex(vl, -w, -h,  w,  0, 0, 1,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w, -h,  w,  0, 0, 1,  1.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w,  h,  w,  0, 0, 1,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w, -h,  w,  0, 0, 1,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w,  h,  w,  0, 0, 1,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w,  h,  w,  0, 0, 1,  0.0f, 1.0f, BlockTexture::Grass_Top);
}

void AddBackFaceLOD(std::vector<float> &vl, int maxHeight) {
    float h = maxHeight;
    float w = MAX_CHUNK_WIDTH * 0.5f;
    AddVertex(vl,  w, -h, -w,  0, 0,-1,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w, -h, -w,  0, 0,-1,  1.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w,  h, -w,  0, 0,-1,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w, -h, -w,  0, 0,-1,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w,  h, -w,  0, 0,-1,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w,  h, -w,  0, 0,-1,  0.0f, 1.0f, BlockTexture::Grass_Top);
}

void AddTopFaceLOD(std::vector<float> &vl, int maxHeight) {
    float h = maxHeight;
    float w = MAX_CHUNK_WIDTH * 0.5f;
    AddVertex(vl, -w,  h,  w,  0, 1, 0,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w,  h,  w,  0, 1, 0,  1.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w,  h, -w,  0, 1, 0,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w,  h,  w,  0, 1, 0,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w,  h, -w,  0, 1, 0,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w,  h, -w,  0, 1, 0,  0.0f, 1.0f, BlockTexture::Grass_Top);
}

void AddBottomFaceLOD(std::vector<float> &vl, int maxHeight) {
    float h = maxHeight;
    float w = MAX_CHUNK_WIDTH * 0.5f;
    AddVertex(vl, -w, -h, -w,  0,-1, 0,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w, -h, -w,  0,-1, 0,  1.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w, -h,  w,  0,-1, 0,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w, -h, -w,  0,-1, 0,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w, -h,  w,  0,-1, 0,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w, -h,  w,  0,-1, 0,  0.0f, 1.0f, BlockTexture::Grass_Top);
}

void AddRightFaceLOD(std::vector<float> &vl, int maxHeight) {
    float h = maxHeight;
    float w = MAX_CHUNK_WIDTH * 0.5f;
    AddVertex(vl,  w, -h,  w,  1, 0, 0,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w, -h, -w,  1, 0, 0,  1.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w,  h, -w,  1, 0, 0,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w, -h,  w,  1, 0, 0,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w,  h, -w,  1, 0, 0,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl,  w,  h,  w,  1, 0, 0,  0.0f, 1.0f, BlockTexture::Grass_Top);
}

void AddLeftFaceLOD(std::vector<float> &vl, int maxHeight) {
    float h = maxHeight;
    float w = MAX_CHUNK_WIDTH * 0.5f;
    AddVertex(vl, -w, -h, -w, -1, 0, 0,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w, -h,  w, -1, 0, 0,  1.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w,  h,  w, -1, 0, 0,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w, -h, -w, -1, 0, 0,  0.0f, 0.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w,  h,  w, -1, 0, 0,  1.0f, 1.0f, BlockTexture::Grass_Top);
    AddVertex(vl, -w,  h, -w, -1, 0, 0,  0.0f, 1.0f, BlockTexture::Grass_Top);
}
#pragma endregion
*/

void Chunk::GenerateTree(int localX, int baseY, int localZ, int height) {
    // Standard-Eichenbaum-Dimensionen
    int trunkHeight = height; // z.B. 4 bis 6 Blöcke hoch
    int leavesStart = baseY + trunkHeight - 2;
    int leavesTop = baseY + trunkHeight + 2;

    // 1. Stamm generieren
    for (int y = baseY; y < baseY + trunkHeight; ++y) {
        if (y < MAX_CHUNK_HEIGHT) {
            blocks[localX][y][localZ] = (uint8_t)BlockType::Oak_Log;
        }
    }

    // 2. Blätter-Krone generieren (Schichten-basiert wie in Minecraft)
    for (int y = leavesStart; y <= leavesTop; ++y) {
        if (y < 0 || y >= MAX_CHUNK_HEIGHT) continue;

        int radius = 2;
        // Ganz oben wird die Krone etwas schmaler
        if (y == leavesTop || y == leavesStart) {
            radius = 1;
        }

        for (int x = -radius; x <= radius; ++x) {
            for (int z = -radius; z <= radius; ++z) {
                // Ecken der Blätter abrunden (optional, sieht natürlicher aus)
                if (radius == 2 && abs(x) == 2 && abs(z) == 2 && (y == leavesTop - 1 || y == leavesStart + 1)) {
                    continue;
                }

                int targetX = localX + x;
                int targetZ = localZ + z;

                // Prüfen, ob sich der Block innerhalb des Chunks befindet
                if (targetX >= 0 && targetX < MAX_CHUNK_WIDTH &&
                    targetZ >= 0 && targetZ < MAX_CHUNK_WIDTH) {

                    // Nur Blätter setzen, wenn dort nicht schon ein Stamm ist
                    if (blocks[targetX][y][targetZ] == (uint8_t)BlockType::Air ||
                        blocks[targetX][y][targetZ] == 0) {
                        blocks[targetX][y][targetZ] = (uint8_t)BlockType::Oak_Leaves;
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
            float n = noise.GetNoise(nx, nz);
            if (n > 0) {
                n = powf(n, 2.5f);
            }
            height += (n + 1) * 0.5f * (MAX_PLAINS_HEIGHT - MIN_PLAINS_HEIGHT);

            if ((int)height > maxHeight) {
                maxHeight = (int)height;
            }
            for (int y = 0; y < (int)height; ++y) {
                if (y >= height - 2) {
                    SetBlock(glm::ivec3(x, y, z), (uint8_t)BlockType::Grass_Block);
                } else if (y > height - 6) {
                    SetBlock(glm::ivec3(x, y, z), (uint8_t)BlockType::Dirt);
                } else {
                    SetBlock(glm::ivec3(x, y, z), (uint8_t)BlockType::Stone);
                }
            }
        }
    }

    // Example: Create a tree at 8, 8 with height of 5
    int treeX = 8;
    int treeZ = 8;
    int surfaceY = 0;
    for (int y = MAX_CHUNK_HEIGHT - 1; y >= 0; --y) {
        if (blocks[treeX][y][treeZ] != 0) {
            surfaceY = y + 1;
            break;
        }
    }
    GenerateTree(treeX, surfaceY, treeZ, 5);
}

Chunk::~Chunk() {
    GLUtil::DestroyVertexArray(vao);
    GLUtil::DestroyVertexBuffer(vbo);
    GLUtil::DestroyVertexArray(lvao);
    GLUtil::DestroyVertexBuffer(lvbo);
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
    std::vector<float> vertices;
    vertices.reserve(MAX_CHUNK_WIDTH * MAX_CHUNK_WIDTH * maxHeight * 9);
    // std::vector<float> verticesLOD;
    // verticesLOD.reserve(MAX_CHUNK_WIDTH * MAX_CHUNK_WIDTH * maxHeight * 9);
    
    int worldStartX = position.x * MAX_CHUNK_WIDTH;
    int worldStartZ = position.y * MAX_CHUNK_WIDTH;

    for (int x = 0; x < MAX_CHUNK_WIDTH; x++) {
        for (int y = 0; y < MAX_CHUNK_HEIGHT; y++) {
            for (int z = 0; z < MAX_CHUNK_WIDTH; z++) {
                if (blocks[x][y][z] == 0) continue;

                int wx = worldStartX + x;
                int wz = worldStartZ + z;

                auto getBlockGlobal = [&](int worldX, int y, int z) -> int {
                    if (y < 0 || y >= MAX_CHUNK_HEIGHT) return 0;

                    int chunkX = std::floor(worldX / MAX_CHUNK_WIDTH);
                    int chunkZ = std::floor(z / MAX_CHUNK_WIDTH);
                    if (chunks.find(glm::ivec2(chunkX, chunkZ)) != chunks.end()) {
                        std::unique_ptr<Chunk> &c = chunks.at(glm::ivec2(chunkX, chunkZ));
                        int localX = std::floor(worldX % MAX_CHUNK_WIDTH);
                        int localZ = std::floor(z % MAX_CHUNK_WIDTH);

                        return c->GetBlock(glm::ivec3(localX, y, localZ));
                    }
                    return 0;
                };

                if (blocks[x][y][z] == (uint8_t)BlockType::Air) continue;
                if (getBlockGlobal(wx, y + 1, wz) == 0) AddTopFace   (vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].top);
                if (getBlockGlobal(wx, y - 1, wz) == 0) AddBottomFace(vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].bottom);
                if (getBlockGlobal(wx, y, wz + 1) == 0) AddFrontFace (vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].front);
                if (getBlockGlobal(wx, y, wz - 1) == 0) AddBackFace  (vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].back);
                if (getBlockGlobal(wx + 1, y, wz) == 0) AddRightFace (vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].right);
                if (getBlockGlobal(wx - 1, y, wz) == 0) AddLeftFace  (vertices, x, y, z, blockTypeToBlockFace[blocks[x][y][z]].left);
            }
        }
    }

    vertexCount = vertices.size() / 9;
    if (vertexCount == 0) return;

    vertexBuffer.reserve(vertices.size());
    for (float v : vertices) {
        vertexBuffer.push_back(v);
    }

    // vertexCountLOD = verticesLOD.size() / 9;
    // if (vertexCountLOD == 0) return;

    // vertexBufferLOD.reserve(verticesLOD.size());
    // for (float v : verticesLOD) {
    //     vertexBufferLOD.push_back(v);
    // }
}

void Chunk::UploadData() {
    GLUtil::DestroyVertexArray(vao);
    GLUtil::DestroyVertexBuffer(vbo);
    GLUtil::DestroyVertexArray(lvao);
    GLUtil::DestroyVertexBuffer(lvbo);

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(1, &vbo);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexBuffer.size(), vertexBuffer.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, NULL);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, sizeof(float) * 9, (void *)(8 * sizeof(float)));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    vertexBuffer.clear();
    std::vector<float>().swap(vertexBuffer);

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
    // glBufferData(GL_ARRAY_BUFFER, sizeof(float) * vertexBufferLOD.size(), vertexBufferLOD.data(), GL_STATIC_DRAW);
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

    // vertexBufferLOD.clear();
    // std::vector<float>().swap(vertexBufferLOD);
}

void Chunk::Render(const Shader &shader, const glm::vec3 &playerPos, float distForLOD) {
    if (vertexCount == 0) return;

    glm::mat4 transform(1);
    transform = glm::translate(transform, glm::vec3(position.x * MAX_CHUNK_WIDTH, 0, position.y * MAX_CHUNK_WIDTH));
    shader.SetMatrix4("transform", transform);

    glm::vec3 chunkPos = glm::vec3((position.x * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f),
                                    maxHeight * 0.5f,
                                   (position.y * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f)
                                  );
    if (glm::distance(chunkPos, playerPos) <= distForLOD) {
        glBindVertexArray(vao);
        glDrawArrays(GL_TRIANGLES, 0, vertexCount);
    } else {
        shader.SetInt("lod", 1);
        glBindVertexArray(lvao);
        // glDrawArrays(GL_TRIANGLES, 0, vertexCountLOD);
        glDrawArrays(GL_TRIANGLES, 0, 36);
        shader.SetInt("lod", 0);
    }
    glBindVertexArray(0);

}
