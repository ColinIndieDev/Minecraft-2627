#include <string>
#include <unistd.h>
#include <pthread.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "GLUtil.hpp"
#include "Shader.hpp"
#include "Window.hpp"
#include "Cubemap.hpp"
#include "Chunk.hpp"
#include "Frustum.hpp"
#include "BlockTexture.hpp"
#include "Sun.hpp"
#include "Shadow.hpp"

#include "HeapTrack.hpp"
#include "StackTracker.hpp"

#include "Game.hpp"

#define ZNEAR 0.01f
#define ZFAR 1'000.0f

bool mouseLocked = true;

#pragma region GLFW Callbacks
void FramebufferSizeCallback(GLFWwindow *window, int width, int height) {
    if (height == 0 || width == 0) {
        return;
    }
    Game::perspective = glm::perspective(glm::radians(Game::camera.fov), (float)width / height, ZNEAR, ZFAR);
    glViewport(0, 0, width, height);
}

void CursorPosCallback(GLFWwindow *window, double xPosIn, double yPosIn) {
    if (!mouseLocked) return;

    float xPos = (float)xPosIn;
    float yPos = (float)yPosIn;

    if (Game::camera.firstMouse) {
        Game::camera.lastX = xPos;
        Game::camera.lastY = yPos;
        Game::camera.firstMouse = false;
    }

    float xOff = (xPos - Game::camera.lastX) * Game::camera.sensitivity;
    float yOff = (Game::camera.lastY - yPos) * Game::camera.sensitivity;

    Game::camera.lastX = xPos;
    Game::camera.lastY = yPos;

    Game::camera.yaw += xOff;
    Game::camera.pitch += yOff;

    Game::camera.pitch = fmin(Game::camera.pitch, 89.0f);
    Game::camera.pitch = fmax(Game::camera.pitch, -89.0f);

    Game::camera.front.x = cosf(glm::radians(Game::camera.yaw)) * cosf(glm::radians(Game::camera.pitch));
    Game::camera.front.y = sinf(glm::radians(Game::camera.pitch));
    Game::camera.front.z = sinf(glm::radians(Game::camera.yaw)) * cosf(glm::radians(Game::camera.pitch));
}

void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        int curMode = glfwGetInputMode(window, GLFW_CURSOR);
        
        if (curMode == GLFW_CURSOR_DISABLED) {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            mouseLocked = false;
        } else {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            mouseLocked = true;
            
            int width, height;
            glfwGetWindowSize(window, &width, &height);
            glfwSetCursorPos(window, width * 0.5f, height * 0.5f);
        }
    }
}

#pragma endregion

