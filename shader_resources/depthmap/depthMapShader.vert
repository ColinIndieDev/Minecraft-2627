#version 450 core
layout (std430, binding = 0) readonly buffer vertexPullBuffer {
    int meshData[];
};

// Weird name in order for no naming collision in the geometry stage
out vec2 gTexCoords;
flat out int gTexID;

uniform mat4 transform;

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
    gTexCoords = uvs[face][indices[curVertexID]];
    gTexID = meshData[idx + 1] & 0xFF;

    gl_Position = transform * vec4(vertex, 1.0);
}
