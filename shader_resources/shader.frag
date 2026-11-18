#version 450 core
out vec4 fragColor;

in vec2 texCoords;
flat in int texID;
in vec3 fragPos;
in vec3 eyePos;
in vec3 T;
in vec3 B;
in vec3 N;

struct DirectionalLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform sampler2DArray tex;
uniform sampler2DArray shadowMap;
uniform sampler2DArray texNormal;
uniform vec4 inputColor;
uniform vec3 viewPos;
uniform bool shadows;
uniform bool fog;
uniform bool normalMapping;
uniform int fogStartBlocks;
uniform int fogEndBlocks;
uniform float normalStrength;
uniform float farPlane;
uniform mat4 view;
uniform DirectionalLight directionalLight;
layout (std140, binding = 0) uniform LightSpaceMatrices {
    mat4 lightSpaceMatrices[16];
};
uniform float cascadePlaneDistances[16];
uniform int cascadeCount;

vec4 grassColor = vec4(51.0 / 255.0, 171.0 / 255.0, 65.0 / 255.0, 1.0);
vec4 leavesColor = vec4(20.0 / 255.0, 156.0 / 255.0, 45.0 / 255.0, 1.0);
vec4 waterColor = vec4(12.0 / 255.0, 155.0 / 255.0, 220.0 / 255.0, 0.8);

#define BLOCK_TEXTURE_GRASS_TOP         0
#define BLOCK_TEXTURE_GRASS_SIDE        1
#define BLOCK_TEXTURE_OAK_LEAVES_SIDE   6
#define BLOCK_TEXTURE_WATER             8

float ShadowCalculation(vec3 lightDir) {
    if (!shadows) return 0.0;

    vec4 fragPosViewSpace = view * vec4(fragPos, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = -1;
    for (int i = 0; i < cascadeCount; ++i) {
        if (depthValue < cascadePlaneDistances[i]) {
            layer = i;
            break;
        }
    }
    if (layer == -1) layer = cascadeCount;

    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(fragPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    if (projCoords.z > 1.0 ||
            projCoords.z < 0.0 ||
            projCoords.x < 0.0 ||
            projCoords.x > 1.0 ||
            projCoords.y < 0.0 ||
            projCoords.y > 1.0) {
        return 0.0;
    }

    float currentDepth = projCoords.z;

    if (currentDepth > 1.0) {
        return 0.0;
    }
    
    vec3 norm = normalize(N);
    float bias = max(0.0005 * (1.0 - dot(norm, lightDir)), 0.0005);
    const float biasModifier = 0.0005;
    if (layer == cascadeCount) {
        bias *= 1 / (farPlane * biasModifier);
    } else {
        bias *= 1 / (cascadePlaneDistances[layer] * biasModifier);
    }

    float shadow = 0.0;
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for(int x = -1; x <= 1; ++x) {
        for(int y = -1; y <= 1; ++y) {
            float pcfDepth = texture(shadowMap, vec3(projCoords.xy + vec2(x, y) * texelSize, layer)).r;
            shadow += (currentDepth - bias) > pcfDepth ? 1.0 : 0.0;
        }
    }
    shadow /= 9.0;

    return shadow;
}

void main() {
    vec4 color;
    bool applySpecular = false;
    float specularStrength = 128.0;
    float specularIntensity = 5.0;
    // Possible this is not a transformation matrix but it works
    mat3 TBN = mat3(
        0.0, 0.0, 1.0,
        0.0, 1.0, 0.0,
        1.0, 0.0, 0.0
    );
    if (normalMapping) {
        TBN = transpose(mat3(T, B, N));
    }

    // Apply green color if texture is the grass top block texture
    if (texID == BLOCK_TEXTURE_GRASS_TOP) {
        color = grassColor * texture(tex, vec3(texCoords, texID));
    } else if (texID == BLOCK_TEXTURE_GRASS_SIDE) {
        color = texture(tex, vec3(texCoords, texID));
    } else if (texID == BLOCK_TEXTURE_OAK_LEAVES_SIDE) {
        color = leavesColor * texture(tex, vec3(texCoords, texID));
    } else if (texID == BLOCK_TEXTURE_WATER) {
        color = waterColor * texture(tex, vec3(texCoords, texID));
        applySpecular = true;
    } else {
        color = texture(tex, vec3(texCoords, texID));
    }

    if (color.a < 0.1) {
        discard;
    }

    vec3 ambient = directionalLight.ambient * color.rgb;
    vec3 norm = vec3(0.0);
    if (normalMapping) {
        // Normal textures are LabPBR which has to be "encrypted" like this
        vec2 normalXY = texture(texNormal, vec3(texCoords, texID)).rg;
        normalXY = normalXY * 2.0 - 1.0;
        normalXY *= normalStrength;
        float normalZ = sqrt(max(0.0, 1.0 - dot(normalXY, normalXY)));
        norm = normalize(vec3(normalXY, normalZ));
    } else {
        norm = normalize(N);
    }
    vec3 lightDir = normalize(-directionalLight.direction);

    vec3 tangentViewPos  = TBN * viewPos;
    vec3 tangentFragPos  = TBN * fragPos;
    vec3 tangentLightDir = normalMapping ? normalize(TBN * lightDir) : lightDir;

    float diff = max(dot(norm, tangentLightDir), 0.0);
    vec3 diffuse = directionalLight.diffuse * diff * color.rgb;
    vec3 viewDir = normalize(tangentViewPos - tangentFragPos);
    vec3 reflectDir = reflect(-tangentLightDir, norm);
    vec3 halfwayDir = normalize(tangentLightDir + viewDir);
    float spec = pow(max(dot(norm, halfwayDir), 0.0), specularStrength);
    vec3 specular = int(applySpecular) * specularIntensity * directionalLight.specular * spec;

    float shadow = ShadowCalculation(lightDir);
    vec3 result = ambient + (1.0 - shadow) * (diffuse + specular);
    result = pow(result, vec3(1.0 / 2.2));

    float fogFactor = 1.0;
    if (fog) {
        float distance = length(eyePos);
        fogFactor = clamp((distance - fogStartBlocks) / (fogEndBlocks - fogStartBlocks), 0.0, 1.0);
        fogFactor = 1.0 - fogFactor;
    }

    fragColor = vec4(result, color.a * fogFactor);
}
