#pragma once

inline const char* default_shadow_vs_shader = R"(
#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;

out vec2 TexCoords;

uniform mat4 lightSpaceMatrix;
uniform mat4 model;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = lightSpaceMatrix * model * vec4(aPos, 1.0);
}

)";

inline const char* terrain_ring_fs_shader = R"(
#version 450 core

in VS_OUT
{
    vec3 worldPos;
}
fs_in;

out vec4 FragColor;

uniform sampler2DArray heightRingArray;
uniform sampler2DArray normalRingArray;
uniform sampler2DArray materialGaussianAlbedoArray;
uniform sampler2DArray materialAlbedoLutArray;
uniform sampler2DArray materialNormalArray;
uniform sampler2DArray materialOrmArray; // R = AO, G = roughness, B = metallic
uniform float materialTileSize[4];

uniform vec2 ringOrigin;
uniform float ringWorldSize;
uniform int ringLayer;
uniform float ringTexelCount;

uniform vec2 innerRingOrigin;
uniform float innerRingWorldSize;
uniform bool hasInnerRing;

uniform sampler2DArrayShadow shadowMap;
uniform samplerCube irradianceMap;
uniform bool haveSkybox;
uniform float ambientIntensity;

uniform vec3 viewPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float lightIntensity;

uniform vec3 fogColor;
uniform float fogDensity;

uniform mat4 view;

layout(std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[8];
};
uniform float cascadePlaneDistances[8];
uniform int cascadeCount;

const float PI = 3.14159265359;

const uint kMaterialGrass = 0u;
const uint kMaterialRock = 1u;
const uint kMaterialSnow = 2u;
const uint kMaterialDirt = 3u;
const float kWeightEpsilon = 0.01;
// Must match kInnerMarginTexels in terrain_ring_vs.glsl exactly — this is
// what keeps the discard boundary aligned with where the vertex shader's
// inner morph reaches full convergence.
const float kInnerMarginTexels = 1.5;

float GetCascadeLayer(float depthViewSpace)
{
    for (int i = 0; i < cascadeCount; i++)
    {
        if (depthViewSpace < cascadePlaneDistances[i])
            return float(i);
    }
    return float(cascadeCount);
}

float SampleShadow(vec3 fragPosWorldSpace, int layer)
{
    float shadow = 0;
    vec4 fragPosLightSpace =
        lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if (currentDepth > 1.0)
        return 1.0;

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            shadow += texture(shadowMap,
                              vec4(projCoords.xy + vec2(x, y) * texelSize,
                                   layer, currentDepth));
        }
    }
    shadow /= 9.0;

    return shadow;
}

float ShadowCalculation(vec3 fragPosWorldSpace, vec3 N)
{
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = int(GetCascadeLayer(depthValue));
    if (layer == -1)
        layer = cascadeCount;

    float shadow = SampleShadow(fragPosWorldSpace, layer);
    float blendRange = 0.15;
    if (layer < cascadeCount)
    {
        float distToEdge = cascadePlaneDistances[layer] - depthValue;
        float fadeThreshold = cascadePlaneDistances[layer] * blendRange;

        if (distToEdge < fadeThreshold)
        {
            float transition = 1.0 - (distToEdge / fadeThreshold);
            float shadowNext = SampleShadow(fragPosWorldSpace, layer + 1);
            shadow = mix(shadow, shadowNext, transition);
        }
    }

    return 1.0 - shadow;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ACESFilm(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 ComputeTerrainTangent(vec3 worldPos)
{
    vec3 dp1 = dFdx(worldPos);
    vec3 dp2 = dFdy(worldPos);
    vec2 duv1 = dFdx(worldPos.xy);
    vec2 duv2 = dFdy(worldPos.xy);
    return normalize(dp1 * duv2.y - dp2 * duv1.y);
}

vec2 HashVertex(ivec2 p)
{
    vec2 pf = vec2(p);
    return fract(sin(pf * mat2(127.1, 311.7, 269.5, 183.3)) * 43758.5453);
}

struct TriangleGridResult
{
    float w1, w2, w3;
    ivec2 vertex1, vertex2, vertex3;
};

TriangleGridResult ComputeTriangleGrid(vec2 uv)
{
    uv *= 3.464;

    const mat2 gridToSkewedGrid = mat2(1.0, 0.0, -0.57735027, 1.15470054);
    vec2 skewedCoord = gridToSkewedGrid * uv;

    ivec2 baseId = ivec2(floor(skewedCoord));
    vec3 temp = vec3(fract(skewedCoord), 0.0);
    temp.z = 1.0 - temp.x - temp.y;

    TriangleGridResult result;
    if (temp.z > 0.0)
    {
        result.w1 = temp.z;
        result.w2 = temp.y;
        result.w3 = temp.x;
        result.vertex1 = baseId;
        result.vertex2 = baseId + ivec2(0, 1);
        result.vertex3 = baseId + ivec2(1, 0);
    }
    else
    {
        result.w1 = -temp.z;
        result.w2 = 1.0 - temp.y;
        result.w3 = 1.0 - temp.x;
        result.vertex1 = baseId + ivec2(1, 1);
        result.vertex2 = baseId + ivec2(1, 0);
        result.vertex3 = baseId + ivec2(0, 1);
    }
    return result;
}

struct MaterialSample
{
    vec3 albedo;
    vec3 normalTS;
    float ao;
    float roughness;
    float metallic;
};

MaterialSample SampleMaterial(uint materialIndex, vec2 tiledUV, vec2 duvdx,
                              vec2 duvdy)
{
    TriangleGridResult grid = ComputeTriangleGrid(tiledUV);
    vec2 uv1 = tiledUV + HashVertex(grid.vertex1);
    vec2 uv2 = tiledUV + HashVertex(grid.vertex2);
    vec2 uv3 = tiledUV + HashVertex(grid.vertex3);

    float layer = float(materialIndex);

    vec3 G1 =
        textureGrad(materialGaussianAlbedoArray, vec3(uv1, layer), duvdx, duvdy)
            .rgb;
    vec3 G2 =
        textureGrad(materialGaussianAlbedoArray, vec3(uv2, layer), duvdx, duvdy)
            .rgb;
    vec3 G3 =
        textureGrad(materialGaussianAlbedoArray, vec3(uv3, layer), duvdx, duvdy)
            .rgb;

    vec3 G = grid.w1 * G1 + grid.w2 * G2 + grid.w3 * G3;
    G = (G - vec3(0.5))
            * inversesqrt(grid.w1 * grid.w1 + grid.w2 * grid.w2
                          + grid.w3 * grid.w3)
        + vec3(0.5);

    vec3 albedo;
    albedo.r = texture(materialAlbedoLutArray, vec3(G.r, 0.5, layer)).r;
    albedo.g = texture(materialAlbedoLutArray, vec3(G.g, 0.5, layer)).g;
    albedo.b = texture(materialAlbedoLutArray, vec3(G.b, 0.5, layer)).b;

    vec3 N1 =
        textureGrad(materialNormalArray, vec3(uv1, layer), duvdx, duvdy).rgb;
    vec3 N2 =
        textureGrad(materialNormalArray, vec3(uv2, layer), duvdx, duvdy).rgb;
    vec3 N3 =
        textureGrad(materialNormalArray, vec3(uv3, layer), duvdx, duvdy).rgb;
    vec3 normalSample = grid.w1 * N1 + grid.w2 * N2 + grid.w3 * N3;

    vec3 O1 = textureGrad(materialOrmArray, vec3(uv1, layer), duvdx, duvdy).rgb;
    vec3 O2 = textureGrad(materialOrmArray, vec3(uv2, layer), duvdx, duvdy).rgb;
    vec3 O3 = textureGrad(materialOrmArray, vec3(uv3, layer), duvdx, duvdy).rgb;
    vec3 ormSample = grid.w1 * O1 + grid.w2 * O2 + grid.w3 * O3;

    MaterialSample result;
    result.albedo = albedo;
    result.normalTS = normalize(normalSample * 2.0 - 1.0);
    result.ao = ormSample.r;
    result.roughness = ormSample.g;
    result.metallic = ormSample.b;
    return result;
}

void main()
{
    if (hasInnerRing)
    {
        vec2 innerLocal =
            (fs_in.worldPos.xy - innerRingOrigin) / innerRingWorldSize;
        float signedDist = min(min(innerLocal.x, 1.0 - innerLocal.x),
                               min(innerLocal.y, 1.0 - innerLocal.y));

        float texelSize = ringWorldSize / ringTexelCount;
        float marginNorm =
            (texelSize * kInnerMarginTexels) / innerRingWorldSize;

        if (signedDist >= marginNorm)
        {
            discard;
        }
    }

    vec2 ringUV = (fs_in.worldPos.xy - ringOrigin) / ringWorldSize;
    vec4 ringSample = texture(heightRingArray, vec3(ringUV, float(ringLayer)));
    float rockWeight = ringSample.g;
    float snowWeight = ringSample.b;
    float dirtWeight = ringSample.a;

    vec3 Ngeo =
        normalize(texture(normalRingArray, vec3(ringUV, float(ringLayer))).rgb);

    vec2 tiledUV[4];
    vec2 duvdx[4];
    vec2 duvdy[4];
    for (int m = 0; m < 4; ++m)
    {
        vec2 uv = fs_in.worldPos.xy / materialTileSize[m];
        tiledUV[m] = uv;
        duvdx[m] = dFdx(uv);
        duvdy[m] = dFdy(uv);
    }

    const uint materialOrder[4] =
        uint[](kMaterialGrass, kMaterialRock, kMaterialDirt, kMaterialSnow);
    float weightOrder[4] = float[](1.0, rockWeight, dirtWeight, snowWeight);

    vec3 albedo = vec3(0.0);
    vec3 normalTS = vec3(0.0, 0.0, 1.0);
    float ao = 1.0;
    float roughness = 1.0;
    float metallic = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        if (i > 0 && weightOrder[i] <= kWeightEpsilon)
            continue;

        uint mIdx = materialOrder[i];
        MaterialSample s =
            SampleMaterial(mIdx, tiledUV[mIdx], duvdx[mIdx], duvdy[mIdx]);

        if (i == 0)
        {
            albedo = s.albedo;
            normalTS = s.normalTS;
            ao = s.ao;
            roughness = s.roughness;
            metallic = s.metallic;
        }
        else
        {
            float w = weightOrder[i];
            albedo = mix(albedo, s.albedo, w);
            normalTS = normalize(mix(normalTS, s.normalTS, w));
            ao = mix(ao, s.ao, w);
            roughness = mix(roughness, s.roughness, w);
            metallic = mix(metallic, s.metallic, w);
        }
    }

    vec3 T = ComputeTerrainTangent(fs_in.worldPos);
    T = normalize(T - Ngeo * dot(Ngeo, T));
    vec3 B = cross(Ngeo, T);
    mat3 TBN = mat3(T, B, Ngeo);
    vec3 N = normalize(TBN * normalTS);

    roughness = clamp(roughness, 0.045, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

    vec3 V = normalize(viewPos - fs_in.worldPos);
    float NdotV = max(dot(N, V), 0.0001);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float shadow = 0.0;
    vec3 Lo = vec3(0.0);
    if (lightIntensity > 0.0)
    {
        vec3 L = normalize(-lightDir);
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

        vec3 radiance = lightColor * lightIntensity;
        Lo = (kD * albedo / PI + specular) * radiance * NdotL;

        shadow = ShadowCalculation(fs_in.worldPos, N);
        Lo *= (1.0 - shadow);
    }

    vec3 ambient;
    if (haveSkybox)
    {
        vec3 irradiance = texture(irradianceMap, N).rgb;
        ambient = irradiance * albedo * ambientIntensity * ao;
    }
    else
    {
        ambient = albedo * ao * 0.03;
    }

    const float kShadowAmbientDarkening = 0.4;
    ambient *= mix(1.0, 1.0 - kShadowAmbientDarkening, shadow);

    vec3 color = Lo + ambient;

    float fogDistance = length(viewPos - fs_in.worldPos);
    float fogFactor = clamp(exp(-pow(fogDistance * fogDensity, 2.0)), 0.0, 1.0);
    color = mix(fogColor, color, fogFactor);

    color = ACESFilm(color);
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}

)";

inline const char* skybox_vs_shader = R"(
#version 410 core
layout(location = 0) in vec3 aPos;

out vec3 TexCoords;

uniform mat4 projection;
uniform mat4 view;

void main()
{
    TexCoords = aPos;
    vec4 pos = projection * view * vec4(aPos, 1.0);
    gl_Position = pos.xyww;
}

)";

