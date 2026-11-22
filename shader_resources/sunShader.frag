#version 450 core
layout (location = 0) out vec4 fragColor;
layout (location = 1) out vec4 brightColor;

in vec2 texCoords;

uniform vec3 sunColor;

void main() {
    float d = length(texCoords - 0.5) * 2.0;
    float glow = smoothstep(1.0, 0.0, d);
    float core = smoothstep(0.3, 0.0, d);
    vec3 color = sunColor * (glow * 0.5 + core * 1.5);
    float alpha = glow;

    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > 1.0) {
        brightColor = vec4(color, alpha);
    } else {
        brightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }

    if (alpha < 0.01) discard;
    fragColor = vec4(color, alpha);
}
