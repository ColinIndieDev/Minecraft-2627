#pragma once

#include <cstdint>
#include <vector>
#include <unordered_map>
#include <memory>

#include <glm/glm.hpp>

#include "Shader.hpp"

enum class ChunkGenState : uint8_t {
    None = 0,
    Dirty,
    Done
};

#define MAX_CHUNK_WIDTH 16
#define MAX_CHUNK_HEIGHT 256

#define MIN_PLAINS_HEIGHT 50
#define MAX_PLAINS_HEIGHT 200

struct Chunk {
    ChunkGenState genState = ChunkGenState::None;     
    std::vector<int> vertexBuffer;
    uint8_t blocks[MAX_CHUNK_WIDTH][MAX_CHUNK_HEIGHT][MAX_CHUNK_WIDTH] = {0};
    uint32_t lvao = 0, lvbo = 0;
    uint32_t vao = 0, ssbo = 0;
    size_t vertexCount = 0;
    int maxHeight = 0;
    glm::ivec2 position = glm::ivec2(0);

    void GenerateTree(int localX, int baseY, int localZ, int height);
    
    Chunk(const glm::ivec2 &position);
    ~Chunk();
    void SetBlock(const glm::ivec3 &pos, uint8_t type);
    uint8_t GetBlock(const glm::ivec3 &pos);
    void RebuildMesh(std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>> &chunks);
    void UploadData();
    void Render(const Shader &shader, const glm::vec3 &playerPos, float distForLOD);
};
