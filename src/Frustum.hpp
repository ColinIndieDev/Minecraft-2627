#pragma once

#include <glm/glm.hpp>

struct Plane {
    glm::vec3 normal = glm::vec3(0);
    float dist = 0;

    float GetDistance(const glm::vec3 &point) const {
        return glm::dot(normal, point) + dist;
    }
};

struct Frustum {
    enum class Side : uint8_t {
        LEFT = 0,
        RIGHT,
        BOTTOM,
        TOP,
        NEAR,
        FAR
    };

    Plane planes[6];

    void Update(const glm::mat4 &viewProjection) {
        glm::mat4 vp = glm::transpose(viewProjection);

        planes[(int)(Side::LEFT)].normal   = glm::vec3(vp[3] + vp[0]);
        planes[(int)(Side::LEFT)].dist     = vp[3][3] + vp[0][3];

        planes[(int)(Side::RIGHT)].normal  = glm::vec3(vp[3] - vp[0]);
        planes[(int)(Side::RIGHT)].dist    = vp[3][3] - vp[0][3];

        planes[(int)(Side::BOTTOM)].normal = glm::vec3(vp[3] + vp[1]);
        planes[(int)(Side::BOTTOM)].dist   = vp[3][3] + vp[1][3];

        planes[(int)(Side::TOP)].normal    = glm::vec3(vp[3] - vp[1]);
        planes[(int)(Side::TOP)].dist      = vp[3][3] - vp[1][3];

        planes[(int)(Side::NEAR)].normal   = glm::vec3(vp[3] + vp[2]);
        planes[(int)(Side::NEAR)].dist     = vp[3][3] + vp[2][3];

        planes[(int)(Side::FAR)].normal    = glm::vec3(vp[3] - vp[2]);
        planes[(int)(Side::FAR)].dist      = vp[3][3] - vp[2][3];

        for (auto &plane : planes) {
            float length = glm::length(plane.normal);
            plane.normal /= length;
            plane.dist /= length;
        }
    }

    bool IsCubeVisible(const glm::vec3 &center, const glm::vec3 &size) const {
        glm::vec3 halfSize = size * 0.5f;
        for (auto &plane : planes) {
            /*
            float radius = (halfSize.x * std::abs(plane.normal.x)) +
                           (halfSize.y * std::abs(plane.normal.y)) +
                           (halfSize.z * std::abs(plane.normal.z));
            */

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
};