inline const char* debug_cascade_fs_shader = R"(
#version 410 core
out vec4 FragColor;

uniform vec4 color;

void main()
{             
    FragColor = color;
}

)";

inline const char* outline_mask_vs_shader = R"(
#version 410 core

layout(location = 0) in vec3 aPos;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

)";

inline const char* terrain_vs_shader = R"(
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

)";

inline const char* terrain_tcs_shader = R"(
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

)";

inline const char* default_shadow_gs_shader = R"(
#version 410 core

layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;

layout(std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};
/*
uniform mat4 lightSpaceMatrices[16];
*/

void main()
{
    for (int i = 0; i < 3; i++)
    {
        gl_Position =
            lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
        gl_Layer = gl_InvocationID;
        EmitVertex();
    }
    EndPrimitive();
}

)";

inline const char* startup_screen_fs_shader = R"(
#version 410 core
in vec2 vUV;
out vec4 FragColor;
uniform sampler2D uTex;
void main()
{
    FragColor = texture(uTex, vUV);
}

)";

inline const char* terrain_shadow_fs_shader = R"(
#version 450 core

void main()
{}

)";

inline const char* equirect_to_cubemap_fs_shader = R"(
#version 410 core
out vec4 FragColor;
in vec3 localPos;

uniform sampler2D equirectangularMap;

const vec2 invAtan = vec2(0.1591, 0.3183);
vec2 SampleSphericalMap(vec3 v)
{
    vec2 uv = vec2(atan(v.y, v.x), asin(v.z));
    uv *= invAtan;
    uv += 0.5;
    uv.y = 1.0 - uv.y;
    return uv;
}

void main()
{
    vec2 uv = SampleSphericalMap(normalize(localPos));
    FragColor = vec4(texture(equirectangularMap, uv).rgb, 1.0);
}

)";

inline const char* terrain_fs_shader = R"(
#version 450 core

in TES_OUT
{
    vec3 worldPos;
    vec3 normal;
    vec2 heightMapUV;
    flat uint textureLayer;
}
fs_in;

out vec4 FragColor;

uniform sampler2DArray materialGaussianAlbedoArray;
uniform sampler2DArray materialAlbedoLutArray;
uniform sampler2DArray materialNormalArray;
uniform sampler2DArray materialOrmArray; // R = AO, G = roughness, B = metallic
uniform usampler2DArray paintMaskArray;
uniform float chunkResolution;
uniform float materialTileSize[4]; // must match kMaterials.size() in
                                   // gl_terrain_renderer.cpp

uniform sampler2DArrayShadow shadowMap;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform bool haveSkybox;
uniform float maxPrefilterLod;
uniform float ambientIntensity;
uniform float reflectionExposure;

uniform vec3 lightDir; // world space
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 viewPos;

uniform mat4 view;

layout(std140, binding = 0) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[8];
};
uniform float cascadePlaneDistances[8];
uniform int cascadeCount;

const float PI = 3.14159265359;

const uint kMaterialGrass = 0u;
const uint kMaterialRock = 1u;
const uint kMaterialSnow = 2u;
const uint kMaterialDirt = 3u;

