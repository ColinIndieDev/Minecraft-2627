#version 450 core
out vec4 fragColor;

in vec2 texCoords;
flat in int texID;

uniform sampler2DArray tex;

#define BLOCK_TEXTURE_OAK_LEAVES_SIDE    6
#define BLOCK_TEXTURE_SPRUCE_LEAVES_SIDE 12

void main() {
    vec4 color = vec4(0.0, 0.0, 0.0, 1.0);
    if (texID == BLOCK_TEXTURE_OAK_LEAVES_SIDE || texID == BLOCK_TEXTURE_SPRUCE_LEAVES_SIDE) {
        color = texture(tex, vec3(texCoords, texID));
    } 

    if (color.a < 0.1) {
        discard;
    }
    fragColor = vec4(0.0, 0.0, 0.0, 1.0);
}
