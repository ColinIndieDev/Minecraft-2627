#pragma once

#include <pthread.h>
#include <queue>
#include <unordered_map>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <Performance/Frustum.hpp>
#include <Environment/Cubemap.hpp>
#include <Core/Camera3D.hpp>
#include <Objects/Model.hpp>

#include "Chunk.hpp"

#define NUM_WORKER_THREADS 12

#define DEFAULT_VIEW_DISTANCE 32

#define ZNEAR 0.01f
#define ZFAR 1'000.0f

namespace Game {
    inline int lastViewDistance = DEFAULT_VIEW_DISTANCE;
    inline int viewDistance = DEFAULT_VIEW_DISTANCE;

    inline glm::mat4 perspective;
    inline Camera3D camera(glm::vec3(30, 90, 30), 45.0f, 0.1f);

    inline auto cmpNearestChunk = [](const auto& a, const auto& b) {
        glm::vec3 chunkPosA = glm::vec3((a->position.x * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f),
                MAX_CHUNK_HEIGHT * 0.5f,
                (a->position.y * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f)
                );
        glm::vec3 chunkPosB = glm::vec3((b->position.x * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f),
                MAX_CHUNK_HEIGHT * 0.5f,
                (b->position.y * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f)
                );
        int da = (chunkPosA.x - camera.pos.x) * (chunkPosA.x - camera.pos.x)
            + (chunkPosA.z - camera.pos.z) * (chunkPosA.z - camera.pos.z);
        int db = (chunkPosB.x - camera.pos.x) * (chunkPosB.x - camera.pos.x)
            + (chunkPosB.z - camera.pos.z) * (chunkPosB.z - camera.pos.z);
        return da > db;
    };

    inline bool workerRunning = true;
    inline pthread_mutex_t workerMutex = PTHREAD_MUTEX_INITIALIZER;
    inline pthread_mutex_t chunksMutex = PTHREAD_MUTEX_INITIALIZER;
    inline pthread_cond_t workerCond = PTHREAD_COND_INITIALIZER;

    inline pthread_t workerThreads[NUM_WORKER_THREADS];

    inline std::unordered_map<glm::ivec2, Chunk> chunks;

    inline std::priority_queue<Chunk *, std::vector<Chunk *>, decltype(cmpNearestChunk)> chunksToBuild(cmpNearestChunk);
    inline std::queue<Chunk *> chunksToLoad;
    inline glm::ivec2 lastPlayerChunkPos = glm::ivec2(0);

    inline bool mouseLocked = true;

    inline Cubemap map;
    inline Frustum frustum;

    inline Model model;

    inline Shader vertexPullShader;
    inline Shader cubemapShader;
    inline Shader sunShader;
    inline Shader depthMapShader;
    inline Shader hdrShader;
    inline Shader blurShader;
    inline Shader occlusionShader;
    inline Shader occlusionSunShader;
    inline Shader modelShader;

    inline uint32_t textureArray = 0;
    inline uint32_t textureArrayNormals = 0;

    inline std::vector<Chunk *> chunksSorted;

    void Init();
    void Update();
    void Render();
    void Destroy();

    void *ChunkGenWorker(void *args);
    void CreateChunkGenWorkers();
    void DestroyChunkGenWorkers();

    void UploadChunkDataToGPU();
    void AddChunkToBuildQueue(Chunk *chunk);

    void GenChunksAroundPlayer();
    void GetVisibleChunks(std::vector<Chunk *> &chunksSorted, Frustum &frustum);

    glm::vec2 GetPlayerChunkPos();
    void HandleInput();
}
