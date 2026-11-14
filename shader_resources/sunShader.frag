#version 450 core
out vec4 fragColor;

in vec2 texCoords;

vec3 sunColor = vec3(1.0, 1.0, 0.3);

void main() {
    float d = length(texCoords - 0.5) * 2.0;
    float glow = smoothstep(1.0, 0.0, d);
    float core = smoothstep(0.3, 0.0, d);
    vec3 color = sunColor * (glow * 0.5 + core * 1.5);
    float alpha = glow;
    if (alpha < 0.01) discard;
    fragColor = vec4(color, alpha);
}
