#pragma once

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <string>
#include <cstdio>

// Temporary
#include <iostream>

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

    static void APIENTRY glDebugOutput(GLenum source,
                                GLenum type,
                                unsigned int id,
                                GLenum severity,
                                GLsizei length,
                                const char *message,
                                const void *userParam) {
        if(id == 131169 || id == 131185 || id == 131218 || id == 131204) return;

        std::cout << "---------------" << std::endl;
        std::cout << "Debug message (" << id << "): " <<  message << std::endl;

        switch (source)
        {
            case GL_DEBUG_SOURCE_API:             std::cout << "Source: API"; break;
            case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   std::cout << "Source: Window System"; break;
            case GL_DEBUG_SOURCE_SHADER_COMPILER: std::cout << "Source: Shader Compiler"; break;
            case GL_DEBUG_SOURCE_THIRD_PARTY:     std::cout << "Source: Third Party"; break;
            case GL_DEBUG_SOURCE_APPLICATION:     std::cout << "Source: Application"; break;
            case GL_DEBUG_SOURCE_OTHER:           std::cout << "Source: Other"; break;
        } std::cout << std::endl;

        switch (type)
        {
            case GL_DEBUG_TYPE_ERROR:               std::cout << "Type: Error"; break;
            case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: std::cout << "Type: Deprecated Behaviour"; break;
            case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  std::cout << "Type: Undefined Behaviour"; break;
            case GL_DEBUG_TYPE_PORTABILITY:         std::cout << "Type: Portability"; break;
            case GL_DEBUG_TYPE_PERFORMANCE:         std::cout << "Type: Performance"; break;
            case GL_DEBUG_TYPE_MARKER:              std::cout << "Type: Marker"; break;
            case GL_DEBUG_TYPE_PUSH_GROUP:          std::cout << "Type: Push Group"; break;
            case GL_DEBUG_TYPE_POP_GROUP:           std::cout << "Type: Pop Group"; break;
            case GL_DEBUG_TYPE_OTHER:               std::cout << "Type: Other"; break;
        } std::cout << std::endl;

        switch (severity)
        {
            case GL_DEBUG_SEVERITY_HIGH:         std::cout << "Severity: high"; break;
            case GL_DEBUG_SEVERITY_MEDIUM:       std::cout << "Severity: medium"; break;
            case GL_DEBUG_SEVERITY_LOW:          std::cout << "Severity: low"; break;
            case GL_DEBUG_SEVERITY_NOTIFICATION: std::cout << "Severity: notification"; break;
        } std::cout << std::endl;
        std::cout << std::endl;
    }

    void Init(uint32_t width, uint32_t height, std::string title, bool debugMode) {
        glfwInitHint(GLFW_PLATFORM, GLFW_PLATFORM_X11);
        glfwInit();
        // OpenGL 4.5 (supports things we need for vertex pulling)
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, debugMode);
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

        int flags = 0; 
        glGetIntegerv(GL_CONTEXT_FLAGS, &flags);
        if (flags & GL_CONTEXT_FLAG_DEBUG_BIT) {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageCallback(glDebugOutput, NULL);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, NULL, GL_TRUE);
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
