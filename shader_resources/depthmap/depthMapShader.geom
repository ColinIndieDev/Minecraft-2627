#version 450 core

layout (triangles, invocations = 5) in;
layout (triangle_strip, max_vertices = 3) out;

in vec2 gTexCoords[];
flat in int gTexID[];

out vec2 texCoords;
flat out int texID;

layout (std140, binding = 0) uniform LightSpaceMatricesUniform {
    mat4 lightSpaceMatrices[16];
};

void main() {
    for (int i = 0; i < 3; ++i) {
        gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        texCoords = gTexCoords[i];
        texID = gTexID[i];
        EmitVertex();
    }
    EndPrimitive();
}
