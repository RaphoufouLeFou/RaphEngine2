#pragma once

inline const char* default_shadow_vs_shader = R"(
#version 410 core
layout (location = 0) in vec3 aPos;

uniform mat4 model;

void main()
{
    gl_Position = model * vec4(aPos, 1.0);
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

inline const char* default_shadow_fs_shader = R"(
#version 410 core

void main()
{             
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
uniform sampler2D texture_specular;
uniform sampler2D texture_normal;
uniform sampler2DArrayShadow shadowMap;

uniform vec3 lightDir; // world space
uniform vec3 viewPos;
uniform bool HaveTexture;
uniform bool HaveNormalMap;
uniform bool HaveSpecularMap;
uniform mat4 view;

layout(std140) uniform LightSpaceMatrices
{
    mat4 lightSpaceMatrices[8];
};
uniform float cascadePlaneDistances[8];
uniform int cascadeCount;

float GetCascadeLayer(float depthViewSpace)
{
    for (int i = 0; i < cascadeCount; ++i)
    {
        if (depthViewSpace < cascadePlaneDistances[i])
            return float(i);
    }
    return float(cascadeCount);
}

float ShadowCalculation(vec3 fragPosWorldSpace, vec3 worldNormal)
{
    vec4 fragPosViewSpace = view * vec4(fragPosWorldSpace, 1.0);
    float depthValue = abs(fragPosViewSpace.z);

    int layer = int(GetCascadeLayer(depthValue));

    if (layer == -1)
        layer = cascadeCount;

    float shadow = 0;

    float blendRange = 0.1; // 10% of cascade size
    float transition = 0.0;
    if (layer < cascadeCount)
    {
        float distToEdge = cascadePlaneDistances[layer] - depthValue;
        if (distToEdge < (cascadePlaneDistances[layer] * blendRange))
        {
            transition = 1.0
                - (distToEdge / (cascadePlaneDistances[layer] * blendRange));
        }
    }

    vec4 fragPosLightSpace =
        lightSpaceMatrices[layer] * vec4(fragPosWorldSpace, 1.0);
    vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
    projCoords = projCoords * 0.5 + 0.5;

    float currentDepth = projCoords.z;
    if (currentDepth > 1.0)
        return 0.0;

    float bias = max(0.05 * (1.0 - dot(worldNormal, lightDir)), 0.01);
    bias *= (layer < 2) ? 0.015 : 0.008;

    // 3x3 Hardware PCF
    vec2 texelSize = 1.0 / vec2(textureSize(shadowMap, 0));
    for (int x = -1; x <= 1; x++)
    {
        for (int y = -1; y <= 1; y++)
        {
            shadow += texture(shadowMap,
                              vec4(projCoords.xy + vec2(x, y) * texelSize,
                                   layer, currentDepth - bias));
        }
    }
    shadow /= 9.0;

    return 1.0 - shadow;
}

void main()
{
    vec3 normal;
    if (HaveNormalMap)
    {
        normal = texture(texture_normal, fs_in.TexCoords).rgb * 2.0 - 1.0;
        normal = normalize(normal);
    }
    else
    {
        normal = normalize(transpose(fs_in.TBN) * fs_in.FragNormal);
    }

    vec3 worldNormal = normalize(fs_in.TBN * normal);

    vec3 lightDirT = normalize(fs_in.TangentLightDir);
    vec3 viewDir = normalize(fs_in.TangentViewPos - fs_in.TangentFragPos);
    vec3 halfwayDir = normalize(lightDirT + viewDir);

    vec3 color = vec3(191.0, 64.0, 191.0) / 255.0; // default purple

    if ((fs_in.TexCoords.x > 0.5) != (fs_in.TexCoords.y > 0.5))
        color = vec3(0.0);

    if (HaveTexture)
        color = texture(texture_diffuse, fs_in.TexCoords).rgb;

    vec3 lightColor = vec3(1.0);
    vec3 ambient = 0.1 * lightColor;

    float diff = max(dot(lightDirT, normal), 0.0);
    vec3 diffuse = diff * lightColor;

    float specFact =
        HaveSpecularMap ? texture(texture_specular, fs_in.TexCoords).r : 1.0;
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0) * specFact;
    vec3 specular = vec3(spec);

    float shadow = ShadowCalculation(fs_in.FragPos, worldNormal);

    vec3 lighting = (ambient + (1.0 - shadow) * (diffuse + specular)) * color;
    FragColor = vec4(lighting, 1.0);
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
    mat3 TBN; // tangent → world
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

    // FIX 1: Use the normal matrix to correctly handle non-uniform scaling.
    //         Previously aNormal was passed raw (object space), breaking bias
    //         and non-normal-map lighting entirely.
    mat3 normalMatrix = transpose(inverse(mat3(model)));

    vec3 N = normalize(normalMatrix * aNormal);
    vec3 T = normalize(normalMatrix * aTangent);
    T = normalize(T - dot(T, N) * N); // Gram-Schmidt re-orthogonalization
    vec3 B = cross(N, T);

    // FIX 2: TBN is tangent → world (the natural convention).
    //         Previously it was stored transposed (world → tangent), which
    //         forced the fragment shader to call transpose(TBN) to use it
    //         normally — confusing and error-prone.
    mat3 TBN = mat3(T, B, N); // tangent → world
    mat3 TBN_T = transpose(TBN); // world   → tangent

    vs_out.FragNormal = N;
    vs_out.TBN = TBN; // passed to FS to convert tangent normal → world for bias
    vs_out.TangentLightDir = TBN_T * lightDir;
    vs_out.TangentViewPos = TBN_T * viewPos; // FIX 3: was missing TBN_T *
    vs_out.TangentFragPos = TBN_T * vs_out.FragPos;

    // NOTE: lightPos, lightSpaceMatrix, FragPosLightSpace removed — all dead
    //       code left over from the pre-CSM implementation.

    gl_Position = projection * view * model * vec4(aPos, 1.0);
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

