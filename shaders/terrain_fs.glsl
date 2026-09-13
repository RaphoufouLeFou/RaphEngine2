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
