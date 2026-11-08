#include <unordered_map>
#include <string>
#include <memory>
#include <queue>
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

#define DISTANCE_FOR_LOD_CHUNK 1000

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

std::unordered_map<glm::ivec2, std::unique_ptr<Chunk>> chunks;

pthread_t threads[THREADS];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
std::queue<Chunk *> chunksToBuild;
std::queue<Chunk *> chunksToLoad;
bool running = true;

void *worker_loop(void *args) {
    while (running) {
        pthread_mutex_lock(&mutex);
        if (chunksToBuild.empty() && running) {
            pthread_cond_wait(&cond, &mutex);
            // usleep(1000);
            // continue;
        }
        if (!running) {
            pthread_mutex_unlock(&mutex);
            break;
        }
        Chunk *chunk = chunksToBuild.front();
        chunksToBuild.pop();
        pthread_mutex_unlock(&mutex);

        chunk->RebuildMesh(chunks);

        pthread_mutex_lock(&mutex);
        chunksToLoad.push(chunk);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

int main(void) {
    Window window = {
        .framebufferSizeCallback = FramebufferSizeCallback,
        .cursorPosCallback = CursorPosCallback
    };

    window.Init(800, 500, "Hello OpenGL!");
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    Shader s("shader_resources/shader.vert", "shader_resources/shader.frag");
    Shader cs("cubemapShader.vert", "cubemapShader.frag");
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

    pthread_mutex_lock(&mutex);
    for (int x = 0; x < 150; ++x) {
        for (int z = 0; z < 150; ++z) {
            chunks.emplace(glm::ivec2(x, z), std::make_unique<Chunk>(glm::ivec2(x, z)));
            auto &chunk = chunks.at(glm::ivec2(x, z));
            chunksToBuild.push(chunk.get());
        }
    }
    pthread_cond_broadcast(&cond);
    pthread_mutex_unlock(&mutex);

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

        s.Use();
        s.SetVector3("viewPos", camera.pos);
        s.SetVector3("directionalLight.direction", glm::vec3(-0.2f, -1.0f, -0.3f));
        s.SetVector3("directionalLight.ambient", glm::vec3(0.5f));
        s.SetVector3("directionalLight.diffuse", glm::vec3(0.7f));
        s.SetVector3("directionalLight.specular", glm::vec3(0.1f));
        s.SetMatrix4("projection", perspective);
        s.SetMatrix4("view", camera.GetViewMatrix());

        s.SetInt("tex", 0);
        s.SetInt("lod", 0);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, textureArray);

        for (auto &[pos, chunk] : chunks) {
            if (chunk->genState == ChunkGenState::Done && 
                frustum.IsCubeVisible(glm::vec3(
                                          (chunk->position.x * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f),
                                           chunk->maxHeight * 0.5f,
                                          (chunk->position.y * MAX_CHUNK_WIDTH) + (MAX_CHUNK_WIDTH * 0.5f)
                                      ),
                                      glm::vec3(MAX_CHUNK_WIDTH, chunk->maxHeight, MAX_CHUNK_WIDTH))) {
                chunk->Render(s, camera.pos, DISTANCE_FOR_LOD_CHUNK);
            }
        }

        glBindTexture(GL_TEXTURE_2D_ARRAY, 0);

        map.Draw(glm::vec3(0), glm::vec4(255), camera, cs, perspective);

        window.Swap();
        fprintf(stderr, "%d\n", window.GetFPS());
    }
    window.Close();
}
