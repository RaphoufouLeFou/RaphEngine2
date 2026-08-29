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
    for (int i = 0; i < cascadeCount; ++i)
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

        vec3 radiance = vec3(lightIntensity);
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

        vec3 ambientHDR = kD_amb * diffuseIBL + specularIBL;
        ambientHDR = vec3(1.0) - exp(-ambientHDR * reflectionExposure);

        ambient = ambientHDR * ao;
    }
    else
    {
        ambient = 0.03 * albedo * ao;
    }

    vec3 color = Lo + ambient + emissive;

    FragColor = vec4(color, 1.0);
}
