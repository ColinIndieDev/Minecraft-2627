#version 330 core
out vec4 fragColor;

in vec2 texCoords;
flat in int texID;
in vec3 fragPos;
in vec3 normal;

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

// vec4 grassColor = vec4(90.0 / 255.0, 255 / 255.0, 110.0 / 255.0, 1.0);
vec4 grassColor = vec4(51.0 / 255.0, 171.0 / 255.0, 65.0 / 255.0, 1.0);
vec4 leavesColor = vec4(20.0 / 255.0, 156.0 / 255.0, 45.0 / 255.0, 1.0);

#define BLOCK_TEXTURE_GRASS_TOP         0
#define BLOCK_TEXTURE_GRASS_SIDE        1
#define BLOCK_TEXTURE_OAK_LEAVES_SIDE   6

void main() {
    vec4 color;
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
    const float specularStrength = 16.0;
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), specularStrength);
    vec3 specular = directionalLight.specular * spec;

    vec3 result = ambient + diffuse + specular;
    fragColor = vec4(result, 1.0);
}
