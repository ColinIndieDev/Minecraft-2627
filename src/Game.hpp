#pragma once

#include <pthread.h>
#include <queue>
#include <algorithm>
#include <unordered_map>

#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "Chunk.hpp"
#include "Frustum.hpp"
#include "Window.hpp"
#include "Camera3D.hpp"

#define NUM_WORKER_THREADS 12

#define DEFAULT_VIEW_DISTANCE 32

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

    inline void *ChunkGenWorker(void *args) {
        while (workerRunning) {
            pthread_mutex_lock(&workerMutex);
            while (chunksToBuild.empty() && workerRunning) {
                pthread_cond_wait(&workerCond, &workerMutex);
            }
            if (!workerRunning) {
                pthread_mutex_unlock(&workerMutex);
                break;
            }
            Chunk *chunk = chunksToBuild.top();
            chunksToBuild.pop();
            pthread_mutex_unlock(&workerMutex);

            chunk->genState = ChunkGenState::MeshBuilding;

            pthread_mutex_lock(&chunksMutex);

            chunk->RebuildMesh(chunks);

            pthread_mutex_unlock(&chunksMutex);

            pthread_mutex_lock(&workerMutex);

            if (chunk->needsRebuild) {
                chunk->needsRebuild = false;
                chunk->genState = ChunkGenState::MeshQueued;
                chunksToBuild.push(chunk);
                pthread_cond_signal(&workerCond);
            }
            else {
                chunk->genState = ChunkGenState::MeshReadyForUpload;
                chunksToLoad.push(chunk);
            }

            pthread_mutex_unlock(&workerMutex);
        }
        return NULL;
    }

    inline void CreateChunkGenWorkers() {
        for (int i = 0; i < NUM_WORKER_THREADS; ++i) {
            pthread_create(&workerThreads[i], NULL, ChunkGenWorker, NULL);
        }
    }

    inline void UploadChunkDataToGPU() {
        pthread_mutex_lock(&workerMutex);
        int uploadsInFrame = 0;
        while (!chunksToLoad.empty() && uploadsInFrame++ < 8) {
            Chunk *chunk = chunksToLoad.front();
            chunksToLoad.pop();

            if (chunk->needsRebuild) {
                chunk->needsRebuild = false;
                chunk->genState = ChunkGenState::MeshQueued;
                chunksToBuild.push(chunk);
                pthread_cond_signal(&workerCond);
                continue;
            }

            chunk->UploadData();
            chunk->genState = ChunkGenState::MeshReady;
        }
        pthread_mutex_unlock(&workerMutex);
    }

    inline glm::vec2 GetPlayerChunkPos() {
        return glm::ivec2(
                (int)std::floor(camera.pos.x / MAX_CHUNK_WIDTH),
                (int)std::floor(camera.pos.z / MAX_CHUNK_WIDTH)
                );
    }

    inline void AddChunkToBuildQueue(Chunk *chunk) {
        if (chunk->genState == ChunkGenState::MeshBuilding || 
                chunk->genState == ChunkGenState::MeshReadyForUpload) {
            chunk->needsRebuild = true;
            return;
        }

        if (chunk->genState == ChunkGenState::MeshQueued) {
            return;
        }

        chunk->genState = ChunkGenState::MeshQueued;
        chunksToBuild.push(chunk);
    }

    inline void GenChunksAroundPlayer() {
        glm::ivec2 playerChunkPos = GetPlayerChunkPos();
        if (playerChunkPos != lastPlayerChunkPos || viewDistance != lastViewDistance) {
            pthread_mutex_lock(&workerMutex);
            pthread_mutex_lock(&chunksMutex);
            for (int x = playerChunkPos.x - viewDistance - 5; x < playerChunkPos.x + viewDistance + 5; ++x) {
                for (int z = playerChunkPos.y - viewDistance - 5; z < playerChunkPos.y + viewDistance + 5; ++z) {
                    glm::ivec2 newChunkPos(x, z);
                    if (chunks.find(newChunkPos) == chunks.end()) {
                        chunks.emplace(newChunkPos, Chunk(newChunkPos));

                        AddChunkToBuildQueue(&chunks.at(newChunkPos));

                        const glm::ivec2 directions[] = {
                            { 1,  0},
                            {-1,  0},
                            { 0,  1},
                            { 0, -1}
                        };

                        for (const auto& dir : directions) {
                            glm::ivec2 neighborPos = newChunkPos + dir;

                            auto neighbor = chunks.find(neighborPos);
                            if (neighbor != chunks.end()) {
                                AddChunkToBuildQueue(&neighbor->second);
                            }
                        }
                    }
                }
            }
            pthread_cond_broadcast(&workerCond);
            pthread_mutex_unlock(&chunksMutex);
            pthread_mutex_unlock(&workerMutex);

            lastPlayerChunkPos = playerChunkPos;
            lastViewDistance = viewDistance;
        }
    }

    inline void HandleInput(Window &window) {
        float speed = 100.0f * window.GetDeltaTime();
        if (window.GetKeyPressed(GLFW_KEY_W)) {
            camera.pos += speed * camera.front;
        }
        if (window.GetKeyPressed(GLFW_KEY_S)) {
            camera.pos -= speed * camera.front;
        }
        if (window.GetKeyPressed(GLFW_KEY_A)) {
            camera.pos -= glm::normalize(glm::cross(camera.front, camera.up)) * speed;
        }
        if (window.GetKeyPressed(GLFW_KEY_D)) {
            camera.pos += glm::normalize(glm::cross(camera.front, camera.up)) * speed;
        }
        if (window.GetKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
            camera.pos.y -= speed;
        }
        if (window.GetKeyPressed(GLFW_KEY_SPACE)) {
            camera.pos.y += speed;
        }
    }

    inline void GetVisibleChunks(std::vector<Chunk *> &chunksSorted, Frustum &frustum) {
        chunksSorted.reserve(chunks.size());
        for (auto &[pos, c] : chunks) {
            chunksSorted.push_back(&c);
        }
        std::sort(chunksSorted.begin(), chunksSorted.end(), [&](auto &a, auto &b) {
            return glm::distance(camera.pos, glm::vec3(a->position.x, 0, a->position.y)) > glm::distance(camera.pos, glm::vec3(b->position.x, 0, b->position.y));
        });
        chunksSorted.erase(
            std::remove_if(chunksSorted.begin(), chunksSorted.end(), [&](auto &chunk) {
                glm::vec3 chunkPos = glm::vec3(
                                               (chunk->position.x * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f),
                                                chunk->maxHeight * 0.5f,
                                               (chunk->position.y * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f)
                                              );
                glm::vec3 chunkSize = glm::vec3(MAX_CHUNK_WIDTH, chunk->maxHeight, MAX_CHUNK_WIDTH);
                bool inFrustum = !frustum.IsCubeVisible(chunkPos, chunkSize);
                bool inRenderDistance = glm::distance(glm::vec2(chunkPos.x, chunkPos.z), glm::vec2(camera.pos.x, camera.pos.z)) > viewDistance * MAX_CHUNK_WIDTH;
                return inFrustum || inRenderDistance || chunk->genState != ChunkGenState::MeshReady;
            }),
            chunksSorted.end()
        );
    }
}
