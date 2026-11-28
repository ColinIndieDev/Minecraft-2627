#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>

#include <glm/glm.hpp>

#include <Core/Shader.hpp>

enum class ChunkGenState : uint8_t {
    TerrainGenerated = 0,
    MeshQueued,
    MeshBuilding,
    MeshReadyForUpload,
    MeshReady,
    MeshDirty
};

#define MAX_CHUNK_WIDTH 16
#define MAX_CHUNK_HEIGHT 256

#define MIN_PLAINS_HEIGHT 50 /* Default: 50 */
#define MAX_PLAINS_HEIGHT 200 /* Default: 200 */
#define SEA_LEVEL 90

struct Chunk {
    ChunkGenState genState = ChunkGenState::TerrainGenerated;     
    std::vector<int> vertexBuffer;
    std::vector<int> vertexBufferTransparent;
    uint8_t blocks[MAX_CHUNK_WIDTH][MAX_CHUNK_HEIGHT][MAX_CHUNK_WIDTH] = {0};
    uint32_t vao = 0, ssbo = 0;
    uint32_t tvao = 0, tssbo = 0;
    size_t vertexCount = 0;
    size_t vertexCountTransparent = 0;
    int maxHeight = 0;
    glm::ivec2 position = glm::ivec2(0);
    bool needsRebuild = false;

    void GenerateTree(int localX, int baseY, int localZ, int height, int &maxHeight);
    
    Chunk(const glm::ivec2 &position);
    ~Chunk();
    void SetBlock(const glm::ivec3 &pos, uint8_t type);
    uint8_t GetBlock(const glm::ivec3 &pos);
    void RebuildMesh(std::unordered_map<glm::ivec2, Chunk> &chunks);
    void UploadData();
    void Render(const Shader &shader, const glm::vec3 &playerPos);
    void RenderTransparent(const Shader &shader, const glm::vec3 &playerPos);
};
