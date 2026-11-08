#pragma once

#include <glm/glm.hpp>

#include <string>

struct Shader {
    uint32_t id = 0;

    Shader(const std::string &vertPath, const std::string &fragPath);
    void Use() const;
    void SetMatrix4(const std::string &name, const glm::mat4 &matrix) const;
    void SetVector2(const std::string &name, const glm::vec2 &vec2)   const;
    void SetVector3(const std::string &name, const glm::vec3 &vec3)   const;
    void SetVector4(const std::string &name, const glm::vec4 &vec4)   const;
    void SetColor  (const std::string &name, const glm::vec4 &color)  const;
    void SetInt    (const std::string &name, int i)                   const;
};