int main(void) {
    srand(time(NULL));

    Window window = {
        .framebufferSizeCallback = FramebufferSizeCallback,
        .cursorPosCallback = CursorPosCallback
    };
    window.Init(800, 500, "Hello OpenGL!", true);
    glfwSetKeyCallback(window.windowPtr, KeyCallback);
   
#pragma region ImGUI Setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window.windowPtr, true);
    ImGui_ImplOpenGL3_Init("#version 450");

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF("assets/fonts/Minecraft.ttf", 14.0f);
    ImFont* fontBig = io.Fonts->AddFontFromFileTTF("assets/fonts/Minecraft.ttf", 21.0f);
#pragma endregion

#pragma region OpenGL Setup
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glDepthFunc(GL_LESS);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
#pragma endregion

    Shader vertexPullShader("shader_resources/shaderPull.vert", "shader_resources/shader.frag");
    Shader cubemapShader("shader_resources/cubemapShader.vert", "shader_resources/cubemapShader.frag");
    Shader sunShader("shader_resources/sunShader.vert", "shader_resources/sunShader.frag");
    Shader depthMapShader("shader_resources/depthMapShader.vert", "shader_resources/depthMapShader.frag", "shader_resources/depthMapShader.geom");

    Game::perspective = glm::perspective(glm::radians(Game::camera.fov), (float)window.screenWidth / window.screenHeight, ZNEAR, ZFAR);

    std::string paths[6] = {
        "assets/sky/px.png",
        "assets/sky/nx.png",
        "assets/sky/py.png",
        "assets/sky/ny.png",
        "assets/sky/nz.png",
        "assets/sky/pz.png",
    };
    Cubemap map(paths);

    Sun sun;

    Frustum frustum;
    
    uint32_t lastShadowResolution = 4096;
    uint32_t shadowResolution = lastShadowResolution;
    bool drawShadows = false;

    std::vector<float> shadowCascadeLevels { 
        ZFAR / 100.0f, ZFAR / 70.0f, ZFAR / 50.0f, ZFAR / 25.0f 
    };
    DirectionalShadow directionalShadow(shadowResolution, shadowCascadeLevels.size() + 1);

    int lastFogStartChunk = DEFAULT_VIEW_DISTANCE - 2;
    int lastFogEndChunk = DEFAULT_VIEW_DISTANCE;
    int fogStartChunk = lastFogStartChunk;
    int fogEndChunk = lastFogEndChunk;
    bool fog = false;

    glm::vec3 dirLightDirection = glm::vec3(-0.2f, -0.3f, -0.5f);
    glm::vec3 dirLightAmbient = glm::vec3(0.1f);
    glm::vec3 dirLightDiffuse = glm::vec3(0.85f);
    glm::vec3 dirLightSpecular = glm::vec3(0.1f);

    uint32_t textureArray = GetTextureArray();
    uint32_t textureArrayNormals = GetTextureArrayNormals();

    float normalStrength = 1.0f;
    bool normalMapping = false;

    Game::CreateChunkGenWorkers();

    while (!window.ShouldClose()) {
        Game::UploadChunkDataToGPU();
        Game::GenChunksAroundPlayer();

        window.Update();
        frustum.Update(Game::perspective * Game::camera.GetViewMatrix());

#pragma region ImGUI Menu
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ImGui::Begin("General")) {
            ImGui::PushFont(font);

            ImGui::PushFont(fontBig);
            ImGui::Text("Performance");
            ImGui::PopFont();
            ImGui::Text("FPS: %d", window.GetFPS());
            ImGui::Text("Last frame: %.1fms", window.GetDeltaTime() * 1000.0f);
            ImGui::PushFont(fontBig);
            ImGui::Text("Memory");
            ImGui::PopFont();
            ImGui::Text("Heap memory (current): %zu MiB", HeapTrack::cur / 1024 / 1024);
            ImGui::Text("Heap memory (peak): %zu MiB", HeapTrack::peak / 1024 / 1024);
            ImGui::Text("Stack memory (current): %.3f MiB", StackTracker::GetStackUsage() / 1024.0 / 1024.0);

            pthread_mutex_lock(&Game::workerMutex);
            /* For comprehension the possible chunk states
            TerrainGenerated = 0,
            MeshQueued,
            MeshBuilding,
            MeshReadyForUpload,
            MeshReady,
            MeshDirty
            */
            uint64_t chunkStats[6] = {0};
            for (auto &[pos, chunk] : Game::chunks) {
                chunkStats[(uint32_t)chunk.genState]++;
            }

            ImGui::PushFont(fontBig);
            ImGui::Text("Chunks");
            ImGui::PopFont();
            ImGui::Text("Terrain generated: %zu", chunkStats[0]);
            ImGui::Text("Queued: %zu", chunkStats[1]);
            ImGui::Text("Building: %zu", chunkStats[2]);
            ImGui::Text("Ready to upload: %zu", chunkStats[3]);
            ImGui::Text("Dirty: %zu", chunkStats[5]);
            ImGui::Text("Ready: %zu", chunkStats[4]);
            ImGui::Text("Total: %zu", Game::chunks.size());
            pthread_mutex_unlock(&Game::workerMutex);

            ImGui::PushFont(fontBig);
            ImGui::Text("Shadows");
            ImGui::PopFont();
            ImGui::Checkbox("Enable##1", &drawShadows);
            ImGui::SliderInt("Resolution", (int *)&shadowResolution, 128, 16384);
            if (shadowResolution != lastShadowResolution) {
                directionalShadow.Resize(shadowResolution, shadowCascadeLevels.size() + 1);
                lastShadowResolution = shadowResolution;
            }

            ImGui::PushFont(fontBig);
            ImGui::Text("Render Distance");
            ImGui::PopFont();
            ImGui::SliderInt("Chunks##1", &Game::viewDistance, 2, 256);

            ImGui::PushFont(fontBig);
            ImGui::Text("Fog");
            ImGui::PopFont();
            ImGui::Checkbox("Enable##2", &fog);
            ImGui::Text("Fog Start");
            ImGui::SliderInt("Chunks##2", &fogStartChunk, 0, Game::viewDistance);
            if (fogStartChunk != lastFogStartChunk) {
                lastFogStartChunk = fogStartChunk;
            }
            ImGui::Text("Fog End");
            ImGui::SliderInt("Chunks##3", &fogEndChunk, fogStartChunk, Game::viewDistance);
            if (fogEndChunk != lastFogEndChunk) {
                lastFogEndChunk = fogEndChunk;
            }

            ImGui::PushFont(fontBig);
            ImGui::Text("Normal Mapping");
            ImGui::PopFont();
            ImGui::Checkbox("Enable##3", &normalMapping);
            ImGui::SliderFloat("Normal Strength", &normalStrength, 0.0f, 2.0f);

            ImGui::PopFont();
        }
        ImGui::End();

        if (ImGui::Begin("Lighting")) {
            ImGui::PushFont(font);

            ImGui::PushFont(fontBig);
            ImGui::Text("Directional Light");
            ImGui::PopFont();
            ImGui::SliderFloat3("Direction", (float *)&dirLightDirection, -1.0f, 1.0f);
            ImGui::SliderFloat3("Ambient", (float *)&dirLightAmbient, 0.0f, 1.0f);
            ImGui::SliderFloat3("Diffuse", (float *)&dirLightDiffuse, 0.0f, 1.0f);
            ImGui::SliderFloat3("Specular", (float *)&dirLightSpecular, 0.0f, 1.0f);

            ImGui::PopFont();
        }
        ImGui::End();

        ImGui::Render();
#pragma endregion

        window.Clear(glm::vec4(0, 255, 255, 255));

        Game::HandleInput(window);

        std::vector<Chunk *> chunksSorted;
        Game::GetVisibleChunks(chunksSorted, frustum);

#pragma region Shadows
        glm::mat4 lightSpaceMatrix;
        if (drawShadows) {
            float nearPlane = 0.1f, farPlane = 500.0f;
            glm::mat4 lightProjection = glm::ortho(-150.0f, 150.0f, -150.0f, 150.0f, nearPlane, farPlane);

            glm::vec3 lightDir = glm::normalize(
                -dirLightDirection
            );
            glm::vec3 lightPos = Game::camera.pos - lightDir * 200.0f;

            glm::mat4 lightView = glm::lookAt(lightPos,
                                              Game::camera.pos,
                                              glm::vec3(0.0f, 1.0f, 0.0f));
            lightSpaceMatrix = lightProjection * lightView;

            directionalShadow.LoadLightSpaceMatricesToBuffer(shadowCascadeLevels, Game::perspective, Game::camera.GetViewMatrix(), lightDir, 
                                                             ZNEAR, ZFAR, (float)window.screenWidth / window.screenHeight, Game::camera.fov);

            depthMapShader.Use();

            directionalShadow.BindDepthMap();

            for (auto &chunk : chunksSorted) {
                chunk->Render(depthMapShader, Game::camera.pos);
                chunk->RenderTransparent(depthMapShader, Game::camera.pos);
            }

            directionalShadow.UnbindDepthMap();

            glViewport(0, 0, window.screenWidth, window.screenHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
#pragma endregion

        map.Draw(glm::vec3(0), glm::vec4(255), Game::camera, cubemapShader, Game::perspective);

#pragma region Rendering Setup
        vertexPullShader.Use();
        vertexPullShader.SetVector3("viewPos", Game::camera.pos);
        vertexPullShader.SetVector3("directionalLight.direction", dirLightDirection);
        vertexPullShader.SetVector3("directionalLight.ambient", dirLightAmbient);
        vertexPullShader.SetVector3("directionalLight.diffuse", dirLightDiffuse);
        vertexPullShader.SetVector3("directionalLight.specular", dirLightSpecular);

        vertexPullShader.SetMatrix4("projection", Game::perspective);
        vertexPullShader.SetMatrix4("view", Game::camera.GetViewMatrix());

        vertexPullShader.SetFloat("farPlane", ZFAR);
        vertexPullShader.SetInt("cascadeCount", shadowCascadeLevels.size());
        for (int i = 0; i < shadowCascadeLevels.size(); ++i) {
            vertexPullShader.SetFloat("cascadePlaneDistances[" + std::to_string(i) + "]", shadowCascadeLevels[i]);
        }

        vertexPullShader.SetInt("fogStartBlocks", fogStartChunk * MAX_CHUNK_WIDTH);
        vertexPullShader.SetInt("fogEndBlocks", fogEndChunk * MAX_CHUNK_WIDTH);

        vertexPullShader.SetFloat("normalStrength", normalStrength);

        vertexPullShader.SetInt("tex", 0);
        vertexPullShader.SetInt("shadowMap", 1);
        vertexPullShader.SetInt("texNormal", 2);

        vertexPullShader.SetBool("fog", fog);
        vertexPullShader.SetBool("shadows", drawShadows);
        vertexPullShader.SetBool("normalMapping", normalMapping);

        glBindTextureUnit(0, textureArray);
        glBindTextureUnit(1, directionalShadow.depthMaps);
        glBindTextureUnit(2, textureArrayNormals);
#pragma endregion

        for (auto &chunk : chunksSorted) {
            chunk->Render(vertexPullShader, Game::camera.pos);
        }

        for (auto &chunk : chunksSorted) {
            chunk->RenderTransparent(vertexPullShader, Game::camera.pos);
        }

        sun.Draw(sunShader, Game::camera, Game::perspective, dirLightDirection);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.Swap();
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    window.Close();
}
