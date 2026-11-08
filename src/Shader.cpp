#include "Shader.hpp"

#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>

#include <cstdint>

char *ReadFile(const std::string &path) {
    FILE *f = fopen(path.c_str(), "rb");
    if (!f) {
        return NULL;
    }

    fseek(f, 0, SEEK_END);
    uint32_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buffer = new char[size + 1];
    if (!buffer) {
        fclose(f);
        return NULL;
    }

    uint32_t read = fread(buffer, 1, size, f);
    fclose(f);

    if (read != size) {
        free(buffer);
        return NULL;
    }

    buffer[size] = '\0';
    return buffer;
}

bool CheckCompileErrors(uint32_t shader, const std::string &type) {
    int success = 0;
    char infoLog[1024] = {0};
    if (type != "PROGRAM") {
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            glGetShaderInfoLog(shader, 1024, NULL, infoLog);
            fprintf(stderr, "Shader compilation error: %s\n-> %s", type.c_str(), infoLog);
            return false;
        }
    } else {
        glGetProgramiv(shader, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(shader, 1024, NULL, infoLog);
            fprintf(stderr, "Program linking error of type: %s\n%s", type.c_str(), infoLog);
            return false;
        }
    }
    return true;
}

Shader::Shader(const std::string &vertPath, const std::string &fragPath) {
    const char *vShaderCode = ReadFile(vertPath);
    const char *fShaderCode = ReadFile(fragPath);

    uint32_t vertex = 0;
    uint32_t fragment = 0;
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    CheckCompileErrors(vertex, "VERTEX");
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);
    CheckCompileErrors(fragment, "FRAGMENT");
    id = glCreateProgram();
    glAttachShader(id, vertex);
    glAttachShader(id, fragment);
    glLinkProgram(id);
    CheckCompileErrors(id, "PROGRAM");

    delete[] vShaderCode;
    delete[] fShaderCode;
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::Use() const {
    glUseProgram(id);
}

void Shader::SetMatrix4(const std::string &name, const glm::mat4 &matrix) const {
    glUniformMatrix4fv(glGetUniformLocation(id, name.c_str()), 1, GL_FALSE, glm::value_ptr(matrix));
}

void Shader::SetVector2(const std::string &name, const glm::vec2 &vec2) const {
    glUniform2f(glGetUniformLocation(id, name.c_str()), vec2.x, vec2.y);
}

void Shader::SetVector3(const std::string &name, const glm::vec3 &vec3) const {
    glUniform3f(glGetUniformLocation(id, name.c_str()), vec3.x, vec3.y, vec3.z);
}

void Shader::SetVector4(const std::string &name, const glm::vec4 &vec4) const {
    glUniform4f(glGetUniformLocation(id, name.c_str()), vec4.x, vec4.y, vec4.z, vec4.w);
}

void Shader::SetColor(const std::string &name, const glm::vec4 &color) const {
    glUniform4f(glGetUniformLocation(id, name.c_str()), color.r / 255, color.g / 255, color.b / 255, color.a / 255);
}

void Shader::SetInt(const std::string &name, int i) const {
    glUniform1i(glGetUniformLocation(id, name.c_str()), i);
}
