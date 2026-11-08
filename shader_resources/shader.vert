#version 450 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoords;
layout (location = 3) in float aTexID;

out vec2 texCoords;
flat out int texID;
out vec3 fragPos;
out vec3 normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 transform;

void main() {
    texCoords = aTexCoords;
    texID = int(aTexID);
    fragPos = vec3(transform * vec4(aPos, 1.0));
    normal = mat3(transpose(inverse(transform))) * aNormal;
    gl_Position = projection * view * vec4(fragPos, 1.0);
}
