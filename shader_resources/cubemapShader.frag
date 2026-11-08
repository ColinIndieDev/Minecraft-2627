#version 450 core
out vec4 fragColor;

in vec3 texCoords;

uniform samplerCube cubemap;
uniform vec4 inputColor;

void main() {
    fragColor = texture(cubemap, texCoords) * inputColor;
}

