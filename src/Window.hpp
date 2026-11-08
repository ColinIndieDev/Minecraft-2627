#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <string>
#include <cstdio>

struct Window {
    uint32_t screenWidth = 0, screenHeight = 0;
    GLFWwindow *windowPtr = NULL;

    uint32_t nbFrames = 0;
    float lastTime = 0.0f;
    float lastFrame = 0.0f;
    float firstFrame = 0.0f;
    float dt = 0.0f;
    uint32_t fps = 0;

    void (*framebufferSizeCallback)(GLFWwindow *, int, int) = NULL;
    void (*cursorPosCallback)(GLFWwindow *, double, double) = NULL;

    void Init(uint32_t width, uint32_t height, std::string title) {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
        glfwInit();
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

        windowPtr = glfwCreateWindow(width, height, title.c_str(), NULL, NULL);
        if (!windowPtr) {
            fprintf(stderr, "Failed to create GLFW window\n");
            glfwTerminate();
            exit(-1);
        }
        glfwMakeContextCurrent(windowPtr);
        glfwSwapInterval(0);

        glfwSetFramebufferSizeCallback(windowPtr, framebufferSizeCallback);
        glfwSetCursorPosCallback(windowPtr, cursorPosCallback);

        glfwSetInputMode(windowPtr, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

        if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
            fprintf(stderr, "Failed to init GLAD\n");
            exit(-1);
        }    

        int w, h;
        glfwGetFramebufferSize(windowPtr, &w, &h);
        glViewport(0, 0, w, h);
        screenWidth = w;
        screenHeight = h;
    }

    void Update() {
        glfwGetWindowSize(windowPtr, (int *)&screenWidth, (int *)&screenHeight);
        float curTime = glfwGetTime();
        nbFrames++;
        if (curTime- lastTime >= 1.0) {
            fps = nbFrames;
            nbFrames = 0;
            lastTime += 1.0f;
        }
        dt = curTime - lastFrame;
        lastFrame = curTime;
    }

    float GetDeltaTime() {
        return dt;
    }

    uint32_t GetFPS() {
        return fps;
    }

    bool ShouldClose() {
        return glfwWindowShouldClose(windowPtr);
    }

    void Clear(const glm::vec4 &color) {
        glClearColor(color.r / 255.0f, color.g / 255.0f, color.b / 255.0f, color.a / 255.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    bool GetKeyPressed(int key) {
        return glfwGetKey(windowPtr, key) == GLFW_PRESS;
    }

    void Swap() {
        glfwSwapBuffers(windowPtr);
        glfwPollEvents();
    }

    void Close() {
        glfwTerminate();
    }
};