// Large-period scattering scales, in meters, for random dirt/rock patches —
// separate from materialTileSize, which is the texture repeat scale.
const float kRockPatchScale = 60.0;
const float kDirtPatchScale = 45.0;

float GetCascadeLayer(float depthViewSpace)
{
    for (int i = 0; i < cascadeCount; i++)
    {
        if (depthViewSpace < cascadePlaneDistances[i])
            return float(i);
    }
    return float(cascadeCount);
}

float SampleShadow(vec3 fragPosWorldSpace, int layer)
{
    float shadow = 0;
    vec4 fragPosLightSpace =
        lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if (currentDepth > 1.0)
        return 1.0;

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            shadow += texture(shadowMap,
                              vec4(projCoords.xy + vec2(x, y) * texelSize,
                                   layer, currentDepth));
        }
    }
    shadow /= 9.0;

    return shadow;
}

float ShadowCalculation(vec3 fragPosWorldSpace, vec3 N)
{
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = int(GetCascadeLayer(depthValue));

    if (layer == -1)
        layer = cascadeCount;

    float shadow = SampleShadow(fragPosWorldSpace, layer);
    float blendRange = 0.15;
    if (layer < cascadeCount)
    {
        float distToEdge = cascadePlaneDistances[layer] - depthValue;
        float fadeThreshold = cascadePlaneDistances[layer] * blendRange;

        if (distToEdge < fadeThreshold)
        {
            float transition = 1.0 - (distToEdge / fadeThreshold);
            float shadowNext = SampleShadow(fragPosWorldSpace, layer + 1);
            shadow = mix(shadow, shadowNext, transition);
        }
    }

    return 1.0 - shadow;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0
        + (max(vec3(1.0 - roughness), F0) - F0)
        * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ACESFilm(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Large-scale (tens-of-meters) smooth noise, used only to scatter dirt/rock
// patches — unrelated to the per-texel triangle grid below.
float macroHash(vec2 p)
{
    p = 50.0 * fract(p * 0.3183099 + vec2(0.71, 0.113));
    return fract(p.x * p.y * (p.x + p.y));
}

float macroNoise(vec2 p)
{
    vec2 i = floor(p);
    vec2 f = fract(p);
    vec2 u = f * f * (3.0 - 2.0 * f);

    float a = macroHash(i);
    float b = macroHash(i + vec2(1.0, 0.0));
    float c = macroHash(i + vec2(0.0, 1.0));
    float d = macroHash(i + vec2(1.0, 1.0));

    return mix(mix(a, b, u.x), mix(c, d, u.x), u.y);
}

vec2 HashVertex(ivec2 p)
{
    vec2 pf = vec2(p);
    return fract(sin(pf * mat2(127.1, 311.7, 269.5, 183.3)) * 43758.5453);
}

struct TriangleGridResult
{
    float w1, w2, w3;
    ivec2 vertex1, vertex2, vertex3;
};

// Partitions uv-space into an equilateral-triangle lattice and returns the
// barycentric weights and vertex ids of the triangle containing uv, so 3
// randomly offset copies of a tile can be blended to break up periodic
// repetition. Deliot and Heitz, "Procedural Stochastic Textures by Tiling
// and Blending" (2019), Listing 1.2.
TriangleGridResult ComputeTriangleGrid(vec2 uv)
{
    uv *= 3.464; // 2 * sqrt(3): sizes the input relative to the hexagonal tiles

    const mat2 gridToSkewedGrid = mat2(1.0, 0.0, -0.57735027, 1.15470054);
    vec2 skewedCoord = gridToSkewedGrid * uv;

    ivec2 baseId = ivec2(floor(skewedCoord));
    vec3 temp = vec3(fract(skewedCoord), 0.0);
    temp.z = 1.0 - temp.x - temp.y;

    TriangleGridResult result;
    if (temp.z > 0.0)
    {
        result.w1 = temp.z;
        result.w2 = temp.y;
        result.w3 = temp.x;
        result.vertex1 = baseId;
        result.vertex2 = baseId + ivec2(0, 1);
        result.vertex3 = baseId + ivec2(1, 0);
    }
    else
    {
        result.w1 = -temp.z;
        result.w2 = 1.0 - temp.y;
        result.w3 = 1.0 - temp.x;
        result.vertex1 = baseId + ivec2(1, 1);
        result.vertex2 = baseId + ivec2(1, 0);
        result.vertex3 = baseId + ivec2(0, 1);
    }
    return result;
}

struct MaterialSample
{
    vec3 albedo;
    vec3 normalTS;
    float roughness;
    float metallic;
    float ao;
};

// Samples one material's full texture set with tiling-and-blending
// stochastic texturing: 3 randomly offset copies of the tile are blended,
// so the same pattern never repeats identically nearby. Albedo uses the
// histogram-preserving (variance-preserving) blend of the paper's Listing
// 1.5 — naive linear blending visibly flattens contrast and reveals the
// triangle grid. Normal and ORM reuse the same triangle/offsets (so the
// "random pick" stays consistent across maps at a given point) but blend
// linearly, skipping a second Gaussianize+LUT pass per map.
MaterialSample SampleMaterial(uint materialIndex, vec3 worldPos)
{
    vec2 tiledUV = worldPos.xy / materialTileSize[materialIndex];

    // Derivatives must be computed on the un-offset UV — computing them
    // after adding the per-vertex random offset would make mip/anisotropic
    // selection see a discontinuity at every triangle edge.
    vec2 duvdx = dFdx(tiledUV);
    vec2 duvdy = dFdy(tiledUV);

    TriangleGridResult grid = ComputeTriangleGrid(tiledUV);
    vec2 uv1 = tiledUV + HashVertex(grid.vertex1);
    vec2 uv2 = tiledUV + HashVertex(grid.vertex2);
    vec2 uv3 = tiledUV + HashVertex(grid.vertex3);

    float layer = float(materialIndex);

    vec3 G1 =
        textureGrad(materialGaussianAlbedoArray, vec3(uv1, layer), duvdx, duvdy)
            .rgb;
    vec3 G2 =
        textureGrad(materialGaussianAlbedoArray, vec3(uv2, layer), duvdx, duvdy)
            .rgb;
    vec3 G3 =
        textureGrad(materialGaussianAlbedoArray, vec3(uv3, layer), duvdx, duvdy)
            .rgb;

    vec3 G = grid.w1 * G1 + grid.w2 * G2 + grid.w3 * G3;
    G = (G - vec3(0.5))
            * inversesqrt(grid.w1 * grid.w1 + grid.w2 * grid.w2
                          + grid.w3 * grid.w3)
        + vec3(0.5);

    vec3 albedo;
    albedo.r = texture(materialAlbedoLutArray, vec3(G.r, 0.5, layer)).r;
    albedo.g = texture(materialAlbedoLutArray, vec3(G.g, 0.5, layer)).g;
    albedo.b = texture(materialAlbedoLutArray, vec3(G.b, 0.5, layer)).b;

    vec3 N1 =
        textureGrad(materialNormalArray, vec3(uv1, layer), duvdx, duvdy).rgb;
    vec3 N2 =
        textureGrad(materialNormalArray, vec3(uv2, layer), duvdx, duvdy).rgb;
    vec3 N3 =
        textureGrad(materialNormalArray, vec3(uv3, layer), duvdx, duvdy).rgb;
    vec3 normalSample = grid.w1 * N1 + grid.w2 * N2 + grid.w3 * N3;

    vec3 O1 = textureGrad(materialOrmArray, vec3(uv1, layer), duvdx, duvdy).rgb;
    vec3 O2 = textureGrad(materialOrmArray, vec3(uv2, layer), duvdx, duvdy).rgb;
    vec3 O3 = textureGrad(materialOrmArray, vec3(uv3, layer), duvdx, duvdy).rgb;
    vec3 ormSample = grid.w1 * O1 + grid.w2 * O2 + grid.w3 * O3;

    MaterialSample result;
    result.albedo = albedo;
    result.normalTS = normalize(normalSample * 2.0 - 1.0);
    result.ao = ormSample.r;
    result.roughness = ormSample.g;
    result.metallic = ormSample.b;
    return result;
}

MaterialSample BlendMaterials(MaterialSample a, MaterialSample b, float t)
{
    MaterialSample result;
    result.albedo = mix(a.albedo, b.albedo, t);
    result.normalTS = normalize(mix(a.normalTS, b.normalTS, t));
    result.roughness = mix(a.roughness, b.roughness, t);
    result.metallic = mix(a.metallic, b.metallic, t);
    result.ao = mix(a.ao, b.ao, t);
    return result;
}

MaterialSample AutomaticMaterial(vec3 worldPos, float slope)
{
    float rockPatchNoise =
        macroNoise(worldPos.xy / kRockPatchScale + vec2(37.1, 58.9));
    float dirtPatchNoise =
        macroNoise(worldPos.xy / kDirtPatchScale + vec2(91.7, 12.3));

    float rockBySlope = smoothstep(0.55, 0.85, slope);
    float rockByNoise = smoothstep(180.0, 220.0, worldPos.z);
    float rockWeight = max(rockBySlope, rockByNoise);

    float snowByHeight = smoothstep(220.0, 320.0, worldPos.z);
    float snowWeight = snowByHeight * (1.0 - rockWeight * 0.5);

    float dirtByNoise = 0; // smoothstep(0.6, 0.8, dirtPatchNoise);
    float dirtWeight = dirtByNoise * (1.0 - rockWeight) * (1.0 - snowWeight);

    MaterialSample grass = SampleMaterial(kMaterialGrass, worldPos);
    MaterialSample rock = SampleMaterial(kMaterialRock, worldPos);
    MaterialSample dirt = SampleMaterial(kMaterialDirt, worldPos);
    MaterialSample snow = SampleMaterial(kMaterialSnow, worldPos);

    MaterialSample result = BlendMaterials(grass, rock, rockWeight);
    result = BlendMaterials(result, dirt, dirtWeight);
    result = BlendMaterials(result, snow, snowWeight);

    return result;
}

uint SamplePaintIndex(vec2 uv, uint layer)
{
    ivec2 texel = ivec2(uv * chunkResolution);
    texel = clamp(texel, ivec2(0), ivec2(int(chunkResolution) - 1));
    return texelFetch(paintMaskArray, ivec3(texel, int(layer)), 0).r;
}

vec3 ComputeTerrainTangent(vec3 worldPos)
{
    vec3 dp1 = dFdx(worldPos);
    vec3 dp2 = dFdy(worldPos);
    vec2 duv1 = dFdx(worldPos.xy);
    vec2 duv2 = dFdy(worldPos.xy);
    return normalize(dp1 * duv2.y - dp2 * duv1.y);
}

void main()
{
    vec3 Ngeo = normalize(fs_in.normal);

    vec3 T = ComputeTerrainTangent(fs_in.worldPos);
    T = normalize(T - Ngeo * dot(Ngeo, T));
    vec3 B = cross(Ngeo, T);
    mat3 TBN = mat3(T, B, Ngeo);

    float slope = 1.0 - Ngeo.z;

    uint paintIndex = SamplePaintIndex(fs_in.heightMapUV, fs_in.textureLayer);

    MaterialSample surface = (paintIndex == 0u)
        ? AutomaticMaterial(fs_in.worldPos, slope)
        : SampleMaterial(paintIndex - 1u, fs_in.worldPos);

    vec3 N = normalize(TBN * surface.normalTS);
    vec3 albedo = surface.albedo;
    float roughness = clamp(surface.roughness, 0.045, 1.0);
    float metallic = clamp(surface.metallic, 0.0, 1.0);
    float ao = surface.ao;

    vec3 V = normalize(viewPos - fs_in.worldPos);
    float NdotV = max(dot(N, V), 0.0001);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    if (lightIntensity > 0.0)
    {
        vec3 L = normalize(lightDir);
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

        vec3 radiance = lightColor * lightIntensity;
        Lo = (kD * albedo / PI + specular) * radiance * NdotL;

        float shadow = ShadowCalculation(fs_in.worldPos, N);
        Lo *= (1.0 - shadow);
    }

    vec3 ambient;
    if (haveSkybox)
    {
        vec3 F_amb = FresnelSchlickRoughness(NdotV, F0, roughness);
        vec3 kD_amb = (1.0 - F_amb) * (1.0 - metallic);

        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuseIBL = irradiance * albedo * ambientIntensity;

        vec3 R = reflect(-V, N);
        vec3 prefilteredColor =
            textureLod(prefilterMap, R, roughness * maxPrefilterLod).rgb;

        vec2 envBRDF = texture(brdfLUT, vec2(NdotV, roughness)).rg;
        vec3 specularIBL = prefilteredColor * (F_amb * envBRDF.x + envBRDF.y);

        ambient = (kD_amb * diffuseIBL + specularIBL) * ao;
    }
    else
    {
        ambient = 0.03 * albedo * ao;
    }

    vec3 color = Lo + ambient;

    color *= reflectionExposure;
    color = ACESFilm(color);
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}

)";

inline const char* brdf_lut_fs_shader = R"(
#version 410 core

out vec2 FragColor;
in vec2 TexCoord;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float GeometrySchlickGGX_IBL(float NdotV, float roughness)
{
    float a = roughness;
    float k = (a * a) / 2.0;
    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith_IBL(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    return GeometrySchlickGGX_IBL(NdotL, roughness)
        * GeometrySchlickGGX_IBL(NdotV, roughness);
}

vec2 IntegrateBRDF(float NdotV, float roughness)
{
    vec3 V;
    V.x = sqrt(1.0 - NdotV * NdotV);
    V.y = 0.0;
    V.z = NdotV;

    float A = 0.0;
    float B = 0.0;

    vec3 N = vec3(0.0, 0.0, 1.0);

    const uint SAMPLE_COUNT = 1024u;
    for (uint i = 0u; i < SAMPLE_COUNT; i++)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(L.z, 0.0);
        float NdotH = max(H.z, 0.0);
        float VdotH = max(dot(V, H), 0.0);

        if (NdotL > 0.0)
        {
            float G = GeometrySmith_IBL(N, V, L, roughness);
            float G_Vis = (G * VdotH) / (NdotH * NdotV);
            float Fc = pow(1.0 - VdotH, 5.0);

            A += (1.0 - Fc) * G_Vis;
            B += Fc * G_Vis;
        }
    }
    A /= float(SAMPLE_COUNT);
    B /= float(SAMPLE_COUNT);
    return vec2(A, B);
}

void main()
{
    float NdotV = max(TexCoord.x, 1e-3);
    FragColor = IntegrateBRDF(NdotV, TexCoord.y);
}

)";

