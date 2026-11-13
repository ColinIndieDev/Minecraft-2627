#include <unordered_map>
#include <string>
#include <memory>
#include <queue>
#include <algorithm>
#include <unistd.h>
#include <pthread.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include "GLUtil.hpp"
#include "Shader.hpp"
#include "Window.hpp"
#include "Camera3D.hpp"
#include "Cubemap.hpp"
#include "Chunk.hpp"
#include "Frustum.hpp"
#include "BlockTexture.hpp"

#define ZNEAR 0.01f
#define ZFAR 100'000.0f

#define THREADS 12

#define DISTANCE_FOR_LOD_CHUNK 1'000'000 /* 1000 = almost 64 chunks */
#define VIEW_DISTANCE 16

glm::mat4 perspective;
Camera3D camera(glm::vec3(30, 90, 30), 45.0f, 0.1f);

#pragma region GLFW Callbacks
void FramebufferSizeCallback(GLFWwindow *window, int width, int height) {
    if (height == 0 || width == 0) {
        return;
    }
    perspective = glm::perspective(glm::radians(camera.fov), (float)width / height, ZNEAR, ZFAR);
    glViewport(0, 0, width, height);
}

void CursorPosCallback(GLFWwindow *window, double xPosIn, double yPosIn) {
    float xPos = (float)xPosIn;
    float yPos = (float)yPosIn;

    if (camera.firstMouse) {
        camera.lastX = xPos;
        camera.lastY = yPos;
        camera.firstMouse = false;
    }

    float xOff = (xPos - camera.lastX) * camera.sensitivity;
    float yOff = (camera.lastY - yPos) * camera.sensitivity;

    camera.lastX = xPos;
    camera.lastY = yPos;

    camera.yaw += xOff;
    camera.pitch += yOff;

    camera.pitch = fmin(camera.pitch, 89.0f);
    camera.pitch = fmax(camera.pitch, -89.0f);

    camera.front.x = cosf(glm::radians(camera.yaw)) * cosf(glm::radians(camera.pitch));
    camera.front.y = sinf(glm::radians(camera.pitch));
    camera.front.z = sinf(glm::radians(camera.yaw)) * cosf(glm::radians(camera.pitch));
}
#pragma endregion

