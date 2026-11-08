#version 450 core
layout (std430, binding = 0) readonly buffer vertexPullBuffer {
    int meshData[];
};

out vec2 texCoords;
flat out int texID;
out vec3 fragPos;
out vec3 normal;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 transform;

#define FACE_FRONT  0
#define FACE_BACK   1
#define FACE_TOP    2
#define FACE_BOTTOM 3
#define FACE_RIGHT  4
#define FACE_LEFT   5

const vec3 offsets[6][4] = {
    {
        vec3(0.0, 0.0, 0.0), 
        vec3(1.0, 0.0, 0.0), 
        vec3(1.0, 1.0, 0.0), 
        vec3(0.0, 1.0, 0.0)  
    },
    {
        vec3(0.0, 0.0, 0.0), 
        vec3(-1.0, 0.0, 0.0),
        vec3(-1.0, 1.0, 0.0),
        vec3(0.0, 1.0, 0.0)  
    },
    {
        vec3(0.0, 0.0, 0.0), 
        vec3(0.0, 0.0, 1.0), 
        vec3(1.0, 0.0, 1.0), 
        vec3(1.0, 0.0, 0.0)  
    },
    {
        vec3(0.0, 0.0, 0.0), 
        vec3(1.0, 0.0, 0.0), 
        vec3(1.0, 0.0, 1.0), 
        vec3(0.0, 0.0, 1.0)  
    },
    {
        vec3(0.0, 0.0, 0.0), 
        vec3(0.0, 0.0, -1.0),
        vec3(0.0, 1.0, -1.0),
        vec3(0.0, 1.0, 0.0)  
    },
    {
        vec3(0.0, 0.0, 0.0), 
        vec3(0.0, 0.0, 1.0), 
        vec3(0.0, 1.0, 1.0), 
        vec3(0.0, 1.0, 0.0)  
    }
};

const vec2 uvs[6][4] = {
    {
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0) 
    },
    {
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0) 
    },
    {
        vec2(0.0, 0.0),
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0) 
    },
    {
        vec2(0.0, 0.0),
        vec2(1.0, 1.0),
        vec2(0.0, 1.0),
        vec2(1.0, 0.0) 
    },
    {
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0) 
    },
    {
        vec2(0.0, 1.0),
        vec2(1.0, 1.0),
        vec2(1.0, 0.0),
        vec2(0.0, 0.0) 
    }
};

const vec3 normals[6] = {
    vec3(0, 0, 1), 
    vec3(0, 0, -1),
    vec3(0, 1, 0),
    vec3(0, -1, 0),
    vec3(1, 0, 0),
    vec3(-1, 0, 0)
};

const int indices[6] = int[](0, 1, 2, 0, 2, 3);

void main() {
    const int idx = gl_VertexID / 6 * 2;
    vec3 vertex = vec3(0);
    vertex.x = meshData[idx] & 1023;
    vertex.y = (meshData[idx] >> 10) & 1023;
    vertex.z = (meshData[idx] >> 20) & 1023;
    const int curVertexID = gl_VertexID % 6;
    int face = (meshData[idx + 1] >> 8) & 0xFF;
    vertex += offsets[face][indices[curVertexID]];

    texCoords = uvs[face][indices[curVertexID]];
    texID = meshData[idx + 1] & 0xFF;

    fragPos = vec3(transform * vec4(vertex, 1.0));
    normal = mat3(transpose(inverse(transform))) * normals[face];
    gl_Position = projection * view * vec4(fragPos, 1.0);
}
