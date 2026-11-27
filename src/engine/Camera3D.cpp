#include "Camera3D.hpp"

Camera3D::Camera3D(const glm::vec3 &pos, float fov, float sensitivity) : pos(pos), fov(fov), sensitivity(sensitivity) {}

glm::mat4 Camera3D::GetViewMatrix() {
    return glm::lookAt(pos, pos + front, up);
}