auto cmpNearestChunk = [](const auto& a, const auto& b) {
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

std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>> chunks;

pthread_t threads[THREADS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
std::priority_queue<Chunk *, std::vector<Chunk *>, decltype(cmpNearestChunk)> chunksToBuild(cmpNearestChunk);
std::queue<Chunk *> chunksToLoad;
bool running = true;
glm::ivec2 lastPlayerChunkPos = glm::ivec2(0);

void *worker_loop(void *args) {
    while (running) {
        pthread_mutex_lock(&mutex);
        while (chunksToBuild.empty() && running) {
            pthread_cond_wait(&cond, &mutex);
            // usleep(1000);
            // continue;
        }
        if (!running) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        Chunk *chunk = chunksToBuild.top();
        chunksToBuild.pop();
        pthread_mutex_unlock(&mutex);

        chunk->RebuildMesh(chunks);

        pthread_mutex_lock(&mutex);
        chunksToLoad.push(chunk);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

glm::vec2 GetPlayerChunkPos() {
    return glm::ivec2(camera.pos.x / MAX_CHUNK_WIDTH,
                      camera.pos.z / MAX_CHUNK_WIDTH);
}

int main(void) {
    srand(time(NULL));
    Window window = {
        .framebufferSizeCallback = FramebufferSizeCallback,
        .cursorPosCallback = CursorPosCallback
    };

    window.Init(800, 500, "Hello OpenGL!", true);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    Shader vertexPullShader("shader_resources/shaderPull.vert", "shader_resources/shader.frag");
    Shader cs("shader_resources/cubemapShader.vert", "shader_resources/cubemapShader.frag");
    perspective = glm::perspective(glm::radians(camera.fov), (float)window.screenWidth / window.screenHeight, ZNEAR, ZFAR);

    std::string paths[6] = {
        "sky/px.png",
        "sky/nx.png",
        "sky/py.png",
        "sky/ny.png",
        "sky/nz.png",
        "sky/pz.png",
    };
    Cubemap map(paths);

    Frustum frustum;

    uint32_t textureArray = GetTextureArray();

    for (int i = 0; i < THREADS; ++i) {
        pthread_create(&threads[i], NULL, worker_loop, NULL);
    }

    /*
    pthread_mutex_lock(&mutex);
    for (int x = -VIEW_DISTANCE; x < VIEW_DISTANCE; ++x) {
        for (int z = -VIEW_DISTANCE; z < VIEW_DISTANCE; ++z) {
            chunks.emplace(glm::ivec2(x, z), std::make_unique<Chunk>(glm::ivec2(x, z)));
            auto &chunk = chunks.at(glm::ivec2(x, z));
            chunksToBuild.push(chunk.get());
        }
    }
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);
    */

    while (!window.ShouldClose()) {
        pthread_mutex_lock(&mutex);
        int uploadsInFrame = 0;
        while (!chunksToLoad.empty() && uploadsInFrame++ < 8) {
            Chunk *chunk = chunksToLoad.front();
            chunk->UploadData();
            chunk->genState = ChunkGenState::Done;
            chunksToLoad.pop();
        }
        pthread_mutex_unlock(&mutex);

        glm::ivec2 playerChunkPos = GetPlayerChunkPos();
        if (playerChunkPos != lastPlayerChunkPos) {
            pthread_mutex_lock(&mutex);
            for (int x = playerChunkPos.x - VIEW_DISTANCE; x < playerChunkPos.x + VIEW_DISTANCE; ++x) {
                for (int z = playerChunkPos.y - VIEW_DISTANCE; z < playerChunkPos.y + VIEW_DISTANCE; ++z) {
                    glm::ivec2 newChunkPos(x, z);
                    if (chunks.find(newChunkPos) == chunks.end()) {
                        chunks.emplace(newChunkPos, std::make_unique<Chunk>(newChunkPos));
                        auto &chunk = chunks.at(newChunkPos);
                        chunksToBuild.push(chunk.get());
                    }
                }
            }
            pthread_cond_signal(&cond);
            pthread_mutex_unlock(&mutex);

            lastPlayerChunkPos = playerChunkPos;
        }

        window.Update();
        frustum.Update(perspective * camera.GetViewMatrix());
        window.Clear(glm::vec4(0, 255, 255, 255));

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

        vertexPullShader.Use();
        vertexPullShader.SetVector3("viewPos", camera.pos);
        vertexPullShader.SetVector3("directionalLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        vertexPullShader.SetVector3("directionalLight.ambient", glm::vec3(0.5f));
        vertexPullShader.SetVector3("directionalLight.diffuse", glm::vec3(0.9f));
        vertexPullShader.SetVector3("directionalLight.specular", glm::vec3(0.1f));
        vertexPullShader.SetMatrix4("projection", perspective);
        vertexPullShader.SetMatrix4("view", camera.GetViewMatrix());

        vertexPullShader.SetInt("tex", 0);
        vertexPullShader.SetInt("lod", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);

        std::vector<std::pair<const glm::ivec2, std::unique_ptr<Chunk>>*> chunksSorted;
        chunksSorted.reserve(chunks.size());
        for (auto &c : chunks) {
            chunksSorted.push_back(&c);
        }
        std::sort(chunksSorted.begin(), chunksSorted.end(), [&](auto &a, auto &b) {
            return glm::distance(camera.pos, glm::vec3(a->first.x, 0, a->first.y)) < glm::distance(camera.pos, glm::vec3(b->first.x, 0, b->first.y));
        });
        chunksSorted.erase(
            std::remove_if(chunksSorted.begin(), chunksSorted.end(), [&](auto &chunk) {
                return !frustum.IsCubeVisible(glm::vec3(
                                          (chunk->second->position.x * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f),
                                           chunk->second->maxHeight * 0.5f,
                                          (chunk->second->position.y * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f)
                                      ), glm::vec3(MAX_CHUNK_WIDTH, chunk->second->maxHeight, MAX_CHUNK_WIDTH)) || chunk->second->genState != ChunkGenState::Done;
            }),
            chunksSorted.end()
        );

        for (auto &c : chunksSorted) {
            std::unique_ptr<Chunk> &chunk = c->second;
            const glm::ivec2 &pos = c->first;
            chunk->Render(vertexPullShader, camera.pos, DISTANCE_FOR_LOD_CHUNK);
        }

        // For water (it also affects leaves but idfc)
        vertexPullShader.SetVector3("directionalLight.specular", glm::vec3(0.8f));
        for (auto &c : chunksSorted) {
            std::unique_ptr<Chunk> &chunk = c->second;
            const glm::ivec2 &pos = c->first;
            chunk->RenderTransparent(vertexPullShader, camera.pos, DISTANCE_FOR_LOD_CHUNK);
        }

        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        map.Draw(glm::vec3(0), glm::vec4(255), camera, cs, perspective);

        window.Swap();
        fprintf(stderr, "%d\n", window.GetFPS());
    }
    window.Close();
}
