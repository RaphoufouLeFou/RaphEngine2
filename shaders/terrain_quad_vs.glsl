#version 450 core

layout(location = 0) in vec4 aVertex; // xy = local UV position, zw = outward
                                      // skirt direction (0,0 for top surface)

uniform sampler2DArray heightNodeArray;

uniform vec2 nodeOrigin;
uniform float nodeWorldSize;
uniform int nodeLayer;
uniform float nodeTexelCount;

uniform mat4 view;
uniform mat4 projection;

out VS_OUT
{
    vec3 worldPos;
}
vs_out;

const float kSkirtOutwardTexels = 0.5;
const float kSkirtAngleDegrees = 12.5;

float SampleHeight(ivec2 texel)
{
    ivec2 clamped = clamp(texel, ivec2(0), ivec2(int(nodeTexelCount)));
    return texelFetch(heightNodeArray, ivec3(clamped, nodeLayer), 0).r;
}

void main()
{
    vec2 localPos = aVertex.xy;
    vec2 skirtDir = aVertex.zw;

    ivec2 texel = ivec2(round(localPos * nodeTexelCount));
    float height = SampleHeight(texel);

    float texelSize = nodeWorldSize / nodeTexelCount;
    bool isSkirt = dot(skirtDir, skirtDir) > 0.5;
    float outwardOffset = kSkirtOutwardTexels * texelSize;
    float dropAmount = outwardOffset / tan(radians(kSkirtAngleDegrees));

    vec2 worldXY =
        nodeOrigin + localPos * nodeWorldSize + skirtDir * outwardOffset;
    float worldHeight = height - (isSkirt ? dropAmount : 0.0);

    vec3 worldPos = vec3(worldXY, worldHeight);
    vs_out.worldPos = worldPos;

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
