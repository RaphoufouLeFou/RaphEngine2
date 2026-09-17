#version 450 core

layout(location = 0) in vec4 aVertex;
layout(location = 1) in vec4 aInstanceData;
uniform sampler2DArray heightNodeArray;
uniform float nodeTexelCount;
uniform mat4 lightSpaceMatrix;

const float kSkirtOutwardTexels = 0.5;
const float kSkirtAngleDegrees = 12.5;

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

    float texelSize = nodeWorldSize / nodeTexelCount;
    bool isSkirt = dot(skirtDir, skirtDir) > 0.5;
    float outwardOffset = kSkirtOutwardTexels * texelSize;
    float dropAmount = outwardOffset / tan(radians(kSkirtAngleDegrees));

    vec2 worldXY =
        nodeOrigin + localPos * nodeWorldSize + skirtDir * outwardOffset;
    float worldHeight = height - (isSkirt ? dropAmount : 0.0);

    gl_Position = lightSpaceMatrix * vec4(worldXY, worldHeight, 1.0);
}
