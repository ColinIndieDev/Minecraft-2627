#version 450 core
out vec4 fragColor;

in vec3 texCoords;

uniform samplerCube cubemap;
uniform vec4 inputColor;

void main() {
    vec4 color = texture(cubemap, texCoords) * inputColor;
    color = pow(color, vec4(1.0 / 2.2));
    fragColor = vec4(color.rgb, 1.0);
}

