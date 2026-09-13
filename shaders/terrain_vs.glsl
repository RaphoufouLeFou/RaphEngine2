#version 450 core

layout(location = 0) in vec2 aLocalPos;

struct ChunkInstance
{
    vec2 worldOrigin;
    vec2 heightRange;
    uint textureLayer;
    float pad0;
};

layout(std430, binding = 0) restrict readonly buffer ChunkInstances
{
    ChunkInstance instances[];
};

uniform float chunkResolution;

out VS_OUT
{
    vec2 worldPosXY;
    vec2 heightMapUV;
    flat uint textureLayer;
    vec2 heightRange;
}
vs_out;

void main()
{
    ChunkInstance instance = instances[gl_InstanceID];

    vs_out.worldPosXY = instance.worldOrigin + aLocalPos * chunkResolution;
    vs_out.heightMapUV = aLocalPos;
    vs_out.textureLayer = instance.textureLayer;
    vs_out.heightRange = instance.heightRange;

    gl_Position = vec4(vs_out.worldPosXY, 0.0, 1.0);
}
