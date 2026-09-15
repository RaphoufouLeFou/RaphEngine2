#version 450 core

layout(location = 0) in vec2 aLocalPos;

uniform sampler2DArray heightRingArray;

uniform vec2 ringOrigin;
uniform float ringWorldSize;
uniform int ringLayer;
uniform float ringTexelCount;

uniform vec2 nextRingOrigin;
uniform float nextRingWorldSize;
uniform bool hasNextRing;

uniform vec2 innerRingOrigin;
uniform float innerRingWorldSize;
uniform bool hasInnerRing;

uniform mat4 view;
uniform mat4 projection;

out VS_OUT
{
    vec3 worldPos;
}
vs_out;

const float kMorphBandWidth = 0.15;
const float kInnerMarginTexels = 1.5;

float SampleExact(ivec2 texel)
{
    ivec2 clamped = clamp(texel, ivec2(0), ivec2(int(ringTexelCount)));
    return texelFetch(heightRingArray, ivec3(clamped, ringLayer), 0).r;
}

float SampleRingTriangulated(vec2 worldXY, vec2 otherRingOrigin,
                             float otherRingWorldSize, int otherRingLayer)
{
    float otherTexelSize = otherRingWorldSize / ringTexelCount;
    vec2 texelCoordF = clamp((worldXY - otherRingOrigin) / otherTexelSize,
                             vec2(0.0), vec2(ringTexelCount));

    ivec2 cell = clamp(ivec2(floor(texelCoordF)), ivec2(0),
                       ivec2(int(ringTexelCount) - 1));
    vec2 frac = texelCoordF - vec2(cell);

    float h00 = texelFetch(heightRingArray, ivec3(cell, otherRingLayer), 0).r;
    float h10 = texelFetch(heightRingArray,
                           ivec3(cell + ivec2(1, 0), otherRingLayer), 0)
                    .r;
    float h01 = texelFetch(heightRingArray,
                           ivec3(cell + ivec2(0, 1), otherRingLayer), 0)
                    .r;

    if (frac.x + frac.y <= 1.0)
    {
        return h00 * (1.0 - frac.x - frac.y) + h10 * frac.x + h01 * frac.y;
    }

    float h11 = texelFetch(heightRingArray,
                           ivec3(cell + ivec2(1, 1), otherRingLayer), 0)
                    .r;
    return h10 * (1.0 - frac.y) + h01 * (1.0 - frac.x)
        + h11 * (frac.x + frac.y - 1.0);
}

void main()
{
    float edgeDist = min(min(aLocalPos.x, 1.0 - aLocalPos.x),
                         min(aLocalPos.y, 1.0 - aLocalPos.y));
    float outerMorphFactor =
        hasNextRing ? (1.0 - smoothstep(0.0, kMorphBandWidth, edgeDist)) : 0.0;

    ivec2 texel = ivec2(round(aLocalPos * ringTexelCount));
    vec2 worldXY = ringOrigin + aLocalPos * ringWorldSize;

    float height = SampleExact(texel);
    if (hasNextRing && outerMorphFactor > 0.0)
    {
        float coarse = SampleRingTriangulated(worldXY, nextRingOrigin,
                                              nextRingWorldSize, ringLayer + 1);
        height = mix(height, coarse, outerMorphFactor);
    }

    if (hasInnerRing)
    {
        vec2 innerLocal = (worldXY - innerRingOrigin) / innerRingWorldSize;
        float signedDist = min(min(innerLocal.x, 1.0 - innerLocal.x),
                               min(innerLocal.y, 1.0 - innerLocal.y));

        float texelSize = ringWorldSize / ringTexelCount;
        float marginNorm =
            (texelSize * kInnerMarginTexels) / innerRingWorldSize;

        float innerMorphFactor = smoothstep(0.0, marginNorm, signedDist);
        if (innerMorphFactor > 0.0)
        {
            float fine = SampleRingTriangulated(
                worldXY, innerRingOrigin, innerRingWorldSize, ringLayer - 1);
            height = mix(height, fine, innerMorphFactor);
        }
    }

    vec3 worldPos = vec3(worldXY, height);
    vs_out.worldPos = worldPos;

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
