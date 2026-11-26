#version 450 core
out vec4 fragColor;

in vec2 texCoords;

uniform sampler2D hdrBuffer;
uniform sampler2D bloomBlur;
uniform sampler2D occlusionBuffer;
uniform bool hdr;
uniform bool bloom;
uniform float exposure;
uniform float bloomIntensity;
// God rays
uniform bool godRays;
uniform vec3 cameraPos;
uniform vec3 lightDirection;
uniform mat4 projection;
uniform mat4 view;
uniform int samples;
uniform float decay;
uniform float density;
uniform float weight;
uniform float godRayExposure;

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

// I know it is risky when i change sth in sunShader, this will not be changed but cannot care less
#define DISTANCE 500
vec3 compute_god_ray() {
    vec3 toSun = normalize(-lightDirection);
    vec3 sunCenter = cameraPos + toSun * DISTANCE;
    vec4 clip = projection * view * vec4(sunCenter, 1.0);
    vec2 ndc = clip.xy / clip.w;
    vec2 lightScreenPos = (ndc + 1.0) * 0.5;

    vec2 localTexCoords = texCoords;
    vec2 direction = texCoords - lightScreenPos;
    direction *= 1.0 / float(samples) * density;

    float illuminationDecay = 1.0;
    vec3 result = texture(occlusionBuffer, texCoords).rgb;
    for (int i = 0; i < samples; ++i) {
        localTexCoords -= direction;
        vec3 texSample = texture(occlusionBuffer, localTexCoords).rgb;
        texSample *= illuminationDecay * weight;
        result += texSample;
        illuminationDecay *= decay;
    }
    return result * godRayExposure;
}

void main() {
    vec3 color = texture(hdrBuffer, texCoords).rgb;
    if (bloom) {
        vec3 bloomColor = texture(bloomBlur, texCoords).rgb * bloomIntensity;
        color += bloomColor;
    }
    if (godRays) {
        vec3 godRayColor = compute_god_ray();
        color += godRayColor;
    }

    if (hdr) {
        color = acesFilm(color * exposure);
    }

    color = pow(max(color, vec3(0.0)), vec3(1.0 / 2.2));

    fragColor = vec4(color, 1.0);
}
