#include "Config.hpp"
#include "imgui.h"

#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <Core/Window.hpp>
#include <Performance/HeapTracker.hpp>
#include <Performance/StackTracker.hpp>
#include <Lighting/Shadow.hpp>

namespace Config {
    void InitMenu() {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGui_ImplGlfw_InitForOpenGL(Window::WindowPtr, true);
        ImGui_ImplOpenGL3_Init("#version 450");

        ImGuiIO& io = ImGui::GetIO();
        MenuFont = io.Fonts->AddFontFromFileTTF("assets/fonts/Minecraft.ttf", 14.0f);
        MenuFontBig = io.Fonts->AddFontFromFileTTF("assets/fonts/Minecraft.ttf", 21.0f);
    }

    void DisplayMenu() {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        if (ShouldRenderMenu) {
            if (ImGui::Begin("Menu")) {
                if (ImGui::CollapsingHeader("General")) {
                    ImGui::PushFont(MenuFont);

                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("Performance");
                    ImGui::PopFont();
                    ImGui::Text("FPS: %d", Window::Fps);
                    ImGui::Text("Last frame: %.1fms", Window::Dt * 1000.0f);
                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("Memory");
                    ImGui::PopFont();
                    ImGui::Text("Heap memory (current): %zu MiB", HeapTrack::cur / 1024 / 1024);
                    ImGui::Text("Heap memory (peak): %zu MiB", HeapTrack::peak / 1024 / 1024);
                    ImGui::Text("Stack memory (current): %.3f MiB", StackTracker::GetStackUsage() / 1024.0 / 1024.0);

                    pthread_mutex_lock(&Game::workerMutex);
                    uint64_t chunkStats[6] = {0};
                    for (auto &[pos, chunk] : Game::chunks) {
                        chunkStats[(uint32_t)chunk.genState]++;
                    }

                    ImGui::PushFont(MenuFontBig);
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

                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("Shadows");
                    ImGui::PopFont();
                    ImGui::Checkbox("Enable##1", &Shadow.drawShadows);
                    ImGui::SliderInt("Resolution", (int *)&Shadow.shadowResolution, 128, 16384);
                    if (Shadow.shadowResolution != Shadow.lastShadowResolution) {
                        DirectionalShadow::Resize(Shadow.shadowResolution, Shadow.shadowCascadeLevels.size() + 1);
                        Shadow.lastShadowResolution = Shadow.shadowResolution;
                    }

                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("Render Distance");
                    ImGui::PopFont();
                    ImGui::SliderInt("Chunks##1", &Game::viewDistance, 2, 256);

                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("Fog");
                    ImGui::PopFont();
                    ImGui::Checkbox("Enable##2", &Fog.fog);
                    ImGui::Text("Fog Start");
                    ImGui::SliderInt("Chunks##2", &Fog.fogStartChunk, 0, Game::viewDistance);
                    if (Fog.fogStartChunk != Fog.lastFogStartChunk) {
                        Fog.lastFogStartChunk = Fog.fogStartChunk;
                    }
                    ImGui::Text("Fog End");
                    ImGui::SliderInt("Chunks##3", &Fog.fogEndChunk, Fog.fogStartChunk, Game::viewDistance);
                    if (Fog.fogEndChunk != Fog.lastFogEndChunk) {
                        Fog.lastFogEndChunk = Fog.fogEndChunk;
                    }

                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("Normal Mapping");
                    ImGui::PopFont();
                    ImGui::Checkbox("Enable##3", &NormalMapping.normalMapping);
                    ImGui::SliderFloat("Normal Strength", &NormalMapping.normalStrength, 0.0f, 2.0f);

                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("Anti Aliasing");
                    ImGui::PopFont();
                    ImGui::Checkbox("MSAA", &Msaa.msaa);
                    if (Msaa.msaa != Msaa.prevMsaa) {
                        if (Msaa.msaa) {
                            glEnable(GL_MULTISAMPLE);
                        } else {
                            glDisable(GL_MULTISAMPLE);
                        }
                        Msaa.prevMsaa = Msaa.msaa;
                    }

                    ImGui::PopFont();
                }

                if (ImGui::CollapsingHeader("Lighting")) {
                    ImGui::PushFont(MenuFont);

                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("Directional Light");
                    ImGui::PopFont();

                    ImGui::SliderFloat3("Direction", (float *)&Environment.dirLightDirection, -1.0f, 1.0f);
                    ImGui::ColorEdit3("Ambient Color", (float *)&Environment.dirLightAmbient, ImGuiColorEditFlags_DisplayRGB);
                    ImGui::ColorEdit3("Diffuse Color", (float *)&Environment.dirLightDiffuse, ImGuiColorEditFlags_DisplayRGB);
                    ImGui::ColorEdit3("Specular Color", (float *)&Environment.dirLightSpecular, ImGuiColorEditFlags_DisplayRGB);

                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("Sun");
                    ImGui::PopFont();

                    ImGui::ColorEdit3("Sun Color", (float *)&Environment.sunColor, ImGuiColorEditFlags_DisplayRGB);

                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("HDR");
                    ImGui::PopFont();

                    ImGui::Checkbox("Enable##0", &Hdr.hdr);
                    ImGui::SliderFloat("Exposure##0", &Hdr.exposure, 0.0f, 10.0f);

                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("Bloom");
                    ImGui::PopFont();

                    ImGui::Checkbox("Enable##1", &Bloom.bloom);
                    ImGui::SliderFloat("Intensity", &Bloom.bloomIntensity, 0.0f, 3.0f);
                    ImGui::SliderInt("Blur Amount", &Bloom.totalBlurAmount, 1, 100);

                    ImGui::PushFont(MenuFontBig);
                    ImGui::Text("Godrays");
                    ImGui::PopFont();

                    ImGui::Checkbox("Enable##2", &GodRay.godRays);
                    ImGui::SliderInt("Samples", &GodRay.samples, 10, 1000);
                    ImGui::SliderFloat("Decay", &GodRay.decay, 0.0f, 2.0f);
                    ImGui::SliderFloat("Density", &GodRay.density, 0.0f, 2.0f);
                    ImGui::SliderFloat("Weight", &GodRay.weight, 0.0f, 2.0f);
                    ImGui::SliderFloat("Exposure##1", &GodRay.godRayExposure, 0.0f, 2.0f);

                    ImGui::PopFont();
                }

                if (ImGui::CollapsingHeader("Colors")) {
                    ImGui::PushFont(MenuFont);

                    ImGui::ColorEdit4("Water Color", (float *)&Environment.waterColor, ImGuiColorEditFlags_DisplayRGB);
                    ImGui::ColorEdit4("Leaves Color", (float *)&Environment.leavesColor, ImGuiColorEditFlags_DisplayRGB);
                    ImGui::ColorEdit4("Grass Color", (float *)&Environment.grassColor, ImGuiColorEditFlags_DisplayRGB);
                    ImGui::ColorEdit4("Skybox Tint", (float *)&Environment.skyboxTint, ImGuiColorEditFlags_DisplayRGB);

                    ImGui::PopFont();
                }
            }
            ImGui::End();
        }
        ImGui::Render();
    }

    void RenderMenu() {
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    void DestroyMenu() {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
    }
}
