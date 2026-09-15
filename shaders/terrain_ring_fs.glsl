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
