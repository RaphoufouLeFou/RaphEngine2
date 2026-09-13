#version 450 core

layout(vertices = 4) out;

in VS_OUT
{
    vec2 worldPosXY;
    vec2 heightMapUV;
    flat uint textureLayer;
    vec2 heightRange;
}
tcs_in[];

out TCS_OUT
{
    vec2 worldPosXY;
    vec2 heightMapUV;
    flat uint textureLayer;
    vec2 heightRange;
}
tcs_out[];

uniform vec3 viewPos;

const float kMinTessDistance = 16.0;
const float kMaxTessDistance = 1000.0;
const float kMinTessLevel = 1.0;
const float kMaxTessLevel = 32.0;

float TessLevelFromWorldPos(vec2 worldPosXY)
{
    vec3 worldPos = vec3(worldPosXY, 0.0);
    float dist = distance(viewPos, worldPos);
    float t = 1.0 - smoothstep(kMinTessDistance, kMaxTessDistance, dist);
    return mix(kMinTessLevel, kMaxTessLevel, t);
}

void main()
{
    tcs_out[gl_InvocationID].worldPosXY = tcs_in[gl_InvocationID].worldPosXY;
    tcs_out[gl_InvocationID].heightMapUV = tcs_in[gl_InvocationID].heightMapUV;
    tcs_out[gl_InvocationID].textureLayer =
        tcs_in[gl_InvocationID].textureLayer;
    tcs_out[gl_InvocationID].heightRange = tcs_in[gl_InvocationID].heightRange;

    if (gl_InvocationID == 0)
    {
        vec2 p0 = tcs_in[0].worldPosXY;
        vec2 p1 = tcs_in[1].worldPosXY;
        vec2 p2 = tcs_in[2].worldPosXY;
        vec2 p3 = tcs_in[3].worldPosXY;

        float eLeft = TessLevelFromWorldPos(mix(p0, p3, 0.5));
        float eBottom = TessLevelFromWorldPos(mix(p0, p1, 0.5));
        float eRight = TessLevelFromWorldPos(mix(p1, p2, 0.5));
        float eTop = TessLevelFromWorldPos(mix(p2, p3, 0.5));

        gl_TessLevelOuter[0] = eLeft;
        gl_TessLevelOuter[1] = eBottom;
        gl_TessLevelOuter[2] = eRight;
        gl_TessLevelOuter[3] = eTop;

        gl_TessLevelInner[0] = max(eBottom, eTop);
        gl_TessLevelInner[1] = max(eLeft, eRight);
    }
}
