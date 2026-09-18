#version 450 core

layout(location = 0) in vec4 aVertex;
layout(location = 1) in vec4 aInstanceData;
layout(location = 2) in vec4 aEdgeFlags;

uniform sampler2DArray heightNodeArray;
uniform float nodeTexelCount;
uniform float skirtDropMeters;
uniform float skirtOutwardMeters;
uniform mat4 lightSpaceMatrix;

float SampleHeight(ivec2 texel, int layer)
{
    ivec2 clamped = clamp(texel, ivec2(0), ivec2(int(nodeTexelCount)));
    return texelFetch(heightNodeArray, ivec3(clamped, layer), 0).r;
}

void main()
{
    vec2 nodeOrigin = aInstanceData.xy;
    float nodeWorldSize = aInstanceData.z;
    int nodeLayer = int(aInstanceData.w + 0.5);

    vec2 localPos = aVertex.xy;
    vec2 skirtDir = aVertex.zw;

    ivec2 texel = ivec2(round(localPos * nodeTexelCount));
    float height = SampleHeight(texel, nodeLayer);

    bool isSkirtGeometry = dot(skirtDir, skirtDir) > 0.5;

    float edgeSuppressed = 0.0;
    if (skirtDir.x < -0.5)
        edgeSuppressed = aEdgeFlags.x;
    else if (skirtDir.x > 0.5)
        edgeSuppressed = aEdgeFlags.y;
    else if (skirtDir.y < -0.5)
        edgeSuppressed = aEdgeFlags.z;
    else if (skirtDir.y > 0.5)
        edgeSuppressed = aEdgeFlags.w;

    bool isSkirt = isSkirtGeometry && edgeSuppressed < 0.5;

    float outwardOffset = isSkirt ? skirtOutwardMeters : 0.0;
    float dropAmount = isSkirt ? skirtDropMeters : 0.0;

    vec2 worldXY =
        nodeOrigin + localPos * nodeWorldSize + skirtDir * outwardOffset;
    float worldHeight = height - dropAmount;

    gl_Position = lightSpaceMatrix * vec4(worldXY, worldHeight, 1.0);
}
