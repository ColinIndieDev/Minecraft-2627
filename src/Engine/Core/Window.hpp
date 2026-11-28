#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <string>
#include <cstdio>

namespace Window {
    inline uint32_t ScreenWidth = 0, ScreenHeight = 0;
    inline GLFWwindow *WindowPtr = NULL;

    inline uint32_t NbFrames = 0;
    inline float LastTime = 0.0f;
    inline float LastFrame = 0.0f;
    inline float FirstFrame = 0.0f;
    inline float Dt = 0.0f;
    inline uint32_t Fps = 0;

    inline void (*FramebufferSizeCallback)(GLFWwindow *, int, int) = NULL;
    inline void (*CursorPosCallback)(GLFWwindow *, double, double) = NULL;
    inline void (*KeyCallback)(GLFWwindow *, int, int, int, int)   = NULL;

    void Init(uint32_t width, uint32_t height, std::string title, bool debugMode);
    void Close();

    void Update();
    void Swap();

    bool ShouldClose();

    float GetDeltaTime();
    uint32_t GetFPS();

    void Clear(const glm::vec4 &color);

    bool GetKeyPressed(int key);
}
