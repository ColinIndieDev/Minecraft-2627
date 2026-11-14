#version 450 core
layout (location = 0) in vec2 aPos;

out vec2 texCoords;

uniform mat4 view;
uniform mat4 projection;
uniform vec3 cameraPos;
uniform vec3 lightDirection;

#define DISTANCE 500
#define SIZE 500

void main() {
    vec3 toSun = normalize(-lightDirection);

    vec3 sunCenter = cameraPos + toSun * DISTANCE;

    vec3 camRight = vec3(view[0][0], view[1][0], view[2][0]);
    vec3 camUp = vec3(view[0][1], view[1][1], view[2][1]);

    vec3 worldPos = sunCenter + camRight * aPos.x * SIZE + camUp * aPos.y * SIZE;

    texCoords = aPos + 0.5;
    gl_Position = projection * view * vec4(worldPos, 1.0);
}
