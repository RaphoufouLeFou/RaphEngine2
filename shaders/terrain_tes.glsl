#version 450 core

layout(quads, fractional_odd_spacing, ccw) in;

in TCS_OUT
{
    vec2 worldPosXY;
    vec2 heightMapUV;
    flat uint textureLayer;
    vec2 heightRange;
}
tes_in[];

out TES_OUT
{
    vec3 worldPos;
    vec3 normal;
    vec2 heightMapUV;
    flat uint textureLayer;
}
tes_out;

uniform sampler2DArray heightMapArray;
uniform mat4 view;
uniform mat4 projection;
uniform float chunkResolution;

vec2 BilinearMix(vec2 v0, vec2 v1, vec2 v2, vec2 v3, vec2 t)
{
    vec2 bottomEdge = mix(v0, v1, t.x);
    vec2 topEdge = mix(v3, v2, t.x);
    return mix(bottomEdge, topEdge, t.y);
}

float SampleWorldHeight(vec2 uv, uint layer, vec2 heightRange)
{
    float raw = texture(heightMapArray, vec3(uv, float(layer))).r;
    return mix(heightRange.x, heightRange.y, raw);
}

void main()
{
    vec2 uv = BilinearMix(tes_in[0].heightMapUV, tes_in[1].heightMapUV,
                          tes_in[2].heightMapUV, tes_in[3].heightMapUV,
                          gl_TessCoord.xy);

    vec2 worldXY = BilinearMix(tes_in[0].worldPosXY, tes_in[1].worldPosXY,
                               tes_in[2].worldPosXY, tes_in[3].worldPosXY,
                               gl_TessCoord.xy);

    uint layer = tes_in[0].textureLayer;
    vec2 heightRange = tes_in[0].heightRange;

    float worldHeight = SampleWorldHeight(uv, layer, heightRange);

    vec2 texelUV = 1.0 / vec2(textureSize(heightMapArray, 0).xy);
    float worldTexelSize = chunkResolution * texelUV.x;

    float hL = SampleWorldHeight(uv - vec2(texelUV.x, 0.0), layer, heightRange);
    float hR = SampleWorldHeight(uv + vec2(texelUV.x, 0.0), layer, heightRange);
    float hD = SampleWorldHeight(uv - vec2(0.0, texelUV.y), layer, heightRange);
    float hU = SampleWorldHeight(uv + vec2(0.0, texelUV.y), layer, heightRange);

    vec3 normal = normalize(vec3(hL - hR, hD - hU, 2.0 * worldTexelSize));
    vec3 worldPos = vec3(worldXY, worldHeight);

    tes_out.worldPos = worldPos;
    tes_out.normal = normal;
    tes_out.heightMapUV = uv;
    tes_out.textureLayer = layer;

    gl_Position = projection * view * vec4(worldPos, 1.0);
}
