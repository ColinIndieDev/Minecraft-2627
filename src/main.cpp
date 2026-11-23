#include <string>
#include <unistd.h>
#include <pthread.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include "Shader.hpp"
#include "Window.hpp"
#include "Cubemap.hpp"
#include "Chunk.hpp"
#include "Frustum.hpp"
#include "BlockTexture.hpp"
#include "Sun.hpp"
#include "Shadow.hpp"
#include "HDR.hpp"
#include "Bloom.hpp"

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
    HDR::Resize(width, height);
    Bloom::Resize(width, height);

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

#pragma region Shader Creation
    Shader vertexPullShader("shader_resources/vertexPullShader.vert",
                            "shader_resources/lightingShader.frag");
    Shader cubemapShader   ("shader_resources/cubemap/cubemapShader.vert",
                            "shader_resources/cubemap/cubemapShader.frag");
    Shader sunShader       ("shader_resources/sun/sunShader.vert",
                            "shader_resources/sun/sunShader.frag");
    Shader depthMapShader  ("shader_resources/depthmap/depthMapShader.vert",   
                            "shader_resources/depthmap/depthMapShader.frag", 
                            "shader_resources/depthmap/depthMapShader.geom");
    Shader hdrShader       ("shader_resources/pp/quadShader.vert",
                            "shader_resources/pp/hdrShader.frag");
    Shader blurShader      ("shader_resources/pp/quadShader.vert",       
                            "shader_resources/pp/blurShader.frag");
#pragma endregion

    Game::perspective = glm::perspective(glm::radians(Game::camera.fov), (float)window.screenWidth / window.screenHeight, ZNEAR, ZFAR);

    uint32_t lastShadowResolution = 4096;
    uint32_t shadowResolution = lastShadowResolution;
    bool drawShadows = false;
    std::vector<float> shadowCascadeLevels { 
        ZFAR / 100.0f, ZFAR / 70.0f, ZFAR / 50.0f, ZFAR / 25.0f 
    };

    int lastFogStartChunk = DEFAULT_VIEW_DISTANCE - 2;
    int lastFogEndChunk = DEFAULT_VIEW_DISTANCE;
    int fogStartChunk = lastFogStartChunk;
    int fogEndChunk = lastFogEndChunk;
    bool fog = false;

    bool hdr = false;
    float exposure = 1.0f;

    bool bloom = false;
    int totalBlurAmount = 10;
    float bloomIntensity = 1.0f;

    glm::vec3 dirLightDirection = glm::vec3(-0.2f, -0.3f, -0.5f);
    glm::vec3 dirLightAmbient = glm::vec3(0.1f);
    glm::vec3 dirLightDiffuse = glm::vec3(0.85f);
    glm::vec3 dirLightSpecular = glm::vec3(0.1f);

    glm::vec3 sunColor = glm::vec3(1.0f, 1.0f, 0.3f);

    glm::vec3 skyboxTint = glm::vec3(1.0f, 1.0f, 1.0f);

    float normalStrength = 1.0f;
    bool normalMapping = false;

    bool prevMsaa = false;
    bool msaa = false;

    glm::vec4 grassColor  = glm::vec4(67, 255, 32,  255);
    glm::vec4 leavesColor = glm::vec4(20, 156, 45,  255);
    glm::vec4 waterColor  = glm::vec4(12, 155, 220, 200);
    grassColor /= 255.0f;
    leavesColor /= 255.0f;
    waterColor /= 255.0f;

    std::string paths[6] = {
        "assets/sky/px.png",
        "assets/sky/nx.png",
        "assets/sky/py.png",
        "assets/sky/ny.png",
        "assets/sky/nz.png",
        "assets/sky/pz.png",
    };

    Cubemap map(paths);
    Frustum frustum;

    Sun::Init();
    DirectionalShadow::Init(shadowResolution, shadowCascadeLevels.size() + 1);
    HDR::Init(window.screenWidth, window.screenHeight);
    Bloom::Init(window.screenWidth, window.screenHeight);

    uint32_t textureArray = GetTextureArray();
    uint32_t textureArrayNormals = GetTextureArrayNormals();

    if (msaa) glEnable(GL_MULTISAMPLE);

    Game::CreateChunkGenWorkers();

#pragma region Texture Binding Shader Setup
    hdrShader.Use();
    hdrShader.SetTextureBinding("hdrBuffer", 0);
    hdrShader.SetTextureBinding("bloomBlur", 1);

    vertexPullShader.Use();
    vertexPullShader.SetTextureBinding("tex", 0);
    vertexPullShader.SetTextureBinding("shadowMap", 1);
    vertexPullShader.SetTextureBinding("texNormal", 2);
