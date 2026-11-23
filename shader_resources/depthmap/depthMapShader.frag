#version 450 core

in vec2 texCoords;
flat in int texID;

uniform sampler2DArray tex;

#define BLOCK_TEXTURE_OAK_LEAVES_SIDE    6
#define BLOCK_TEXTURE_WATER              8
#define BLOCK_TEXTURE_SPRUCE_LEAVES_SIDE 12

void main() {
    if (texID == BLOCK_TEXTURE_OAK_LEAVES_SIDE || texID == BLOCK_TEXTURE_SPRUCE_LEAVES_SIDE) {
        if (texture(tex, vec3(texCoords, texID)).a < 0.1) discard;
    } else if (texID == BLOCK_TEXTURE_WATER) {
        discard;
    }
    gl_FragDepth = gl_FragCoord.z;
}