inline const char* default_shadow_fs_shader = R"(
#version 410 core

in vec2 TexCoords;

uniform bool alphaMask;
uniform float alphaCutoff;
uniform bool HaveTexture;
uniform bool HaveOpacityMap;
uniform sampler2D texture_diffuse;
uniform sampler2D texture_opacity;

void main()
{
    if (alphaMask)
    {
        float alpha = HaveOpacityMap
            ? texture(texture_opacity, TexCoords).r
            : (HaveTexture ? texture(texture_diffuse, TexCoords).a : 1.0);

        if (alpha < alphaCutoff)
            discard;
    }
}

)";

inline const char* startup_screen_vs_shader = R"(
#version 410 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aUV;
out vec2 vUV;
void main()
{
    vUV = aUV;
    gl_Position = vec4(aPos, 0.0, 1.0);
}

)";

inline const char* default_fs_shader = R"(
#version 410 core

out vec4 FragColor;

in VS_OUT
{
    vec3 FragPos; // world space
    vec2 TexCoords;
    vec3 FragNormal; // world space
    vec3 TangentLightDir; // tangent space
    vec3 TangentViewPos; // tangent space
    vec3 TangentFragPos; // tangent space
    mat3 TBN; // tangent -> world
}
fs_in;

uniform sampler2D texture_diffuse;
uniform sampler2D texture_normal;
uniform sampler2D texture_metallic;
uniform sampler2D texture_roughness;
uniform sampler2D texture_ao;
uniform sampler2D texture_emissive;
uniform sampler2D texture_opacity;
uniform sampler2D texture_height;
uniform sampler2DArrayShadow shadowMap;

