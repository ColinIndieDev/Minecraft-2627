#pragma once

#include <glm/glm.hpp>

struct Plane {
    glm::vec3 normal = glm::vec3(0);
    float dist = 0;

    float GetDistance(const glm::vec3 &point) const;
};

struct Frustum {
    enum Side {
        LEFT = 0,
        RIGHT,
        BOTTOM,
        TOP,
        NEAR,
        FAR
    };

    Plane planes[6];

    void Update(const glm::mat4 &viewProjection);

    bool IsCubeVisible(const glm::vec3 &center, const glm::vec3 &size) const;
};
