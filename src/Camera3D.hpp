#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

struct Camera3D {
    glm::vec3 pos = glm::vec3(0);
    glm::vec3 front = glm::vec3(0, 0, 1);
    glm::vec3 up = glm::vec3(0, 1, 0);
    float yaw = -90.0f;
    float pitch = 0.0f;
    float lastX = 0.0f;
    float lastY = 0.0f;
    float fov = 45.0f;
    float sensitivity = 0.1f;
    bool firstMouse = true;

    Camera3D(const glm::vec3 &pos, float fov, float sensitivity) : pos(pos), fov(fov), sensitivity(sensitivity) {}

    glm::mat4 GetViewMatrix() {
        return glm::lookAt(pos, pos + front, up);
    }
};