uniform samplerCube irradianceMap;
uniform samplerCube prefilterMap;
uniform sampler2D brdfLUT;
uniform bool haveSkybox;
uniform float maxPrefilterLod;
uniform float ambientIntensity;
uniform float reflectionExposure;

uniform vec3 lightDir; // world space
uniform vec3 lightColor;
uniform float lightIntensity;
uniform vec3 viewPos;

uniform bool HaveTexture;
uniform bool HaveNormalMap;
uniform bool HaveMetallicMap;
uniform bool HaveRoughnessMap;
uniform bool HaveAOMap;
uniform bool HaveEmissiveMap;
uniform bool HaveOpacityMap;
uniform bool HaveHeightMap;

uniform float metallicFactor;
uniform float roughnessFactor;
uniform vec3 emissiveFactor;
uniform bool metallicRoughnessPacked;
uniform bool alphaMask;
uniform float alphaCutoff;

uniform mat4 view;

layout(std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[8];
};
uniform float cascadePlaneDistances[8];
uniform int cascadeCount;

const float PI = 3.14159265359;

float GetCascadeLayer(float depthViewSpace)
{
    for (int i = 0; i < cascadeCount; i++)
    {
        if (depthViewSpace < cascadePlaneDistances[i])
            return float(i);
    }
    return float(cascadeCount);
}

float SampleShadow(vec3 fragPosWorldSpace, int layer)
{
    float shadow = 0;
    vec4 fragPosLightSpace =
        lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if (currentDepth > 1.0)
        return 1.0;

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            shadow += texture(shadowMap,
                              vec4(projCoords.xy + vec2(x, y) * texelSize,
                                   layer, currentDepth));
        }
    }
    shadow /= 9.0;

    return shadow;
}

float ShadowCalculation(vec3 fragPosWorldSpace, vec3 N)
{
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = int(GetCascadeLayer(depthValue));

    if (layer == -1)
        layer = cascadeCount;

    float shadow = SampleShadow(fragPosWorldSpace, layer);
    float blendRange = 0.15;
    if (layer < cascadeCount)
    {
        float distToEdge = cascadePlaneDistances[layer] - depthValue;
        float fadeThreshold = cascadePlaneDistances[layer] * blendRange;

        if (distToEdge < fadeThreshold)
        {
            float transition = 1.0 - (distToEdge / fadeThreshold);
            float shadowNext = SampleShadow(fragPosWorldSpace, layer + 1);
            shadow = mix(shadow, shadowNext, transition);
        }
    }

    return 1.0 - shadow;
}

const float kParallaxScale = 0.04;

vec2 ParallaxMapping(vec2 texCoords, vec3 viewDirTS)
{
    float height = texture(texture_height, texCoords).r;
    vec2 offset = viewDirTS.xy * (height * kParallaxScale);
    return texCoords - offset;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 FresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
    return F0
        + (max(vec3(1.0 - roughness), F0) - F0)
        * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ACESFilm(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

void main()
{
    vec2 uv = fs_in.TexCoords;
    vec3 viewDirTS = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    if (HaveHeightMap)
        uv = ParallaxMapping(uv, viewDirTS);

    vec4 albedoSample = HaveTexture ? texture(texture_diffuse, uv)
                                    : vec4(191.0, 64.0, 191.0, 255.0) / 255.0;
    vec3 albedo = albedoSample.rgb;

    float alpha =
        HaveOpacityMap ? texture(texture_opacity, uv).r : albedoSample.a;
    if (alphaMask && alpha < alphaCutoff)
        discard;

    vec3 normalTS;
    if (HaveNormalMap)
    {
        normalTS = texture(texture_normal, uv).rgb * 2.0 - 1.0;
        normalTS = normalize(normalTS);
    }
    else
    {
        normalTS = normalize(transpose(fs_in.TBN) * fs_in.FragNormal);
    }
    vec3 N = normalize(fs_in.TBN * normalTS);

    float metallic = metallicFactor;
    float roughness = roughnessFactor;
    if (metallicRoughnessPacked)
    {
        vec3 mr = texture(texture_metallic, uv).rgb;
        roughness *= mr.g;
        metallic *= mr.b;
    }
    else
    {
        if (HaveMetallicMap)
            metallic *= texture(texture_metallic, uv).r;
        if (HaveRoughnessMap)
            roughness *= texture(texture_roughness, uv).r;
    }
    metallic = clamp(metallic, 0.0, 1.0);
    roughness = clamp(roughness, 0.045, 1.0);

    float ao = HaveAOMap ? texture(texture_ao, uv).r : 1.0;

    vec3 emissive =
        HaveEmissiveMap ? texture(texture_emissive, uv).rgb : vec3(1.0);
    emissive *= emissiveFactor;

    vec3 V = normalize(viewPos - fs_in.FragPos);
    float NdotV = max(dot(N, V), 0.0001);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    vec3 Lo = vec3(0.0);
    if (lightIntensity > 0.0)
    {
        vec3 L = normalize(lightDir);
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

        vec3 radiance = lightColor * lightIntensity;
        Lo = (kD * albedo / PI + specular) * radiance * NdotL;

        float shadow = ShadowCalculation(fs_in.FragPos, N);
        Lo *= (1.0 - shadow);
    }

    vec3 ambient;
    if (haveSkybox)
    {
        vec3 F_amb = FresnelSchlickRoughness(NdotV, F0, roughness);
        vec3 kD_amb = (1.0 - F_amb) * (1.0 - metallic);

        vec3 irradiance = texture(irradianceMap, N).rgb;
        vec3 diffuseIBL = irradiance * albedo * ambientIntensity;

        vec3 R = reflect(-V, N);
        vec3 prefilteredColor =
            textureLod(prefilterMap, R, roughness * maxPrefilterLod).rgb;

        vec2 envBRDF = texture(brdfLUT, vec2(NdotV, roughness)).rg;
        vec3 specularIBL = prefilteredColor * (F_amb * envBRDF.x + envBRDF.y);

        ambient = (kD_amb * diffuseIBL + specularIBL) * ao;
    }
    else
    {
        ambient = 0.03 * albedo * ao;
    }

    vec3 color = Lo + ambient + emissive;

    color *= reflectionExposure;
    color = ACESFilm(color);
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}

)";

inline const char* default_instanced_vs_shader = R"(
#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;
layout(location = 5) in mat4 instanceModel; // consumes locations 5, 6, 7, 8

out VS_OUT
{
    vec3 FragPos; // world space
    vec2 TexCoords;
    vec3 FragNormal; // world space
    vec3 TangentLightDir; // tangent space
    vec3 TangentViewPos; // tangent space
    vec3 TangentFragPos; // tangent space
    mat3 TBN; // tangent -> world
}
vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform vec3 lightDir; // world space directional light
uniform vec3 viewPos; // world space camera position

void main()
{
    vs_out.FragPos = vec3(instanceModel * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(instanceModel)));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt re-orthogonalization
    vec3 B = cross(N, T);

    mat3 TBN = mat3(T, B, N);
    mat3 TBN_T = transpose(TBN);

    vs_out.FragNormal = N;
    vs_out.TBN = TBN;
    vs_out.TangentLightDir = TBN_T * lightDir;
    vs_out.TangentViewPos = TBN_T * viewPos;
    vs_out.TangentFragPos = TBN_T * vs_out.FragPos;

    gl_Position = projection * view * instanceModel * vec4(aPos, 1.0);
}
)";

inline const char* terrain_quad_fs_shader = R"(
#version 450 core

