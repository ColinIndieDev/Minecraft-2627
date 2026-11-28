#include "Minecraft.hpp"

#include <algorithm>

#include <Environment/Cubemap.hpp>
#include <Core/Window.hpp>
#include <PostProcessing/HDR.hpp>
#include <PostProcessing/Bloom.hpp>
#include <PostProcessing/Godrays.hpp>
#include <Environment/Sun.hpp>
#include <Lighting/Shadow.hpp>

#include "Config.hpp"
#include "BlockTexture.hpp"

namespace Game {
#pragma region GLFW Callbacks
void FramebufferSizeCallback(GLFWwindow *window, int width, int height) {
    if (height == 0 || width == 0) {
        return;
    }
    perspective = glm::perspective(glm::radians(camera.fov), (float)width / height, ZNEAR, ZFAR);
    HDR::Resize(width, height);
    Bloom::Resize(width, height);
    Godrays::Resize(width, height);

    glViewport(0, 0, width, height);
}

void CursorPosCallback(GLFWwindow *window, double xPosIn, double yPosIn) {
    if (!mouseLocked) return;

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
    if (key == GLFW_KEY_H && action == GLFW_PRESS) {
        Config::ShouldRenderMenu = !Config::ShouldRenderMenu;
    }
}
#pragma endregion

#pragma region Multithreading
    void *ChunkGenWorker(void *args) {
        while (true) {
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

    void CreateChunkGenWorkers() {
        for (int i = 0; i < NUM_WORKER_THREADS; ++i) {
            pthread_create(&workerThreads[i], NULL, ChunkGenWorker, NULL);
        }
    }

    void DestroyChunkGenWorkers() {
        pthread_mutex_lock(&workerMutex);

        workerRunning = false;

        pthread_cond_broadcast(&workerCond);

        pthread_mutex_unlock(&workerMutex);

        for (int i = 0; i < NUM_WORKER_THREADS; ++i) {
            pthread_join(workerThreads[i], NULL);
        }
    }
#pragma endregion

#pragma region Chunk Management
    void UploadChunkDataToGPU() {
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

    void AddChunkToBuildQueue(Chunk *chunk) {
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

    void GenChunksAroundPlayer() {
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
#pragma endregion

    void Init() {
        srand(time(NULL));

        Window::FramebufferSizeCallback = FramebufferSizeCallback,
        Window::CursorPosCallback = CursorPosCallback;
        Window::KeyCallback = KeyCallback;
        Window::Init(800, 500, "Hello OpenGL!", true);
        Config::InitMenu();
       
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
        vertexPullShader   = Shader("shader_resources/vertexPullShader.vert",
                                    "shader_resources/lightingShader.frag");
        cubemapShader      = Shader("shader_resources/cubemap/cubemapShader.vert",
                                    "shader_resources/cubemap/cubemapShader.frag");
        sunShader          = Shader("shader_resources/sun/sunShader.vert",
                                    "shader_resources/sun/sunShader.frag");
        depthMapShader     = Shader("shader_resources/depthmap/depthMapShader.vert",   
                                    "shader_resources/depthmap/depthMapShader.frag", 
                                    "shader_resources/depthmap/depthMapShader.geom");
        hdrShader          = Shader("shader_resources/pp/quadShader.vert",
                                    "shader_resources/pp/hdrShader.frag");
        blurShader         = Shader("shader_resources/pp/quadShader.vert",       
                                    "shader_resources/pp/blurShader.frag");
        occlusionShader    = Shader("shader_resources/occlusionShader.vert",       
                                    "shader_resources/occlusionShader.frag");
        occlusionSunShader = Shader("shader_resources/sun/sunShader.vert",       
                                    "shader_resources/sun/occlusionSunShader.frag");
        modelShader        = Shader("shader_resources/models/modelShader.vert",
                                    "shader_resources/models/modelShader.frag");
#pragma endregion

        perspective = glm::perspective(glm::radians(camera.fov), (float)Window::ScreenWidth / Window::ScreenHeight, ZNEAR, ZFAR);

        std::string paths[6] = {
            "assets/sky/px.png",
            "assets/sky/nx.png",
            "assets/sky/py.png",
            "assets/sky/ny.png",
            "assets/sky/nz.png",
            "assets/sky/pz.png",
        };
        map = Cubemap(paths);

        Sun::Init();
        DirectionalShadow::Init(Config::Shadow.shadowResolution, Config::Shadow.shadowCascadeLevels.size() + 1);
        HDR::Init(Window::ScreenWidth, Window::ScreenHeight);
        Bloom::Init(Window::ScreenWidth, Window::ScreenHeight);
        Godrays::Init(Window::ScreenWidth, Window::ScreenHeight);

        textureArray = GetTextureArray();
        textureArrayNormals = GetTextureArrayNormals();

        model = Model("assets/jeffrey_epstein/scene.gltf");

        if (Config::Msaa.msaa) glEnable(GL_MULTISAMPLE);

        CreateChunkGenWorkers();

#pragma region Texture Binding Shader Setup
        hdrShader.SetTextureBinding("hdrBuffer", 0);
        hdrShader.SetTextureBinding("bloomBlur", 1);
        hdrShader.SetTextureBinding("occlusionBuffer", 2);

        vertexPullShader.SetTextureBinding("tex", 0);
        vertexPullShader.SetTextureBinding("shadowMap", 1);
        vertexPullShader.SetTextureBinding("texNormal", 2);

        occlusionShader.SetTextureBinding("tex", 0);

        modelShader.SetTextureBinding("texture_diffuse1", 0);
#pragma endregion
    }

    void Update() {
        UploadChunkDataToGPU();
        GenChunksAroundPlayer();

        Window::Update();
        frustum.Update(perspective * camera.GetViewMatrix());

        HandleInput();

        GetVisibleChunks(chunksSorted, frustum);
    }

    void Render() {
        Config::DisplayMenu();
        Window::Clear(glm::vec4(0, 0, 0, 255));

        if (Config::GodRay.godRays) {
            Godrays::BindOcclusionFramebuffer();
            occlusionShader.Use();
            occlusionShader.SetMatrix4("projection", perspective);
            occlusionShader.SetMatrix4("view", camera.GetViewMatrix());
            glBindTextureUnit(0, textureArray);

            for (auto &chunk : chunksSorted) {
                chunk->Render(occlusionShader, camera.pos);
            }

            for (auto &chunk : chunksSorted) {
                chunk->RenderTransparent(occlusionShader, camera.pos);
            }

            occlusionSunShader.Use();

            Sun::Draw(occlusionSunShader, camera, perspective, Config::Environment.dirLightDirection);
            
            Godrays::UnbindOcclusionFramebuffer();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

#pragma region Shadow Rendering
        if (Config::Shadow.drawShadows) {
            DirectionalShadow::LoadLightSpaceMatricesToBuffer(Config::Shadow.shadowCascadeLevels, perspective, camera.GetViewMatrix(), 
                                                              glm::normalize(-Config::Environment.dirLightDirection), 
                                                             ZNEAR, ZFAR, (float)Window::ScreenWidth / Window::ScreenHeight, camera.fov);

            depthMapShader.Use();

            DirectionalShadow::BindDepthMap();

            for (auto &chunk : chunksSorted) {
                chunk->Render(depthMapShader, camera.pos);
                chunk->RenderTransparent(depthMapShader, camera.pos);
            }

            DirectionalShadow::UnbindDepthMap();

            glViewport(0, 0, Window::ScreenWidth, Window::ScreenHeight);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }
#pragma endregion

        HDR::BindFramebuffer(Config::Msaa.msaa);

#pragma region Rendering Scene
        map.Draw(glm::vec3(0), glm::vec4(Config::Environment.skyboxTint * 255.0f, 255.0f), camera, cubemapShader, perspective);

#pragma region Shader Setup
        vertexPullShader.Use();
        vertexPullShader.SetVector3("viewPos", camera.pos);
        vertexPullShader.SetVector3("directionalLight.direction", Config::Environment.dirLightDirection);
        vertexPullShader.SetVector3("directionalLight.ambient", Config::Environment.dirLightAmbient);
        vertexPullShader.SetVector3("directionalLight.diffuse", Config::Environment.dirLightDiffuse);
        vertexPullShader.SetVector3("directionalLight.specular", Config::Environment.dirLightSpecular);

        vertexPullShader.SetMatrix4("projection", perspective);
        vertexPullShader.SetMatrix4("view", camera.GetViewMatrix());

        vertexPullShader.SetFloat("farPlane", ZFAR);
        vertexPullShader.SetInt("cascadeCount", Config::Shadow.shadowCascadeLevels.size());
        for (int i = 0; i < Config::Shadow.shadowCascadeLevels.size(); ++i) {
            vertexPullShader.SetFloat("cascadePlaneDistances[" + std::to_string(i) + "]", Config::Shadow.shadowCascadeLevels[i]);
        }

        vertexPullShader.SetInt("fogStartBlocks", Config::Fog.fogStartChunk * MAX_CHUNK_WIDTH);
        vertexPullShader.SetInt("fogEndBlocks", Config::Fog.fogEndChunk * MAX_CHUNK_WIDTH);

        vertexPullShader.SetFloat("normalStrength", Config::NormalMapping.normalStrength);

        vertexPullShader.SetBool("fog", Config::Fog.fog);
        vertexPullShader.SetBool("shadows", Config::Shadow.drawShadows);
        vertexPullShader.SetBool("normalMapping", Config::NormalMapping.normalMapping);

        vertexPullShader.SetVector4("waterColor", Config::Environment.waterColor);
        vertexPullShader.SetVector4("grassColor", Config::Environment.grassColor);
        vertexPullShader.SetVector4("leavesColor", Config::Environment.leavesColor);

        glBindTextureUnit(0, textureArray);
        glBindTextureUnit(1, DirectionalShadow::_depthMaps);
        glBindTextureUnit(2, textureArrayNormals);
#pragma endregion

        for (auto &chunk : chunksSorted) {
            chunk->Render(vertexPullShader, camera.pos);
        }

        for (auto &chunk : chunksSorted) {
            chunk->RenderTransparent(vertexPullShader, camera.pos);
        }

        // TODO testing here
        modelShader.Use();
        modelShader.SetMatrix4("projection", perspective);
        modelShader.SetMatrix4("view", camera.GetViewMatrix());
        modelShader.SetMatrix4("transform", glm::mat4(1.0f));
        model.Draw(modelShader);

        sunShader.Use();
        sunShader.SetVector3("sunColor", Config::Environment.sunColor);
        Sun::Draw(sunShader, camera, perspective, Config::Environment.dirLightDirection);
#pragma endregion

        if (Config::Msaa.msaa) {
            HDR::ResolveFramebuffer(Window::ScreenWidth, Window::ScreenHeight);
        }
        HDR::UnbindFramebuffer();

        bool horizontal = false;
        if (Config::Bloom.bloom) {
            horizontal = Bloom::BlurColorBuffer(blurShader, Config::Msaa.msaa ? HDR::_msaaColorBuffers[1] : HDR::_colorBuffers[1], Config::Bloom.totalBlurAmount);
        }

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#pragma region Shader Setup
        hdrShader.Use();
        hdrShader.SetBool("bloom", Config::Bloom.bloom);
        hdrShader.SetBool("hdr", Config::Hdr.hdr);
        hdrShader.SetFloat("exposure", Config::Hdr.exposure);
        hdrShader.SetFloat("bloomIntensity", Config::Bloom.bloomIntensity);
        // God ray stuff
        hdrShader.SetBool("godRays", Config::GodRay.godRays);
        hdrShader.SetInt("samples", Config::GodRay.samples);
        hdrShader.SetFloat("decay", Config::GodRay.decay);
        hdrShader.SetFloat("density", Config::GodRay.density);
        hdrShader.SetFloat("weight", Config::GodRay.weight);
        hdrShader.SetFloat("godRayExposure", Config::GodRay.godRayExposure);

        hdrShader.SetVector3("cameraPos", camera.pos);
        hdrShader.SetVector3("lightDirection", Config::Environment.dirLightDirection);
        hdrShader.SetMatrix4("projection", perspective);
        hdrShader.SetMatrix4("view", camera.GetViewMatrix());

        glBindTextureUnit(0, HDR::_colorBuffers[0]);
        if (Config::Bloom.bloom) {
            glBindTextureUnit(1, Bloom::PingPongColorBuffers[!horizontal]);
        }
        if (Config::GodRay.godRays) {
            glBindTextureUnit(2, Godrays::_occlusionColorBuffer);
        }
#pragma endregion

        HDR::DrawScreenQuad();

        Config::RenderMenu();

        Window::Swap();
    }

    void Destroy() {
        DestroyChunkGenWorkers();

        Config::DestroyMenu();

        Bloom::Destroy();
        HDR::Destroy();
        DirectionalShadow::Destroy();
        Sun::Destroy();

        Window::Close();
    }

    glm::vec2 GetPlayerChunkPos() {
        return glm::ivec2(
                (int)std::floor(camera.pos.x / MAX_CHUNK_WIDTH),
                (int)std::floor(camera.pos.z / MAX_CHUNK_WIDTH)
                );
    }

    void HandleInput() {
        float speed = 100.0f * Window::Dt;
        if (Window::GetKeyPressed(GLFW_KEY_W)) {
            camera.pos += speed * camera.front;
        }
        if (Window::GetKeyPressed(GLFW_KEY_S)) {
            camera.pos -= speed * camera.front;
        }
        if (Window::GetKeyPressed(GLFW_KEY_A)) {
            camera.pos -= glm::normalize(glm::cross(camera.front, camera.up)) * speed;
        }
        if (Window::GetKeyPressed(GLFW_KEY_D)) {
            camera.pos += glm::normalize(glm::cross(camera.front, camera.up)) * speed;
        }
        if (Window::GetKeyPressed(GLFW_KEY_LEFT_SHIFT)) {
            camera.pos.y -= speed;
        }
        if (Window::GetKeyPressed(GLFW_KEY_SPACE)) {
            camera.pos.y += speed;
        }
    }

    void GetVisibleChunks(std::vector<Chunk *> &chunksSorted, Frustum &frustum) {
        chunksSorted.clear();
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
