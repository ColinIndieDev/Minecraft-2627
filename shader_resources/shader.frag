#version 450 core
out vec4 fragColor;

in vec2 texCoords;
flat in int texID;
in vec3 fragPos;
in vec3 normal;
in vec3 eyePos;

struct DirectionalLight {
    vec3 direction;

    vec3 ambient;
    vec3 diffuse;
    vec3 specular;
};

uniform sampler2DArray tex;
uniform vec4 inputColor;
uniform vec3 viewPos;
uniform int lod;
uniform DirectionalLight directionalLight;

vec4 grassColor = vec4(51.0 / 255.0, 171.0 / 255.0, 65.0 / 255.0, 1.0);
vec4 leavesColor = vec4(20.0 / 255.0, 156.0 / 255.0, 45.0 / 255.0, 1.0);
vec4 waterColor = vec4(12.0 / 255.0, 155.0 / 255.0, 220.0 / 255.0, 0.8);

#define BLOCK_TEXTURE_GRASS_TOP         0
#define BLOCK_TEXTURE_GRASS_SIDE        1
#define BLOCK_TEXTURE_OAK_LEAVES_SIDE   6
#define BLOCK_TEXTURE_WATER             8

#define VIEW_DISTANCE 16
#define MAX_CHUNK_WIDTH 16

#define FOG_START (VIEW_DISTANCE - 2) * MAX_CHUNK_WIDTH
#define FOG_END VIEW_DISTANCE * MAX_CHUNK_WIDTH

void main() {
    vec4 color;
    int applySpecular = 0; // false
    float specularStrength = 128.0;
    float specularIntensity = 5.0;

    if (lod == 1) {
        // LOD Chunk uses grass block texture
        color = grassColor * texture(tex, vec3(0, 0, BLOCK_TEXTURE_GRASS_TOP));
    } else {
        // Apply green color if texture is the grass top block texture
        if (texID == BLOCK_TEXTURE_GRASS_TOP) {
            color = grassColor * texture(tex, vec3(texCoords, texID));
        } else if (texID == BLOCK_TEXTURE_GRASS_SIDE) {
            color = texture(tex, vec3(texCoords, texID));
        } else if (texID == BLOCK_TEXTURE_OAK_LEAVES_SIDE) {
            color = leavesColor * texture(tex, vec3(texCoords, texID));
        } else if (texID == BLOCK_TEXTURE_WATER) {
            color = waterColor * texture(tex, vec3(texCoords, texID));
            applySpecular = 1;
        } else {
            color = texture(tex, vec3(texCoords, texID));
        }
    }
    if (color.a < 0.1) {
        discard;
    }

    vec3 ambient = directionalLight.ambient * color.rgb;
    vec3 norm = normalize(normal);
    vec3 lightDir = normalize(-directionalLight.direction);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = directionalLight.diffuse * diff * color.rgb;
    vec3 viewDir = normalize(viewPos - fragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), specularStrength);
    vec3 specular = applySpecular * specularIntensity * directionalLight.specular * spec;

    vec3 result = ambient + diffuse + specular;
    result = pow(result, vec3(1.0 / 2.2));

    float distance = length(eyePos);
    float fogFactor = clamp((distance - FOG_START) / (FOG_END - FOG_START), 0.0, 1.0);
    fogFactor = 1.0 - fogFactor;

    fragColor = vec4(result, color.a * fogFactor);
}