in VS_OUT
{
    vec3 worldPos;
    flat vec2 nodeOrigin;
    flat float nodeWorldSize;
    flat int nodeLayer;
}
fs_in;

out vec4 FragColor;

uniform sampler2DArray heightNodeArray;
uniform sampler2DArray normalNodeArray;
uniform usampler2DArray paintNodeArray;
uniform sampler2DArray materialGaussianAlbedoArray;
uniform sampler2DArray materialAlbedoLutArray;
uniform sampler2DArray materialNormalArray;
uniform sampler2DArray materialOrmArray; // R = AO, G = roughness, B = metallic
uniform float materialTileSize[4];

uniform float nodeTexelCount;

uniform sampler2DArrayShadow shadowMap;
uniform samplerCube irradianceMap;
uniform bool haveSkybox;
uniform float ambientIntensity;

uniform vec3 viewPos;
uniform vec3 lightDir;
uniform vec3 lightColor;
uniform float lightIntensity;

uniform mat4 view;

layout(std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[8];
};
uniform float cascadePlaneDistances[8];
uniform int cascadeCount;

const float PI = 3.14159265359;

const uint kMaterialGrass = 0u;
const uint kMaterialRock = 1u;
const uint kMaterialSnow = 2u;
const uint kMaterialDirt = 3u;
const float kWeightEpsilon = 0.01;

// Normal-offset shadow bias, in meters. Compact mesh geometry rarely
// triggers self-shadowing acne badly enough to need this on top of the
// polygon-offset bias already applied during the depth-cast pass -- but
// terrain's continuous, high-curvature surface, especially at slopes
// nearly edge-on to the light, is close to a worst case for shadow-map
// self-occlusion. Only became visible once terrain started casting into
// near cascades (previously only far, low-texel-density cascades ever
// held terrain depth, which hid this). Starting values, not measured --
// worth tuning against how it actually looks.
const float kBaseNormalBias =
    0.05; // minimum offset, even facing the light directly
const float kSlopeNormalBias =
    0.35; // additional offset at a fully grazing angle
const float kCascadeBiasGrowth =
    1.5; // growth per farther cascade layer (coarser texels need more)

float GetCascadeLayer(float depthViewSpace)
{
    for (int i = 0; i < cascadeCount; i++)
    {
        if (depthViewSpace < cascadePlaneDistances[i])
            return float(i);
    }
    return float(cascadeCount);
}

float SampleShadow(vec3 fragPosWorldSpace, vec3 N, int layer)
{
    float NdotL = max(dot(N, normalize(lightDir)), 0.0);
    float slopeScale = clamp(1.0 - NdotL, 0.0, 1.0);
    float cascadeScale = 1.0 + float(layer) * kCascadeBiasGrowth;
    float normalBias =
        (kBaseNormalBias + kSlopeNormalBias * slopeScale) * cascadeScale;

    vec3 biasedPos = fragPosWorldSpace + N * normalBias;

    float shadow = 0;
    vec4 fragPosLightSpace = lightSpaceMatrices[layer] * vec4(biasedPos, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if (currentDepth > 1.0)
        return 1.0;

    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            shadow += texture(shadowMap,
                              vec4(projCoords.xy + vec2(x, y) * texelSize,
                                   layer, currentDepth));
        }
    }
    shadow /= 9.0;

    return shadow;
}

float ShadowCalculation(vec3 fragPosWorldSpace, vec3 N)
{
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = int(GetCascadeLayer(depthValue));
    if (layer == -1)
        layer = cascadeCount;

    float shadow = SampleShadow(fragPosWorldSpace, N, layer);
    float blendRange = 0.15;
    if (layer < cascadeCount)
    {
        float distToEdge = cascadePlaneDistances[layer] - depthValue;
        float fadeThreshold = cascadePlaneDistances[layer] * blendRange;

        if (distToEdge < fadeThreshold)
        {
            float transition = 1.0 - (distToEdge / fadeThreshold);
            float shadowNext = SampleShadow(fragPosWorldSpace, N, layer + 1);
            shadow = mix(shadow, shadowNext, transition);
        }
    }

    return 1.0 - shadow;
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0000001);
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}