#pragma endregion

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
                DirectionalShadow::Resize(shadowResolution, shadowCascadeLevels.size() + 1);
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

            ImGui::PushFont(fontBig);
            ImGui::Text("Anti Aliasing");
            ImGui::PopFont();
            ImGui::Checkbox("MSAA", &msaa);
            if (msaa != prevMsaa) {
                if (msaa) {
                    glEnable(GL_MULTISAMPLE);
                } else {
                    glDisable(GL_MULTISAMPLE);
                }
                prevMsaa = msaa;
            }

            ImGui::PopFont();
        }
        ImGui::End();

        if (ImGui::Begin("Lighting")) {
            ImGui::PushFont(font);

            ImGui::PushFont(fontBig);
            ImGui::Text("Directional Light");
            ImGui::PopFont();

            ImGui::SliderFloat3("Direction", (float *)&dirLightDirection, -1.0f, 1.0f);
            ImGui::ColorEdit3("Ambient Color", (float *)&dirLightAmbient, ImGuiColorEditFlags_DisplayRGB);
            ImGui::ColorEdit3("Diffuse Color", (float *)&dirLightDiffuse, ImGuiColorEditFlags_DisplayRGB);
            ImGui::ColorEdit3("Specular Color", (float *)&dirLightSpecular, ImGuiColorEditFlags_DisplayRGB);

            ImGui::PushFont(fontBig);
            ImGui::Text("Sun");
            ImGui::PopFont();

            ImGui::ColorEdit3("Sun Color", (float *)&sunColor, ImGuiColorEditFlags_DisplayRGB);

            ImGui::PushFont(fontBig);
            ImGui::Text("HDR");
            ImGui::PopFont();

            ImGui::Checkbox("Enable##0", &hdr);
            ImGui::SliderFloat("Exposure", &exposure, 0.0f, 10.0f);

            ImGui::PushFont(fontBig);
            ImGui::Text("Bloom");
            ImGui::PopFont();

            ImGui::Checkbox("Enable##1", &bloom);
            ImGui::SliderFloat("Intensity", &bloomIntensity, 0.0f, 3.0f);
            ImGui::SliderInt("Blur Amount", &totalBlurAmount, 1, 100);

            ImGui::PopFont();
        }
        ImGui::End();

        if (ImGui::Begin("Colors")) {
            ImGui::PushFont(font);

            ImGui::ColorEdit4("Water Color", (float *)&waterColor, ImGuiColorEditFlags_DisplayRGB);
            ImGui::ColorEdit4("Leaves Color", (float *)&leavesColor, ImGuiColorEditFlags_DisplayRGB);
            ImGui::ColorEdit4("Grass Color", (float *)&grassColor, ImGuiColorEditFlags_DisplayRGB);
            ImGui::ColorEdit4("Skybox Tint", (float *)&skyboxTint, ImGuiColorEditFlags_DisplayRGB);

            ImGui::PopFont();
        }
        ImGui::End();

        ImGui::Render();
#pragma endregion

        window.Clear(glm::vec4(0, 0, 0, 255));

        Game::HandleInput(window);

        std::vector<Chunk *> chunksSorted;
        Game::GetVisibleChunks(chunksSorted, frustum);

#pragma region Shadow Rendering
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

            DirectionalShadow::LoadLightSpaceMatricesToBuffer(shadowCascadeLevels, Game::perspective, Game::camera.GetViewMatrix(), lightDir, 
                                                             ZNEAR, ZFAR, (float)window.screenWidth / window.screenHeight, Game::camera.fov);

            depthMapShader.Use();

            DirectionalShadow::BindDepthMap();

            for (auto &chunk : chunksSorted) {
                chunk->Render(depthMapShader, Game::camera.pos);
                chunk->RenderTransparent(depthMapShader, Game::camera.pos);
            }

            DirectionalShadow::UnbindDepthMap();

            glViewport(0, 0, window.screenWidth, window.screenHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
#pragma endregion

        HDR::BindFramebuffer(msaa);

#pragma region Rendering Scene
        map.Draw(glm::vec3(0), glm::vec4(skyboxTint * 255.0f, 255.0f), Game::camera, cubemapShader, Game::perspective);

#pragma region Shader Setup
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

        vertexPullShader.SetBool("fog", fog);
        vertexPullShader.SetBool("shadows", drawShadows);
        vertexPullShader.SetBool("normalMapping", normalMapping);

        vertexPullShader.SetVector4("waterColor", waterColor);
        vertexPullShader.SetVector4("grassColor", grassColor);
        vertexPullShader.SetVector4("leavesColor", leavesColor);

        glBindTextureUnit(0, textureArray);
        glBindTextureUnit(1, DirectionalShadow::_depthMaps);
        glBindTextureUnit(2, textureArrayNormals);
#pragma endregion

        for (auto &chunk : chunksSorted) {
            chunk->Render(vertexPullShader, Game::camera.pos);
        }

        for (auto &chunk : chunksSorted) {
            chunk->RenderTransparent(vertexPullShader, Game::camera.pos);
        }

        sunShader.Use();
        sunShader.SetVector3("sunColor", sunColor);
        Sun::Draw(sunShader, Game::camera, Game::perspective, dirLightDirection);
#pragma endregion

        if (msaa) {
            HDR::ResolveFramebuffer(window.screenWidth, window.screenHeight);
        }
        HDR::UnbindFramebuffer();

        bool horizontal = false;
        if (bloom) {
            horizontal = Bloom::BlurColorBuffer(blurShader, msaa ? HDR::_msaaColorBuffers[1] : HDR::_colorBuffers[1], totalBlurAmount);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#pragma region Shader Setup
        hdrShader.Use();
        hdrShader.SetBool("bloom", bloom);
        hdrShader.SetBool("hdr", hdr);
        hdrShader.SetFloat("exposure", exposure);
        hdrShader.SetFloat("bloomIntensity", bloomIntensity);
        glBindTextureUnit(0, HDR::_colorBuffers[0]);
        if (bloom) {
            glBindTextureUnit(1, Bloom::_pingPongColorBuffers[!horizontal]);
        }
#pragma endregion

        HDR::DrawScreenQuad();

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        window.Swap();
    }
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    Bloom::Destroy();
    HDR::Destroy();
    DirectionalShadow::Destroy();
    Sun::Destroy();

    window.Close();
}
