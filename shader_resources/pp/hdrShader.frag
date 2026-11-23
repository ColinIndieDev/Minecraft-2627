#version 450 core
out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D hdrBuffer;
uniform sampler2D bloomBlur;
uniform bool hdr;
uniform bool bloom;
uniform float exposure;
uniform float bloomIntensity;

vec3 acesFilm(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;

    return clamp(
        (x * (a * x + b)) /
        (x * (c * x + d) + e),
        0.0,
        1.0
    );
}

void main() {
    vec3 color = texture(hdrBuffer, texCoords).rgb;
    if (bloom) {
        vec3 bloomColor = texture(bloomBlur, texCoords).rgb * bloomIntensity;
        color += bloomColor;
    }

    if (hdr) {
        color = acesFilm(color * exposure);
    }

    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));

    fragColor = vec4(color, 1.0);
}