vec3 FresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 ACESFilm(vec3 x)
{
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 ComputeTerrainTangent(vec3 worldPos)
{
    vec3 dp1 = dFdx(worldPos);
    vec3 dp2 = dFdy(worldPos);
    vec2 duv1 = dFdx(worldPos.xy);
    vec2 duv2 = dFdy(worldPos.xy);
    return normalize(dp1 * duv2.y - dp2 * duv1.y);
}

vec2 HashVertex(ivec2 p)
{
    vec2 pf = vec2(p);
    return fract(sin(pf * mat2(127.1, 311.7, 269.5, 183.3)) * 43758.5453);
}

struct TriangleGridResult
{
    float w1, w2, w3;
    ivec2 vertex1, vertex2, vertex3;
};

TriangleGridResult ComputeTriangleGrid(vec2 uv)
{
    uv *= 3.464;

    const mat2 gridToSkewedGrid = mat2(1.0, 0.0, -0.57735027, 1.15470054);
    vec2 skewedCoord = gridToSkewedGrid * uv;

    ivec2 baseId = ivec2(floor(skewedCoord));
    vec3 temp = vec3(fract(skewedCoord), 0.0);
    temp.z = 1.0 - temp.x - temp.y;

    TriangleGridResult result;
    if (temp.z > 0.0)
    {
        result.w1 = temp.z;
        result.w2 = temp.y;
        result.w3 = temp.x;
        result.vertex1 = baseId;
        result.vertex2 = baseId + ivec2(0, 1);
        result.vertex3 = baseId + ivec2(1, 0);
    }
    else
    {
        result.w1 = -temp.z;
        result.w2 = 1.0 - temp.y;
        result.w3 = 1.0 - temp.x;
        result.vertex1 = baseId + ivec2(1, 1);
        result.vertex2 = baseId + ivec2(1, 0);
        result.vertex3 = baseId + ivec2(0, 1);
    }
    return result;
}

struct MaterialSample
{
    vec3 albedo;
    vec3 normalTS;
    float ao;
    float roughness;
    float metallic;
};

MaterialSample SampleMaterial(uint materialIndex, vec2 tiledUV, vec2 duvdx,
                              vec2 duvdy)
{
    TriangleGridResult grid = ComputeTriangleGrid(tiledUV);
    vec2 uv1 = tiledUV + HashVertex(grid.vertex1);
    vec2 uv2 = tiledUV + HashVertex(grid.vertex2);
    vec2 uv3 = tiledUV + HashVertex(grid.vertex3);

    float layer = float(materialIndex);

    vec3 G1 =
        textureGrad(materialGaussianAlbedoArray, vec3(uv1, layer), duvdx, duvdy)
            .rgb;
    vec3 G2 =
        textureGrad(materialGaussianAlbedoArray, vec3(uv2, layer), duvdx, duvdy)
            .rgb;
    vec3 G3 =
        textureGrad(materialGaussianAlbedoArray, vec3(uv3, layer), duvdx, duvdy)
            .rgb;

    vec3 G = grid.w1 * G1 + grid.w2 * G2 + grid.w3 * G3;
    G = (G - vec3(0.5))
            * inversesqrt(grid.w1 * grid.w1 + grid.w2 * grid.w2
                          + grid.w3 * grid.w3)
        + vec3(0.5);

    vec3 albedo;
    albedo.r = texture(materialAlbedoLutArray, vec3(G.r, 0.5, layer)).r;
    albedo.g = texture(materialAlbedoLutArray, vec3(G.g, 0.5, layer)).g;
    albedo.b = texture(materialAlbedoLutArray, vec3(G.b, 0.5, layer)).b;

    vec3 N1 =
        textureGrad(materialNormalArray, vec3(uv1, layer), duvdx, duvdy).rgb;
    vec3 N2 =
        textureGrad(materialNormalArray, vec3(uv2, layer), duvdx, duvdy).rgb;
    vec3 N3 =
        textureGrad(materialNormalArray, vec3(uv3, layer), duvdx, duvdy).rgb;
    vec3 normalSample = grid.w1 * N1 + grid.w2 * N2 + grid.w3 * N3;

    vec3 O1 = textureGrad(materialOrmArray, vec3(uv1, layer), duvdx, duvdy).rgb;
    vec3 O2 = textureGrad(materialOrmArray, vec3(uv2, layer), duvdx, duvdy).rgb;
    vec3 O3 = textureGrad(materialOrmArray, vec3(uv3, layer), duvdx, duvdy).rgb;
    vec3 ormSample = grid.w1 * O1 + grid.w2 * O2 + grid.w3 * O3;

    MaterialSample result;
    result.albedo = albedo;
    result.normalTS = normalize(normalSample * 2.0 - 1.0);
    result.ao = ormSample.r;
    result.roughness = ormSample.g;
    result.metallic = ormSample.b;
    return result;
}

void main()
{
    vec2 nodeOrigin = fs_in.nodeOrigin;
    float nodeWorldSize = fs_in.nodeWorldSize;
    int nodeLayer = fs_in.nodeLayer;

    vec2 nodeUV = (fs_in.worldPos.xy - nodeOrigin) / nodeWorldSize;
    vec4 nodeSample = texture(heightNodeArray, vec3(nodeUV, float(nodeLayer)));

    ivec2 paintTexel = clamp(ivec2(nodeUV * nodeTexelCount), ivec2(0),
                             ivec2(int(nodeTexelCount)));
    uint paintIndex =
        texelFetch(paintNodeArray, ivec3(paintTexel, nodeLayer), 0).r;

    float rockWeight;
    float snowWeight;
    float dirtWeight;

    if (paintIndex == 0u)
    {
        rockWeight = nodeSample.g;
        snowWeight = nodeSample.b;
        dirtWeight = nodeSample.a;
    }
    else
    {
        uint paintedMaterial = paintIndex - 1u;
        rockWeight = (paintedMaterial == kMaterialRock) ? 1.0 : 0.0;
        dirtWeight = (paintedMaterial == kMaterialDirt) ? 1.0 : 0.0;
        snowWeight = (paintedMaterial == kMaterialSnow) ? 1.0 : 0.0;
    }

    vec3 Ngeo =
        normalize(texture(normalNodeArray, vec3(nodeUV, float(nodeLayer))).rgb);

    vec2 tiledUV[4];
    vec2 duvdx[4];
    vec2 duvdy[4];
    for (int m = 0; m < 4; ++m)
    {
        vec2 uv = fs_in.worldPos.xy / materialTileSize[m];
        tiledUV[m] = uv;
        duvdx[m] = dFdx(uv);
        duvdy[m] = dFdy(uv);
    }

    const uint materialOrder[4] =
        uint[](kMaterialGrass, kMaterialRock, kMaterialDirt, kMaterialSnow);
    float weightOrder[4] = float[](1.0, rockWeight, dirtWeight, snowWeight);

    vec3 albedo = vec3(0.0);
    vec3 normalTS = vec3(0.0, 0.0, 1.0);
    float ao = 1.0;
    float roughness = 1.0;
    float metallic = 0.0;

    for (int i = 0; i < 4; ++i)
    {
        if (i > 0 && weightOrder[i] <= kWeightEpsilon)
            continue;

        uint mIdx = materialOrder[i];
        MaterialSample s =
            SampleMaterial(mIdx, tiledUV[mIdx], duvdx[mIdx], duvdy[mIdx]);

        if (i == 0)
        {
            albedo = s.albedo;
            normalTS = s.normalTS;
            ao = s.ao;
            roughness = s.roughness;
            metallic = s.metallic;
        }
        else
        {
            float w = weightOrder[i];
            albedo = mix(albedo, s.albedo, w);
            normalTS = normalize(mix(normalTS, s.normalTS, w));
            ao = mix(ao, s.ao, w);
            roughness = mix(roughness, s.roughness, w);
            metallic = mix(metallic, s.metallic, w);
        }
    }

    vec3 T = ComputeTerrainTangent(fs_in.worldPos);
    T = normalize(T - Ngeo * dot(Ngeo, T));
    vec3 B = cross(Ngeo, T);
    mat3 TBN = mat3(T, B, Ngeo);
    vec3 N = normalize(TBN * normalTS);

    roughness = clamp(roughness, 0.045, 1.0);
    metallic = clamp(metallic, 0.0, 1.0);

    vec3 V = normalize(viewPos - fs_in.worldPos);
    float NdotV = max(dot(N, V), 0.0001);
    vec3 F0 = mix(vec3(0.04), albedo, metallic);

    float shadow = 0.0;
    vec3 Lo = vec3(0.0);
    if (lightIntensity > 0.0)
    {
        vec3 L = normalize(lightDir);
        vec3 H = normalize(V + L);
        float NdotL = max(dot(N, L), 0.0);

        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = FresnelSchlick(max(dot(H, V), 0.0), F0);

        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * NdotV * NdotL + 0.0001;
        vec3 specular = numerator / denominator;

        vec3 kD = (vec3(1.0) - F) * (1.0 - metallic);

        vec3 radiance = lightColor * lightIntensity;
        Lo = (kD * albedo / PI + specular) * radiance * NdotL;

        shadow = ShadowCalculation(fs_in.worldPos, N);
        Lo *= (1.0 - shadow);
    }

    vec3 ambient;
    if (haveSkybox)
    {
        vec3 irradiance = texture(irradianceMap, N).rgb;
        ambient = irradiance * albedo * ambientIntensity * ao;
    }
    else
    {
        ambient = albedo * ao * 0.03;
    }

    vec3 color = Lo + ambient;

    color = ACESFilm(color);
    color = pow(color, vec3(1.0 / 2.2));

    FragColor = vec4(color, 1.0);
}

)";

inline const char* terrain_tes_shader = R"(
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

)";

inline const char* debug_line_fs_shader = R"(
#version 330 core

out vec4 FragColor;

uniform vec3 u_Color;

void main()
{
    FragColor = vec4(u_Color, 1.0);
}
)";

inline const char* cubemap_capture_vs_shader = R"(
#version 410 core
layout(location = 0) in vec3 aPos;

uniform mat4 projection;
uniform mat4 view;

out vec3 localPos;

void main()
{
    localPos = aPos;
    gl_Position = projection * view * vec4(localPos, 1.0);
}

)";

inline const char* outline_dilate_h_fs_shader = R"(
#version 410 core

in vec2 TexCoord;
out vec2 FragColor;

uniform sampler2D maskTex;
uniform sampler2D depthTex;
uniform int radius;
uniform vec2 texelSize;

void main()
{
    float coverage = 0.0;
    float nearestDepth = 1.0;
    for (int i = -radius; i <= radius; i++)
    {
        vec2 uv = TexCoord + vec2(texelSize.x * float(i), 0.0);
        float c = texture(maskTex, uv).r;
        if (c > 0.5)
        {
            coverage = 1.0;
            nearestDepth = min(nearestDepth, texture(depthTex, uv).r);
        }
    }
    FragColor = vec2(coverage, nearestDepth);
}

)";

inline const char* default_vs_shader = R"(
#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

out VS_OUT
{
    vec3 FragPos; // world space
    vec2 TexCoords;
    vec3 FragNormal; // world space
    vec3 TangentLightDir; // tangent space
    vec3 TangentViewPos; // tangent space
    vec3 TangentFragPos; // tangent space
    mat3 TBN; // tangent -> world
}
vs_out;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;
uniform vec3 lightDir; // world space directional light
uniform vec3 viewPos; // world space camera position

