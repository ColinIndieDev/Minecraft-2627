#include "Frustum.hpp"

float Plane::GetDistance(const glm::vec3 &point) const {
    return glm::dot(normal, point) + dist;
}

void Frustum::Update(const glm::mat4 &viewProjection) {
    glm::mat4 vp = glm::transpose(viewProjection);

    planes[LEFT].normal   = glm::vec3(vp[3] + vp[0]);
    planes[LEFT].dist     = vp[3][3] + vp[0][3];

    planes[RIGHT].normal  = glm::vec3(vp[3] - vp[0]);
    planes[RIGHT].dist    = vp[3][3] - vp[0][3];

    planes[BOTTOM].normal = glm::vec3(vp[3] + vp[1]);
    planes[BOTTOM].dist   = vp[3][3] + vp[1][3];

    planes[TOP].normal    = glm::vec3(vp[3] - vp[1]);
    planes[TOP].dist      = vp[3][3] - vp[1][3];

    planes[NEAR].normal   = glm::vec3(vp[3] + vp[2]);
    planes[NEAR].dist     = vp[3][3] + vp[2][3];

    planes[FAR].normal    = glm::vec3(vp[3] - vp[2]);
    planes[FAR].dist      = vp[3][3] - vp[2][3];

    for (auto &plane : planes) {
        float length = glm::length(plane.normal);
        plane.normal /= length;
        plane.dist /= length;
    }
}

bool Frustum::IsCubeVisible(const glm::vec3 &center, const glm::vec3 &size) const {
    glm::vec3 halfSize = size * 0.5f;
    for (auto &plane : planes) {
        glm::vec3 pVertex = center;

        pVertex.x += (plane.normal.x >= 0.0f) ? halfSize.x : -halfSize.x;
        pVertex.y += (plane.normal.y >= 0.0f) ? halfSize.y : -halfSize.y;
        pVertex.z += (plane.normal.z >= 0.0f) ? halfSize.z : -halfSize.z;

        if (plane.GetDistance(pVertex) < 0) {
            return false;
        }
    }
    return true;
}
