#pragma once

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
    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
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

inline const char* debug_cascade_fs_shader = R"(
#version 410 core
out vec4 FragColor;

uniform vec4 color;

void main()
{             
    FragColor = color;
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

inline const char* debug_line_fs_shader = R"(
#version 330 core

out vec4 FragColor;

uniform vec3 u_Color;

void main()
{
    FragColor = vec4(u_Color, 1.0);
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

inline const char* default_shadow_gs_shader = R"(
#version 410 core

layout(triangles, invocations = 5) in;
layout(triangle_strip, max_vertices = 3) out;

layout (std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[16];
};
/*
uniform mat4 lightSpaceMatrices[16];
*/

void main()
{          
	for (int i = 0; i < 3; ++i)
	{
		gl_Position = lightSpaceMatrices[gl_InvocationID] * gl_in[i].gl_Position;
		gl_Layer = gl_InvocationID;
		EmitVertex();
	}
	EndPrimitive();
}  

)";

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

inline const char* irradiance_convolution_fs_shader = R"(
#version 410 core
out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;

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

            irradiance += texture(environmentMap, sampleVec).rgb * cos(theta)
                * sin(theta);
            nrSamples++;
        }
    }
    irradiance = PI * irradiance * (1.0 / nrSamples);

    FragColor = vec4(irradiance, 1.0);
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

inline const char* outline_mask_fs_shader = R"(
#version 410 core

layout(location = 0) out float FragColor;
void main()
{
    FragColor = 1.0;
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

inline const char* prefilter_convolution_fs_shader = R"(
#version 410 core

out vec4 FragColor;
in vec3 localPos;

uniform samplerCube environmentMap;
uniform float roughness;

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

void main()
{
    vec3 N = normalize(localPos);
    vec3 R = N;
    vec3 V = R;

    const uint SAMPLE_COUNT = 1024u;
    vec3 prefilteredColor = vec3(0.0);
    float totalWeight = 0.0;

    for (uint i = 0u; i < SAMPLE_COUNT; ++i)
    {
        vec2 Xi = Hammersley(i, SAMPLE_COUNT);
        vec3 H = ImportanceSampleGGX(Xi, N, roughness);
        vec3 L = normalize(2.0 * dot(V, H) * H - V);

        float NdotL = max(dot(N, L), 0.0);
        if (NdotL > 0.0)
        {
            prefilteredColor += texture(environmentMap, L).rgb * NdotL;
            totalWeight += NdotL;
        }
    }

    prefilteredColor =
        totalWeight > 0.0 ? prefilteredColor / totalWeight : vec3(0.0);

    FragColor = vec4(prefilteredColor, 1.0);
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