void main()
{
    vs_out.FragPos = vec3(model * vec4(aPos, 1.0));
    vs_out.TexCoords = aTexCoords;

    mat3 normalMatrix = transpose(inverse(mat3(model)));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt re-orthogonalization
    vec3 B = cross(N, T);

    mat3 TBN = mat3(T, B, N);
    mat3 TBN_T = transpose(TBN);

    vs_out.FragNormal = N;
    vs_out.TBN = TBN; 
    vs_out.TangentLightDir = TBN_T * lightDir;
    vs_out.TangentViewPos = TBN_T * viewPos;
    vs_out.TangentFragPos = TBN_T * vs_out.FragPos;

    gl_Position = projection * view * model * vec4(aPos, 1.0);
}

)";

inline const char* default_instanced_shadow_vs_shader = R"(
#version 410 core

layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;
layout(location = 5) in mat4 instanceModel;

out vec2 TexCoords;

uniform mat4 lightSpaceMatrix;

void main()
{
    TexCoords = aTexCoords;
    gl_Position = lightSpaceMatrix * instanceModel * vec4(aPos, 1.0);
}

)";

inline const char* outline_mask_fs_shader = R"(
#version 410 core

layout(location = 0) out float FragColor;
void main()
{
    FragColor = 1.0;
}

)";

inline const char* outline_dilate_v_composite_fs_shader = R"(
#version 410 core

in vec2 TexCoord;
out vec4 FragColor;

uniform sampler2D hDilatedTex;
uniform sampler2D originalMaskTex;
uniform sampler2D sceneDepthTex;
uniform int radius;
uniform vec2 texelSize;
uniform vec3 outlineColor;
uniform float nearPlane;
uniform float farPlane;

float LinearizeDepth(float depth)
{
    float z = depth * 2.0 - 1.0;
    return (2.0 * nearPlane * farPlane)
        / (farPlane + nearPlane - z * (farPlane - nearPlane));
}

void main()
{
    float coverage = 0.0;
    float nearestDepth = 1.0;
    for (int i = -radius; i <= radius; i++)
    {
        vec2 uv = TexCoord + vec2(0.0, texelSize.y * float(i));
        vec2 s = texture(hDilatedTex, uv).rg;
        if (s.r > 0.5)
        {
            coverage = 1.0;
            nearestDepth = min(nearestDepth, s.g);
        }
    }

    float original = texture(originalMaskTex, TexCoord).r;
    if (coverage < 0.5 || original > 0.5)
        discard;

    float sceneDepthHere = texture(sceneDepthTex, TexCoord).r;

    float linearScene = LinearizeDepth(sceneDepthHere);
    float linearOutline = LinearizeDepth(nearestDepth);

    const float depthBiasWorldUnits = 0.05;
    if (linearScene < linearOutline - depthBiasWorldUnits)
        discard;

    FragColor = vec4(outlineColor, 1.0);
}

)";

inline const char* terrain_quad_vs_shader = R"(
#version 450 core

layout(location = 0) in vec4 aVertex;
layout(location = 1) in vec4 aInstanceData;

uniform sampler2DArray heightNodeArray;

uniform float nodeTexelCount;

uniform mat4 view;
uniform mat4 projection;

out VS_OUT
{
    vec3 worldPos;
    flat vec2 nodeOrigin;
    flat float nodeWorldSize;
    flat int nodeLayer;
}
vs_out;

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

    vec3 worldPos = vec3(worldXY, worldHeight);

    vs_out.worldPos = worldPos;
    vs_out.nodeOrigin = nodeOrigin;
    vs_out.nodeWorldSize = nodeWorldSize;
    vs_out.nodeLayer = nodeLayer;

    gl_Position = projection * view * vec4(worldPos, 1.0);
}

)";

inline const char* skybox_fs_shader = R"(
#version 410 core
out vec4 FragColor;
in vec3 TexCoords;

uniform samplerCube skybox;
uniform float exposure;

void main()
{
    vec3 hdrColor = texture(skybox, TexCoords).rgb;

    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);

    mapped = pow(mapped, vec3(1.0 / 2.2));

    FragColor = vec4(mapped, 1.0);
}

)";

inline const char* fullscreen_triangle_vs_shader = R"(
#version 410 core

out vec2 TexCoord;
void main()
{
    vec2 pos = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    TexCoord = pos;
    gl_Position = vec4(pos * 2.0 - 1.0, 0.0, 1.0);
}

)";

inline const char* terrain_shadow_vs_shader = R"(
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

)";

inline const char* debug_line_vs_shader = R"(
#version 330 core

layout(location = 0) in vec3 aPos;

uniform mat4 u_MVP;

void main()
{
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
)";

inline const char* prefilter_convolution_fs_shader = R"(
#version 410 core
out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;
uniform float roughness;
uniform float envResolution; // per-face resolution of the source cubemap
uniform float maxRadiance;

const float PI = 3.14159265359;

float RadicalInverse_VdC(uint bits)
{
    bits = (bits << 16u) | (bits >> 16u);
    bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
    bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
    bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
    bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
    return float(bits) * 2.3283064365386963e-10;
}

vec2 Hammersley(uint i, uint N)
{
    return vec2(float(i) / float(N), RadicalInverse_VdC(i));
}

vec3 ImportanceSampleGGX(vec2 Xi, vec3 N, float roughness)
{
    float a = roughness * roughness;

    float phi = 2.0 * PI * Xi.x;
    float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));
    float sinTheta = sqrt(1.0 - cosTheta * cosTheta);

    vec3 H;
    H.x = cos(phi) * sinTheta;
    H.y = sin(phi) * sinTheta;
    H.z = cosTheta;

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 tangent = normalize(cross(up, N));
    vec3 bitangent = cross(N, tangent);

    vec3 sampleVec = tangent * H.x + bitangent * H.y + N * H.z;
    return normalize(sampleVec);
}

float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return a2 / max(denom, 0.0000001);
}

void main()
{
    vec3 N = normalize(localPos);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    float saTexel = 4.0 * PI / (6.0 * envResolution * envResolution);

    for (uint i = 0u; i < SAMPLE_COUNT; i++)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            float NdotH = max(dot(N, H), 0.0);
            float HdotV = max(dot(H, V), 0.0);
            float D = DistributionGGX(N, H, roughness);
            float pdf = D * NdotH / (4.0 * HdotV) + 0.0001;

            float saSample = 1.0 / (float(SAMPLE_COUNT) * pdf + 0.0001);
            float mipLevel =
                roughness < 0.01 ? 0.0 : 0.5 * log2(saSample / saTexel);

            vec3 sampleColor = textureLod(environmentMap, L, mipLevel).rgb;
            sampleColor = min(sampleColor, vec3(maxRadiance));

            prefilteredColor += sampleColor * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor =
        totalWeight > 0.0 ? prefilteredColor / totalWeight : vec3(0.0);

    FragColor = vec4(prefilteredColor, 1.0);
}

)";

inline const char* irradiance_convolution_fs_shader = R"(
#version 410 core
out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;
uniform float sourceLod;
uniform float maxRadiance;

const float PI = 3.14159265359;

void main()
{
    vec3 N = normalize(localPos);

    vec3 irradiance = vec3(0.0);

    vec3 up = abs(N.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);
    vec3 right = normalize(cross(up, N));
    up = normalize(cross(N, right));

    float sampleDelta = 0.025;
    float nrSamples = 0.0;
    for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)
    {
        for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)
        {
            vec3 tangentSample =
                vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));
            vec3 sampleVec = tangentSample.x * right + tangentSample.y * up
                + tangentSample.z * N;

            vec3 sampleColor =
                textureLod(environmentMap, sampleVec, sourceLod).rgb;
            sampleColor = min(sampleColor, vec3(maxRadiance));

            irradiance += sampleColor * cos(theta) * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / nrSamples);

    FragColor = vec4(irradiance, 1.0);
}

)";

inline const char* terrain_ring_vs_shader = R"(
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

)";

inline const char* debug_cascade_vs_shader = R"(
#version 410 core
layout (location = 0) in vec3 aPos;

uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position = projection * view * vec4(aPos, 1.0);
}

)";

