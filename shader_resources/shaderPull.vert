#version 450 core
layout (std430, binding = 0) readonly buffer vertexPullBuffer {
    int meshData[];
};

out vec2 texCoords;
flat out int texID;
out vec3 fragPos;
out vec3 eyePos;
out vec3 T;
out vec3 B;
out vec3 N;

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

    vec3 tangent1, tangent2;
    vec3 bitangent1, bitangent2;

    // Triangle 1
    vec3 edge1 = offsets[face][1] - offsets[face][0];
    vec3 edge2 = offsets[face][2] - offsets[face][0];
    vec2 deltaUV1 = uvs[face][1] - uvs[face][0];
    vec2 deltaUV2 = uvs[face][2] - uvs[face][0];
    float f = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    tangent1.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent1.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent1.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
    bitangent1.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent1.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent1.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

    // Triangle 2
    edge1 = offsets[face][2] - offsets[face][0];
    edge2 = offsets[face][3] - offsets[face][0];
    deltaUV1 = uvs[face][2] - uvs[face][0];
    deltaUV2 = uvs[face][3] - uvs[face][0];
    f = 1.0 / (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);

    tangent2.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
    tangent2.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
    tangent2.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);

    bitangent2.x = f * (-deltaUV2.x * edge1.x + deltaUV1.x * edge2.x);
    bitangent2.y = f * (-deltaUV2.x * edge1.y + deltaUV1.x * edge2.y);
    bitangent2.z = f * (-deltaUV2.x * edge1.z + deltaUV1.x * edge2.z);

    bool isTriangle1 = curVertexID < 3;
    vec3 tangent = isTriangle1 ? tangent1 : tangent2;
    vec3 bitangent = isTriangle1 ? bitangent1 : bitangent2;

    mat3 modelMatrix = mat3(transform);
    mat3 normalMatrix = transpose(inverse(modelMatrix));
    N = normalize(normalMatrix * normals[face]);
    T = normalize(modelMatrix * tangent);
    T = normalize(T - dot(T, N) * N);
    vec3 transformedBitangent = normalize(modelMatrix * bitangent);
    float handedness = sign(dot(cross(N, T), transformedBitangent));
    B = handedness * cross(N, T);

    fragPos = vec3(transform * vec4(vertex, 1.0));
    eyePos = vec4(view * vec4(fragPos, 1.0)).xyz;
    gl_Position = projection * view * vec4(fragPos, 1.0);
}
